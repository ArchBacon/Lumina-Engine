#pragma once
#include <memory>
#include <unordered_map>
#include "core/asset.hpp"

namespace lumina
{
    class Asset;
    class Engine;

    class AssetManager
    {
    private:
        friend class Engine;
        std::unordered_map<uint32_t, std::shared_ptr<Asset>> assets;
        
    public:        
        template <typename T, typename... Args>
        std::shared_ptr<T> CreateAsset(Args&&... args)
        {
            //Check if T is derived from Asset
            static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset");

            //Check if the asset has a valid path
            auto asset = std::make_shared<T>(std::forward<Args>(args)...);
            const std::string& path = asset->GetFilePath();            
            static_assert(!path.empty(), "Asset must have a valid path");

            //Check if the asset already exists
            const auto id = T::GenerateID(path);
            static_assert(!FindAsset<T>(id), "Asset already exists");

            //If everything is correct, add the asset to the map
            assets[id] = asset;
            return std::static_pointer_cast<T>(assets[id]);
        }

        //TODO: Make this Aysnc
        template <typename T, typename... Args>
        std::shared_ptr<T> LoadAsset(Args&&... args)
        {
            static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset");
            const std::string filePath = T::GetFilePath(args...);
            const auto id = T::GenerateID(filePath);

            if (const auto asset = FindAsset<T>(id))
                return asset;
            
            assets[id] = std::make_shared<T>(std::forward<Args>(args)...);
            assets[id]->id = id;
            assets[id]->filePath = filePath;

            return std::static_pointer_cast<T>(assets[id]);
        }

        template <typename T>
        std::shared_ptr<T> FindAsset(const uint32_t id)
        {
            static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset");
            if (const auto it = assets.find(id); it != assets.end())
            {
                //Static cast because I already check the type to derive from Asset. Reducing overhead.
                return std::static_pointer_cast<T>(it->second);
            }
            return nullptr;
        }

        //Unload assets that are not being used by any other object.
        //NOTE: Might not be performant with large amount of assets.
        void UnloadAssets();
    };
}
