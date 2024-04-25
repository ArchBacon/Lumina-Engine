#include "core/engine.hpp"

#include <chrono>
#include <iostream>
#include <SDL2/SDL.h>

lumina::Engine gEngine;

namespace lumina
{
    void Engine::Initialize()
    {
        // Initialize SDL and create a window
        SDL_Init(SDL_INIT_VIDEO);

        const SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN;
        window = SDL_CreateWindow("Lumina Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowExtent.x, windowExtent.y, windowFlags);
    }

    void Engine::Run()
    {
        auto previousTime = std::chrono::high_resolution_clock::now();
        SDL_Event e;

        while (running)
        {
            const auto currentTime = std::chrono::high_resolution_clock::now();
            const float deltaTime  = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(currentTime - previousTime).count()) / 1000000.0f;
            previousTime           = currentTime;

            std::cout << "Engine::Run DeltaTime: " << deltaTime << "s"
                      << "\n";
            while (SDL_PollEvent(&e) != 0)
            {
                // Close the window when user alt-f4s or clicks the X button
                if (e.type == SDL_QUIT)
                {
                    running = false;
                }
            }
        }
    }

    void Engine::Shutdown()
    {
        SDL_DestroyWindow(window);
    }
} // namespace lumina
