#include "assets/StaticMesh.hpp"

#include "../core/engine.hpp"
#include "../rendering/vk_loader.hpp"

lumina::StaticMesh::~StaticMesh()
{
    
}

void lumina::StaticMesh::Reload()
{
    auto structureFile = *LoadGLTF(&gEngine.Renderer(), source);
}

