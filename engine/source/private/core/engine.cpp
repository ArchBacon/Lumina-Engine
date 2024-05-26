#include "engine.hpp"

#include "../rendering/vk_images.hpp"
#include "../rendering/vk_initializers.hpp"
#include "../rendering/vk_types.hpp"
#include "core/fileio.hpp"
#include "core/log.hpp"

#include <chrono>
#include <SDL/SDL.h>
#include <SDL/SDL_vulkan.h>

lumina::Engine gEngine;

constexpr bool USE_VALIDATION_LAYERS = true;

namespace lumina
{
    void Engine::Initialize()
    {
        Log::Init();

        fileIO = std::make_unique<lumina::FileIO>();
        if (!fileIO)
        {
            throw std::runtime_error("FileIO is not initialized, this should never happen!");
        }

        // Initialize SDL and create a window
        SDL_Init(SDL_INIT_VIDEO);

        const SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN;
        window = SDL_CreateWindow("Lumina Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowExtent.x, windowExtent.y, windowFlags);

        InitVulkan();
        InitSwapchain();
        InitCommands();
        InitSyncStructures();
    }

    void Engine::Run()
    {
        auto previousTime = std::chrono::high_resolution_clock::now();
        SDL_Event e;

        while (running)
        {
            const auto currentTime = std::chrono::high_resolution_clock::now();
            const float deltaTime  = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(currentTime - previousTime).count()) / 1000000.0f;
            previousTime           = currentTime;

            Log::Trace("Engine::Run {}s", deltaTime);
            while (SDL_PollEvent(&e) != 0)
            {
                // Close the window when user alt-f4s or clicks the X button
                if (e.type == SDL_QUIT)
                {
                    running = false;
                }

                if (e.type == SDL_WINDOWEVENT)
                {
                    if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) { stopRendering = true; }
                    if (e.window.event == SDL_WINDOWEVENT_RESTORED) { stopRendering = false; }
                }
            }

            // Do not draw if window is minimized
            if (stopRendering)
            {
                // Throttle the speed to avoid the endless spinning
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            Draw();
        }
    }
    
    void Engine::Draw()
    {
        constexpr uint32_t singleSecond = 1000000000;
        
        // Wait until the GPU has finished rendering the last frame. Timeout of 1 second
        VK_CHECK(vkWaitForFences(device, 1, &GetCurrentFrame().renderFence, true, singleSecond));
        VK_CHECK(vkResetFences(device, 1, &GetCurrentFrame().renderFence));

        // Request image from the swapchain
        uint32_t swapchainImageIndex {};
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain, singleSecond, GetCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex));

        VkCommandBuffer command = GetCurrentFrame().commandBuffer;

        // Now that we are sure that the commands finished executing, we can safely
        // reset the command buffer to begin recording again
        VK_CHECK(vkResetCommandBuffer(command, 0));

        // Begin the command buffer recording. We will use this command buffer exactly once,
        // so we want to let vulkan know that
        VkCommandBufferBeginInfo commandBeginInfo = vkinit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // Start the command buffer recording
        VK_CHECK(vkBeginCommandBuffer(command, &commandBeginInfo));

        // Make the swapchain into writable mode before rendering
        vkutil::TransitionImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        // Make a clear-color from the frame number. This will flash with a 120 frame period.
        VkClearColorValue clearColor {};
        float flash  = std::abs(std::sin(static_cast<float>(frameNumber) / 120.f));
        clearColor = {{0.0f, 0.0f, flash, 1.0f}};

        VkImageSubresourceRange clearRange = vkinit::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

        // Clear image
        vkCmdClearColorImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);

        // Make the swapchain image into presentable mode
        vkutil::TransitionImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        // Finalize the command buffer (we can no longer add commands, but it can now be executed)
        VK_CHECK(vkEndCommandBuffer(command));

        // Prepare the submission to the queue.
        // We want to wait on the presentSemaphore, as the semaphore is signalled when the swapchain is ready
        // we will signal the renderSemaphore to signal that rendering has finished.
        VkCommandBufferSubmitInfo commandInfo = vkinit::CommandBufferSubmitInfo(command);

        VkSemaphoreSubmitInfo waitInfo = vkinit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame().swapchainSemaphore);
        VkSemaphoreSubmitInfo signalInfo = vkinit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame().renderSemaphore);

        VkSubmitInfo2 submit = vkinit::SubmitInfo(&commandInfo, &signalInfo, &waitInfo);

        // Submit the command buffer to the queue and execute it.
        // renderFence will now block until the graphic commands finished execution
        VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, GetCurrentFrame().renderFence));

        // Prepare present
        // This will put the image we just rendered to into the visible window.
        // We want to wait on the renderSemaphore for that, as its necessary that
        // drawing commands have finished before the image is displayed to the user
        VkPresentInfoKHR presentInfo {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.swapchainCount = 1;

        presentInfo.pWaitSemaphores = &GetCurrentFrame().renderSemaphore;
        presentInfo.waitSemaphoreCount = 1;

        presentInfo.pImageIndices = &swapchainImageIndex;

        VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

        // Increase the umber of frames drawn
        ++frameNumber;
    }

    void Engine::Shutdown()
    {
        vkDeviceWaitIdle(device);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            vkDestroyCommandPool(device, frames[i].commandPool, nullptr);

            // Destroy sync objects
            vkDestroyFence(device, frames[i].renderFence, nullptr);
            vkDestroySemaphore(device, frames[i].renderSemaphore, nullptr);
            vkDestroySemaphore(device, frames[i].swapchainSemaphore, nullptr);
        }
        
        DestroySwapchain();

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);

        vkb::destroy_debug_utils_messenger(instance, debugMessenger);
        vkDestroyInstance(instance, nullptr);
        SDL_DestroyWindow(window);
    }

    void Engine::InitVulkan()
    {
        vkb::InstanceBuilder builder {};

        // Make the vulkan instance with basic debug features
        auto resultInstance = builder.set_app_name("Lumina Engine")
            .request_validation_layers(USE_VALIDATION_LAYERS)
            .use_default_debug_messenger()
            .require_api_version(1, 3, 0)
            .build();

        vkb::Instance vkbInstance = resultInstance.value();
        instance = vkbInstance.instance;
        debugMessenger = vkbInstance.debug_messenger;

        SDL_Vulkan_CreateSurface(window, instance, &surface);

        // Vulkan 1.3 features
        VkPhysicalDeviceVulkan13Features features13{};
        features13.dynamicRendering = true;
        features13.synchronization2 = true;

        // Vulkan 1.2 features
        VkPhysicalDeviceVulkan12Features features12{};
        features12.bufferDeviceAddress = true;
        features12.descriptorIndexing = true;

        // Use vkbootstrap to select a GPU
        // We want a GPU that can write to the SDL surface and supports vulkan 1.3 with the correct features.
        vkb::PhysicalDeviceSelector selector {vkbInstance};
        vkb::PhysicalDevice physicalDevice = selector
            .set_minimum_version(1, 3)
            .set_required_features_13(features13)
            .set_required_features_12(features12)
            .set_surface(surface)
            .select()
            .value();

        // Create the final vulkan device
        vkb::DeviceBuilder deviceBuilder {physicalDevice};
        vkb::Device vkbDevice = deviceBuilder.build().value();

        // Get the VkDevice handle used in the rest of a vulkan application
        device = vkbDevice.device;
        chosenGPU = physicalDevice.physical_device;

        // Use vkbootstrap to get a Graphics queue
        graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    }
    
    void Engine::InitSwapchain()
    {
        CreateSwapchain(windowExtent.x, windowExtent.y);
    }
    
    void Engine::InitCommands()
    {
        // Create a command pool for commands submitted to the graphics queue.
        // We also want the pool to allow for resetting of individual command buffers
        VkCommandPoolCreateInfo commandPoolInfo = vkinit::CommandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].commandPool));

            // Allocate the default command buffer that we will use for rendering
            VkCommandBufferAllocateInfo commandAllocateInfo = vkinit::CommandBufferAllocateInfo(frames[i].commandPool, 1);

            VK_CHECK(vkAllocateCommandBuffers(device, &commandAllocateInfo, &frames[i].commandBuffer));
        }
    }
    
    void Engine::InitSyncStructures()
    {
        // Create synchronization structures
        // One fence to control when the GPU has finished rendering the frame,
        // and 2 semaphores to synchronize rendering with the swapchain.
        // We want the fence to start signalled, so we can wait on it on the first frame
        VkFenceCreateInfo fenceCreateInfo = vkinit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::SemaphoreCreateInfo();

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));

            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].swapchainSemaphore));
            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));
        }
    }
    
    void Engine::CreateSwapchain(
        const uint32_t width,
        const uint32_t height
    ) {
        vkb::SwapchainBuilder swapchainBuilder {chosenGPU, device, surface};
        swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        vkb::Swapchain vkbSwapchain = swapchainBuilder
            .set_desired_format(VkSurfaceFormatKHR{swapchainImageFormat, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(width, height)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();

        swapchainExtent = vkbSwapchain.extent;
        // Store swapchain and its related images
        swapchain = vkbSwapchain.swapchain;
        swapchainImages = vkbSwapchain.get_images().value();
        swapchainImageViews = vkbSwapchain.get_image_views().value();
    }
    
    void Engine::DestroySwapchain()
    {
        vkDestroySwapchainKHR(device, swapchain, nullptr);

        // Destroy swapchain resources
        for (const auto& imageView : swapchainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }
    }
} // namespace lumina
