#pragma once

#include "vk_descriptors.hpp"
#include "vk_types.hpp"
#include <unordered_map>
#include <filesystem>

namespace lumina
{
    class VulkanRenderer;
    
    struct GLTFMaterial
    {
        MaterialInstance data;
    };
    
    struct GeometrySurface
    {
        uint32_t startIndex;
        uint32_t indexCount;
        Bounds bounds;
        std::shared_ptr<GLTFMaterial> material;
    };

    struct MeshAsset
    {
        std::string name;

        std::vector<GeometrySurface> surfaces;
        GPUMeshBuffers buffers;
    };

    struct LoadedGLTF : public  IRenderable
    {
    public:
        std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
        std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
        std::unordered_map<std::string, AllocatedImage> images;
        std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

        std::vector<std::shared_ptr<Node>> topNodes;
        std::vector<VkSampler> samplers;

        DescriptorAllocatorGrowable descriptorPool;
        AllocatedBuffer materialDataBuffer;
        VulkanRenderer* creator;

        ~LoadedGLTF() { ClearAll(); }

        void Draw(const glm::mat4& topMatrix, DrawContext& context) override;

    private:
        void ClearAll();
        
    };
    
    std::optional<std::shared_ptr<LoadedGLTF>> LoadGLTF(VulkanRenderer* renderer, std::string_view path);
}
