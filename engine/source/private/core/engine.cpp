#include "engine.hpp"

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
            }
        }
    }

    void Engine::Shutdown()
    {
        vkDeviceWaitIdle(device);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            vkDestroyCommandPool(device, frames[i].commandPool, nullptr);
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
    
    void Engine::InitSyncStructures() {}
    
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
