#include "core/engine.hpp"

#include <chrono>
#include <iostream>
#include <SDL2/SDL.h>

namespace lumina
{
    Engine gEngine;

void Engine::Initialize()
{
    // Initialize SDL and create a window
    SDL_Init(SDL_INIT_VIDEO);

    const SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN;
    mWindow = SDL_CreateWindow(
        "Lumina Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        mWindowExtent.x,
        mWindowExtent.y,
        windowFlags
    );
}

void Engine::Run()
{
    auto previousTime = std::chrono::high_resolution_clock::now();
    SDL_Event e;
    
    while (mRunning)
    {
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTime = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(currentTime - previousTime).count()) / 1000000.0f;
        previousTime = currentTime;

        std::cout << "Engine::Run DeltaTime: " << deltaTime << "s" << "\n";
        while (SDL_PollEvent(&e) != 0)
        {
            // Close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_QUIT) mRunning = false;
        }
    }
}

void Engine::Shutdown()
{
    SDL_DestroyWindow(mWindow);
}
