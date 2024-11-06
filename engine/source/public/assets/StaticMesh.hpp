#pragma once

#include "../../private/core/Asset.hpp"
#include "../../private/rendering/vk_loader.hpp"

namespace lumina
{
    class StaticMesh : public Asset
    {
    public:
        ~StaticMesh() override;
        
        void Reload() override;
    };

    inline void StaticMesh::Reload()
    {
        Asset::Reload();

        LoadGLTF(source);
    }
}
