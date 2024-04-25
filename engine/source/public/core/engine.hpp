#pragma once

namespace lumina
{
    class Engine
    {
    public:
        void Initialize();
        void Run();
        void Shutdown();
    };
}
    
extern lumina::Engine gEngine;
