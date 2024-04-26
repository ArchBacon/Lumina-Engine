#pragma once

#include "types.hpp"

#include <SDL2/SDL_video.h>

namespace lumina
{
    class Engine
    {
    public:
        int2 windowExtent {1024, 576};
        SDL_Window* window {nullptr};
        bool running {true};

        void Initialize();
        void Run();
        void Shutdown();
    };
} // namespace lumina

extern lumina::Engine gEngine;
