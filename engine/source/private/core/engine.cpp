#include "core/engine.hpp"
#include <chrono>
#include <iostream>

Engine gEngine; 

void Engine::Initialize()
{
    std::cout << "Engine::Initialize\n";
}

void Engine::Run()
{
    static float sPassedTime = 0.0f; // Temporary timer to automatically exit the engine

    auto previousTime = std::chrono::high_resolution_clock::now();
    while (sPassedTime <= 2.0f) // Exit engine after 2 seconds
    {
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTime = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(currentTime - previousTime).count()) / 1000000.0f;
        previousTime = currentTime;

        std::cout << "Engine::Run " << sPassedTime << "s - (" << deltaTime << ")" << "\n";
        sPassedTime += deltaTime;
    }
}

void Engine::Shutdown()
{
    std::cout << "Engine::Shutdown\n";
}
