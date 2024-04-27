#pragma once

#include "types.hpp"

#include <memory>

struct SDL_Window;

namespace lumina
{
    class FileIO;

    class Engine
    {
        std::unique_ptr<FileIO> fileIO {nullptr};

    public:
        int2 windowExtent {1024, 576};
        SDL_Window* window {nullptr};
        bool running {true};

        void Initialize();
        void Run();
        void Shutdown();

        [[nodiscard]] FileIO& FileIO() const
        {
            return *fileIO;
        }
    };
} // namespace lumina

extern lumina::Engine gEngine;
