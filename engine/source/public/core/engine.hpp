#pragma once

#include "types.hpp"

namespace lumina
{
    class Engine
    {
    public:
        int2 mWindowExtent {1024, 576};
        struct SDL_Window* mWindow {nullptr};
        bool mRunning {true};
        
        void Initialize();
        void Run();
        void Shutdown();
    };
} // namespace lumina

extern lumina::Engine gEngine;
