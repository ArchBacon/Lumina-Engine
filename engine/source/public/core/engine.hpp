#pragma once

#include "types.hpp"

#include <memory>

struct SDL_Window;

namespace lumina
{
    class FileIO;
    class AssetManager;

    class Engine
    {
        std::unique_ptr<FileIO> fileIO {nullptr};
        std::unique_ptr<AssetManager> assetManager {nullptr};        
        
    public:
        int2 windowExtent {1024, 576};
        SDL_Window* window {nullptr};
        bool running {true};

        void Initialize();
        void Run();
        void Shutdown();

        [[nodiscard]] FileIO& FileIO() const
        {
            return *fileIO;
        }

        [[nodiscard]] AssetManager& AssetManager() const
        { return *assetManager; }
    };
} // namespace lumina

extern lumina::Engine gEngine;
