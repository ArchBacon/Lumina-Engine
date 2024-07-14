#include "engine.hpp"

#include "../rendering/vk_renderer.hpp"
#include "core/fileio.hpp"
#include "core/log.hpp"

#include <chrono>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_vulkan.h"
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
                ImGui_ImplSDL2_ProcessEvent(&e);
            }

            // Do not draw if window is minimized
            if (stopRendering)
            {
                // Throttle the speed to avoid the endless spinning
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            ImGui::ShowDemoWindow();
            ImGui::Render();

            renderer->Draw();
        }
    }

    void Engine::Shutdown()
    {
    }
} // namespace lumina
