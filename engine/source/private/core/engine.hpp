#pragma once

#include "core/types.hpp"

#include <memory>
#include <vkbootstrap/VkBootstrap.h>

struct SDL_Window;

namespace lumina
{
    class FileIO;

    struct FrameData
    {
        VkCommandPool commandPool {};
        VkCommandBuffer commandBuffer {};
        VkSemaphore swapchainSemaphore {};
        VkSemaphore renderSemaphore {};
        VkFence renderFence {};
    };
    constexpr uint8_t FRAME_OVERLAP = 2;

    class Engine
    {
        std::unique_ptr<FileIO> fileIO {nullptr};
        
        VkInstance instance {}; // Vulkan library handle
        VkDebugUtilsMessengerEXT debugMessenger {}; // Vulkan debug output handle
        VkPhysicalDevice chosenGPU {}; // GPU Chosen as the default device
        VkDevice device {}; // Vulkan device for commands
        VkSurfaceKHR surface {}; // Vulkan window surface
        VkSwapchainKHR swapchain {};
        VkFormat swapchainImageFormat {};

        std::vector<VkImage> swapchainImages {};
        std::vector<VkImageView> swapchainImageViews {};
        VkExtent2D swapchainExtent {};

        uint32_t frameNumber {0};
        FrameData frames[FRAME_OVERLAP];
        VkQueue graphicsQueue {};
        uint32_t graphicsQueueFamily {};
        
        int2 windowExtent {1024, 576};
        SDL_Window* window {nullptr};
        bool running {true};
        bool stopRendering {false};
        
    public:
        void Initialize();
        void Run();
        // This should probably be abstracted away out of the engine class
        // Creating a Graphics API of sorts
        void Draw();
        void Shutdown();

        [[nodiscard]] FileIO& FileIO() const
        {
            return *fileIO;
        }

    private:
        void InitVulkan();
        void InitSwapchain();
        void InitCommands();
        void InitSyncStructures();

        void CreateSwapchain(uint32_t width, uint32_t height);
        void DestroySwapchain();

        FrameData& GetCurrentFrame() { return frames[frameNumber % FRAME_OVERLAP]; }
    };
} // namespace lumina

extern lumina::Engine gEngine;
