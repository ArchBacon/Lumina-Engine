#include "core/asset_manager.hpp"

namespace lumina
{
    size_t Asset::nextID = 0;
    void AssetManager::UnloadAssets()
    {
        for (auto it = assets.begin(); it != assets.end();)
        {
            //If the asset is not being used by any other object, unload it.
            if (it->second.use_count() == 1) it = assets.erase(it);
            else ++it;
        }
    }
} // namespace lumina
