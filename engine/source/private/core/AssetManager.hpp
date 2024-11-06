#pragma once

#include "Asset.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace lumina
{
    class AssetManager
    {
        std::unordered_map<size_t, std::shared_ptr<Asset>> assets {};
        
    public:
        template <typename T, typename... Args>
        std::shared_ptr<T> Load(Args&&... args);

    protected:
        template <typename T>
        std::shared_ptr<T> Find(size_t id);
    };

    template <typename T, typename... Args>
    std::shared_ptr<T> AssetManager::Load(Args&&... args)
    {
        const std::string path = T::GetPath(args...);
        const auto id = std::hash<std::string>()(path);

        auto asset = Find<T>(id);
        if (asset) return asset;

        assets[id] = std::make_shared<T>(std::forward<Args>(args)...);
        assets[id]->id = id;
        assets[id]->path = path;

        return assets[id]; // TODO: Dynamic pointer cast?
    }
    
    template <typename T>
    inline std::shared_ptr<T> AssetManager::Find(const size_t id)
    {
        if (const auto it = assets.find(id); it != assets.end())
        {
            return it->second;
        }

        return nullptr;
    }
}
