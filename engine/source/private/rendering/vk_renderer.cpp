#include "vk_renderer.hpp"

#include "../rendering/vk_images.hpp"
#include "../rendering/vk_initializers.hpp"
#include "../rendering/vk_types.hpp"

#include <SDL/SDL.h>
#include <SDL/SDL_vulkan.h>

constexpr bool USE_VALIDATION_LAYERS = true;

namespace lumina
{
    VulkanRenderer::VulkanRenderer()
    {
        Initialize();
    }

    VulkanRenderer::~VulkanRenderer()
    {
        Shutdown();
    }

    void VulkanRenderer::Initialize()
    {
        SDL_Init(SDL_INIT_VIDEO);

        const SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN;
        window = SDL_CreateWindow("Lumina", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowExtent.x, windowExtent.y, windowFlags);

        InitVulkan();
        InitSwapchain();
        InitCommands();
        InitSyncStructures();
    }

    void VulkanRenderer::InitVulkan()
    {
        vkb::InstanceBuilder builder {};

        auto resultInstance = builder.set_app_name("Lumina Engine")
        .request_validation_layers(USE_VALIDATION_LAYERS)
        .use_default_debug_messenger()
        .require_api_version(1, 3,0)
        .build();

        vkb::Instance vkbInstance = resultInstance.value();
        instance = vkbInstance.instance;
        debugMessenger = vkbInstance.debug_messenger;

        SDL_Vulkan_CreateSurface(window, instance, &surface);

        VkPhysicalDeviceVulkan13Features features13 {};
        features13.dynamicRendering = true;
        features13.synchronization2 = true;

        VkPhysicalDeviceVulkan12Features features12 {};
        features12.bufferDeviceAddress = true;
        features12.descriptorIndexing = true;

        vkb::PhysicalDeviceSelector selector {vkbInstance};
        vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1,3).set_required_features_13(features13).set_required_features_12(features12).set_surface(surface).select().value();

        vkb::DeviceBuilder deviceBuilder {physicalDevice};
        vkb::Device vkbDevice = deviceBuilder.build().value();

        device = vkbDevice.device;
        chosenGPU = physicalDevice.physical_device;

        graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    }

    void VulkanRenderer::Draw()
    {
        constexpr uint32_t singleSecond = 1000000000;

        VK_CHECK(vkWaitForFences(device, 1, &GetCurrentFrame().renderFence, VK_TRUE, singleSecond));
        VK_CHECK(vkResetFences(device, 1, &GetCurrentFrame().renderFence));

        uint32_t swapchainImageIndex {};
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain, singleSecond, GetCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex));

        VkCommandBuffer command = GetCurrentFrame().commandBuffer;

        VK_CHECK(vkResetCommandBuffer(command, 0));

        VkCommandBufferBeginInfo commandBeginInfo = vkinit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VK_CHECK(vkBeginCommandBuffer(command, &commandBeginInfo));

        vkutil::TransitionImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        VkClearColorValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        float flash = std::abs(std::sin(static_cast<float>(frameNumber) / 120.f));
        clearColor = { 0.0f, 0.0f, flash, 1.0f };

        VkImageSubresourceRange clearRange = vkinit::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

        vkCmdClearColorImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);

        vkutil::TransitionImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        VK_CHECK(vkEndCommandBuffer(command));

        VkCommandBufferSubmitInfo commandInfo = vkinit::CommandBufferSubmitInfo(command);

        VkSemaphoreSubmitInfo waitInfo = vkinit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame().swapchainSemaphore);
        VkSemaphoreSubmitInfo signalInfo = vkinit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame().renderSemaphore);

        VkSubmitInfo2 submit = vkinit::SubmitInfo(&commandInfo, &signalInfo, &waitInfo);

        VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, GetCurrentFrame().renderFence));

        VkPresentInfoKHR presentInfo {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.swapchainCount = 1;

        presentInfo.pWaitSemaphores = &GetCurrentFrame().renderSemaphore;
        presentInfo.waitSemaphoreCount = 1;

        presentInfo.pImageIndices = &swapchainImageIndex;

        VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

        ++frameNumber;
    }

    void VulkanRenderer::Shutdown()
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

        vkb::destroy_debug_utils_messenger(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
        SDL_DestroyWindow(window);            
    }



    void VulkanRenderer::InitSwapchain()
    {
        CreateSwapchain(windowExtent.x, windowExtent.y);
    }

    void VulkanRenderer::InitCommands()
    {
        VkCommandPoolCreateInfo commandPoolInfo = vkinit::CommandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].commandPool));
            VkCommandBufferAllocateInfo commandAllocateInfo = vkinit::CommandBufferAllocateInfo(frames[i].commandPool, 1);
            VK_CHECK(vkAllocateCommandBuffers(device, &commandAllocateInfo, &frames[i].commandBuffer));
        }
    }

    void VulkanRenderer::InitSyncStructures()
    {
        VkFenceCreateInfo fenceCreateInfo = vkinit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::SemaphoreCreateInfo();

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));
            
            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].swapchainSemaphore));
            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));
        }
    }

    void VulkanRenderer::CreateSwapchain(uint32_t width, uint32_t height)
    {
        vkb::SwapchainBuilder swapchainBuilder {chosenGPU, device, surface};
        swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        vkb::Swapchain vkbSwapchain = swapchainBuilder.set_desired_format(VkSurfaceFormatKHR {swapchainImageFormat, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();
        swapchainExtent = vkbSwapchain.extent;
        swapchain = vkbSwapchain.swapchain;
        swapchainImages = vkbSwapchain.get_images().value();
        swapchainImageViews = vkbSwapchain.get_image_views().value();
    }

    void VulkanRenderer::DestroySwapchain()
    {
        vkDestroySwapchainKHR(device, swapchain, nullptr);

        for (const auto& imageView : swapchainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }
    }







}
