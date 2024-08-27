#pragma once

#include "core/types.hpp"

#include <memory>

struct SDL_Window;

namespace lumina
{
    class FileIO;
    class VulkanRenderer;

    class Engine
    {
        std::unique_ptr<FileIO> fileIO {nullptr};
        std::unique_ptr<VulkanRenderer> renderer {nullptr};

        bool stopRendering {false};
        bool running {true};

    public:
        void Initialize();
        void Run();
        void Shutdown();

        [[nodiscard]] FileIO& FileIO() const
        {
            return *fileIO;
        }

        [[nodiscard]] VulkanRenderer& Renderer() const
        {
            return *renderer;
        }
    };
} // namespace lumina

extern lumina::Engine gEngine;
