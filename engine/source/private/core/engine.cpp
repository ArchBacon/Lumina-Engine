#include "engine.hpp"

#include "../rendering/vk_renderer.hpp"
#include "core/fileio.hpp"
#include "core/log.hpp"

#include <chrono>

#include <imgui/include/imgui.h>
#include "imgui/include/imgui_impl_sdl2.h"
#include "imgui/include/imgui_impl_vulkan.h"
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

        renderer = std::make_unique<lumina::VulkanRenderer>();
        if (!renderer)
        {
            throw std::runtime_error("VulkanRenderer is not initialized, this should never happen!");
        }
    }

    void Engine::Run()
    {
        auto previousTime = std::chrono::high_resolution_clock::now();
        SDL_Event e;

        while (running)
        {
            const auto currentTime = std::chrono::high_resolution_clock::now();
            const float elapsed  = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(currentTime - previousTime).count());
            const float deltaTime = elapsed / 1000000.0f;
            const float frameTime = elapsed / 1000.0f;            
            previousTime           = currentTime;

            Log::Trace("Engine::Run {}s", deltaTime);
            while (SDL_PollEvent(&e) != 0)
            {
                renderer->Run();
                // Close the window when user alt-f4s or clicks the X button
                if (e.type == SDL_QUIT)
                {
                    running = false;
                }

                if (e.type == SDL_WINDOWEVENT)
                {
                    if (e.window.event == SDL_WINDOWEVENT_MINIMIZED)
                    {
                        stopRendering = true;
                    }
                    if (e.window.event == SDL_WINDOWEVENT_RESTORED)
                    {
                        stopRendering = false;
                    }
                }
                renderer->mainCamera.ProcessSDLEvent(e);
                ImGui_ImplSDL2_ProcessEvent(&e);
            }

            if (renderer->resized)
            {
                renderer->ResizeSwapchain();
            }

            // Do not draw if window is minimized
            if (stopRendering)
            {
                // Throttle the speed to avoid the endless spinning
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }           

            renderer->stats.frameTime = frameTime;
            renderer->Draw();
        }
    }

    void Engine::Shutdown()
    {
    }
} // namespace lumina
