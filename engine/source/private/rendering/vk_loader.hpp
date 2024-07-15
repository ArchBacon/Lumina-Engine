#pragma once

#include "vk_types.hpp"
#include <unordered_map>
#include <filesystem>

namespace lumina
{
    struct GeometrySurface
    {
        uint32_t startIndex;
        uint32_t indexCount;
    };

    struct MeshAsset
    {
        std::string name;

        std::vector<GeometrySurface> surfaces;
        GPUMeshBuffers buffers;
    };

    class VulkanRenderer;

    std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGLTFMeshes(VulkanRenderer* renderer, const std::filesystem::path& path);
}