#pragma once
#include "core/types.hpp"
#include <memory>
#include <vkbootstrap/VkBootstrap.h>

struct SDL_Window;

namespace lumina
{
    class Engine;
    
    struct FrameData
    {
        VkCommandPool commandPool {};
        VkCommandBuffer commandBuffer {};
        VkSemaphore swapchainSemaphore {};
        VkSemaphore renderSemaphore {};
        VkFence renderFence {};
    };

    constexpr uint8_t FRAME_OVERLAP = 2;
    
    class VulkanRenderer
    {
        friend class Engine;
        
    public:

        VulkanRenderer();
        ~VulkanRenderer();

        VulkanRenderer(const VulkanRenderer&)            = delete;
        VulkanRenderer(VulkanRenderer&&)                 = delete;
        VulkanRenderer& operator=(const VulkanRenderer&) = delete;
        VulkanRenderer& operator=(VulkanRenderer&&)      = delete;
        
        VkInstance instance {};
        VkDebugUtilsMessengerEXT debugMessenger {};
        VkPhysicalDevice chosenGPU {};
        VkDevice device {};
        VkSurfaceKHR surface {};
        VkSwapchainKHR swapchain {};
        VkFormat swapchainImageFormat {};

        std::vector<VkImage> swapchainImages {};
        std::vector<VkImageView> swapchainImageViews {};
        VkExtent2D swapchainExtent {};

        uint32_t frameNumber {0};
        FrameData frames[FRAME_OVERLAP];
        VkQueue graphicsQueue {};
        uint32_t graphicsQueueFamily {};

        int2 windowExtent {1080, 720};
        SDL_Window* window {nullptr};
        bool running {true};
        bool stopRendering {false};

        void Initialize();
        void Run();
        void Draw();
        void Shutdown();

    private:
        void InitVulkan();
        void InitSwapchain();
        void InitCommands();
        void InitSyncStructures();

        void CreateSwapchain(uint32_t width, uint32_t height);
        void DestroySwapchain();

        FrameData& GetCurrentFrame()
        {
            return frames[frameNumber % FRAME_OVERLAP];
        }

        
        
    
    };
}
