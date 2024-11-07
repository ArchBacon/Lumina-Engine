#pragma once

#include "Asset.hpp"

namespace lumina
{
    class StaticMesh : public Asset
    {
    public:
        ~StaticMesh() override;
        
        void Reload() override;
    };
}
