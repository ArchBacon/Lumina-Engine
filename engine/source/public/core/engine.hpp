#pragma once

#include "types.hpp"

namespace lumina
{
    class Engine
    {
    public:
        int2 windowExtent {1024, 576};
        struct SDL_Window* window {nullptr};
        bool bRunning {true};
        
        void Initialize();
        void Run();
        void Shutdown();
    };
} // namespace lumina

extern lumina::Engine gEngine;
