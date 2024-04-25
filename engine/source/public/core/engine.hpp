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
} // namespace lumina

extern lumina::Engine gEngine;
