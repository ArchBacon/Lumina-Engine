#pragma once

#include "core/types.hpp"
#include <vkbootstrap/VkBootstrap.h>
#include <memory>

struct SDL_Window;

namespace lumina
{
    class FileIO;

    class Engine
    {
        std::unique_ptr<FileIO> fileIO {nullptr};
        VkInstance instance {}; // Vulkan library handle
        VkDebugUtilsMessengerEXT debugMessenger {}; // Vulkan debug output handle
        VkPhysicalDevice chosenGPU {}; // GPU Chosen as the default device
        VkDevice device {}; // Vulkan device for commands
        VkSurfaceKHR surface {}; // Vulkan window surface
        
        int2 windowExtent {1024, 576};
        SDL_Window* window {nullptr};
        bool running {true};
        
    public:
        void Initialize();
        void Run();
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
    };
} // namespace lumina

extern lumina::Engine gEngine;
