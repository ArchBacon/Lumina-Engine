#include "core/engine.hpp"

#include "core/asset_manager.hpp"
#include "core/fileio.hpp"
#include "core/log.hpp"

#include <chrono>
#include <SDL/SDL.h>

lumina::Engine gEngine;

namespace lumina
{
    void Engine::Initialize()
    {
        Log::Init();

        fileIO = std::make_unique<lumina::FileIO>();
        if (!fileIO)
        {
            throw std::runtime_error("FileIO is not initialized, this should never happen!");
        }

        assetManager = std::make_unique<lumina::AssetManager>();
        if (!assetManager)
        {
            throw std::runtime_error("AssetManager is not initialized, this should never happen!");
        }

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

            Log::Trace("Engine::Run {}s", deltaTime);
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
