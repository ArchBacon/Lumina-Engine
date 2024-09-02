#include "vk_loader.hpp"

#include "stb_image/stb_image.h"
#include "vk_buffer_utils.hpp"
#include "vk_initializers.hpp"
#include "vk_renderer.hpp"
#include "vk_types.hpp"

#include <fastgltf/include/fastgltf/core.hpp>
#include <fastgltf/include/fastgltf/glm_element_traits.hpp>
#include <fastgltf/include/fastgltf/tools.hpp>
#include <fastgltf/include/fastgltf/types.hpp>
#include <glm/gtx/quaternion.hpp>

namespace lumina
{
    std::optional<AllocatedImage> LoadGLTFImage(VulkanRenderer* renderer, fastgltf::Asset& asset, fastgltf::Image& image)
    {
        AllocatedImage newImage {};

        int width, height, nrChannels;

        std::visit(
            fastgltf::visitor {
                [](auto& arg) {},
                [&](fastgltf::sources::URI& filePath) {
                    assert(filePath.fileByteOffset == 0);
                    assert(filePath.uri.isLocalPath());

                    const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
                    if (unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4))
                    {
                        VkExtent3D imageSize;
                        imageSize.width  = width;
                        imageSize.height = height;
                        imageSize.depth  = 1;

                        newImage = renderer->CreateImage(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

                        stbi_image_free(data);
                    }
                },
                [&](fastgltf::sources::Array& array) {
                    if (unsigned char* data = stbi_load_from_memory(array.bytes.data(), static_cast<int>(array.bytes.size()), &width, &height, &nrChannels, 4))
                    {
                        VkExtent3D imageSize;
                        imageSize.width  = width;
                        imageSize.height = height;
                        imageSize.depth  = 1;

                        newImage = renderer->CreateImage(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

                        stbi_image_free(data);
                    }
                },
                [&](fastgltf::sources::BufferView& view) {
                    auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                    auto& buffer     = asset.buffers[bufferView.bufferIndex];

                    std::visit(
                        fastgltf::visitor {
                            [](auto& arg) {},
                            [&](fastgltf::sources::Array& array) {
                                if (unsigned char* data = stbi_load_from_memory(
                                        array.bytes.data() + bufferView.byteOffset,
                                        static_cast<int>(bufferView.byteLength),
                                        &width,
                                        &height,
                                        &nrChannels,
                                        4))
                                {
                                    VkExtent3D imageSize;
                                    imageSize.width  = width;
                                    imageSize.height = height;
                                    imageSize.depth  = 1;

                                    newImage = renderer->CreateImage(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

                                    stbi_image_free(data);
                                }
                            }},
                        buffer.data);
                },
            },
            image.data);
        if (newImage.image == VK_NULL_HANDLE)
        {
            return {};
        }
        else
        {
            return newImage;
        }
    }

    VkFilter ExtractFilter(fastgltf::Filter filter)
    {
        switch (filter)
        {
            case fastgltf::Filter::Nearest:
            case fastgltf::Filter::NearestMipMapNearest:
            case fastgltf::Filter::NearestMipMapLinear:  return VK_FILTER_NEAREST;

            case fastgltf::Filter::Linear:
            case fastgltf::Filter::LinearMipMapNearest:
            case fastgltf::Filter::LinearMipMapLinear:
            default:                                    return VK_FILTER_LINEAR;
        }
    }

    VkSamplerMipmapMode ExtractMipmapMode(fastgltf::Filter filter)
    {
        switch (filter)
        {
            case fastgltf::Filter::NearestMipMapNearest:
            case fastgltf::Filter::LinearMipMapNearest:  return VK_SAMPLER_MIPMAP_MODE_NEAREST;

            case fastgltf::Filter::NearestMipMapLinear:
            case fastgltf::Filter::LinearMipMapLinear:
            default:                                    return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }
    }

    void LoadedGLTF::Draw(const glm::mat4& topMatrix, DrawContext& context)
    {
        for (auto& node : topNodes)
        {
            node->Draw(topMatrix, context);
        }
    }

    void LoadedGLTF::ClearAll()
    {
        VkDevice device = creator->device;

        descriptorPool.DestroyPool(device);
        DestroyBuffer(creator->allocator, materialDataBuffer);

        for (auto& [k, v] : meshes)
        {
            DestroyBuffer(creator->allocator, v->buffers.indexBuffer);
            DestroyBuffer(creator->allocator, v->buffers.vertexBuffer);
        }

        for (auto& [k, v] : images)
        {
            if (v.image == creator->errorCheckerboardImage.image)
            {
                continue;
            }
            creator->DestroyImage(v);
        }

        for (auto& sampler : samplers)
        {
            vkDestroySampler(device, sampler, nullptr);
        }
    }

    std::optional<std::shared_ptr<lumina::LoadedGLTF>> lumina::LoadGLTF(VulkanRenderer* renderer, std::string_view path)
    {
        Log::Info("Loading GLTF file: {}", path);

        auto scene       = std::make_shared<LoadedGLTF>();
        scene->creator   = renderer;
        LoadedGLTF& file = *scene;

        fastgltf::Parser parser {};

        constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers
                                     | fastgltf::Options::LoadExternalBuffers;

        fastgltf::GltfDataBuffer data;
        data.loadFromFile(path);

        fastgltf::Asset gltfAsset;

        std::filesystem::path filePath = path;

        auto type = fastgltf::determineGltfFileType(&data);
        if (type == fastgltf::GltfType::glTF)
        {
            auto load = parser.loadGltf(&data, filePath.parent_path(), gltfOptions);
            if (load)
            {
                gltfAsset = std::move(load.get());
            }
            else
            {
                Log::Error("Failed to load GLTF file: {}", std::to_string(fastgltf::to_underlying(load.error())));
                return {};
            }
        }
        else if (type == fastgltf::GltfType::GLB)
        {
            auto load = parser.loadGltfBinary(&data, filePath.parent_path(), gltfOptions);
            if (load)
            {
                gltfAsset = std::move(load.get());
            }
            else
            {
                Log::Error("Failed to load GLTF file: {}", std::to_string(fastgltf::to_underlying(load.error())));
                return {};
            }
        }
        else
        {
            Log::Error("Failed to load GLTF file: Unknown file type");
            return {};
        }

        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> poolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3}};

        file.descriptorPool.InitializePool(renderer->device, gltfAsset.materials.size(), poolSizes);

        for (fastgltf::Sampler& sampler : gltfAsset.samplers)
        {
            VkSamplerCreateInfo samplerInfo {};
            samplerInfo.sType  = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
            samplerInfo.minLod = 0.0f;

            samplerInfo.magFilter  = ExtractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
            samplerInfo.minFilter  = ExtractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));
            samplerInfo.mipmapMode = ExtractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            VkSampler newSampler;
            vkCreateSampler(renderer->device, &samplerInfo, nullptr, &newSampler);

            file.samplers.push_back(newSampler);
        }

        std::vector<std::shared_ptr<MeshAsset>> meshes;
        std::vector<std::shared_ptr<Node>> nodes;
        std::vector<AllocatedImage> images;
        std::vector<std::shared_ptr<GLTFMaterial>> materials;

        for (fastgltf::Image& image : gltfAsset.images)
        {
            std::optional<AllocatedImage> img = LoadGLTFImage(renderer, gltfAsset, image);

            if (img.has_value())
            {
                images.push_back(*img);
                file.images[image.name.c_str()] = *img;
            }
            else
            {
                images.push_back(renderer->errorCheckerboardImage);
                Log::Warn("GLTF failed to load Texture: {}", image.name);
            }
        }

        file.materialDataBuffer = CreateBuffer(
            renderer->allocator,
            sizeof(GLTFMetallicRoughness::MaterialConstants) * gltfAsset.materials.size(),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);
        int dataIndex {0};

        auto sceneMaterialConstants = static_cast<GLTFMetallicRoughness::MaterialConstants*>(file.materialDataBuffer.allocationInfo.pMappedData);

        for (fastgltf::Material& material : gltfAsset.materials)
        {
            auto newMaterial = std::make_shared<GLTFMaterial>();
            materials.push_back(newMaterial);
            file.materials[material.name.c_str()] = newMaterial;

            GLTFMetallicRoughness::MaterialConstants constants {};
            constants.colorFactors.x = material.pbrData.baseColorFactor[0];
            constants.colorFactors.y = material.pbrData.baseColorFactor[1];
            constants.colorFactors.z = material.pbrData.baseColorFactor[2];
            constants.colorFactors.w = material.pbrData.baseColorFactor[3];

            constants.metallicRoughnessFactors.x = material.pbrData.metallicFactor;
            constants.metallicRoughnessFactors.y = material.pbrData.roughnessFactor;
            sceneMaterialConstants[dataIndex]    = constants;

            auto passType = MaterialPass::MainColor;
            if (material.alphaMode == fastgltf::AlphaMode::Blend)
            {
                passType = MaterialPass::Transparent;
            }

            GLTFMetallicRoughness::MaterialResources materialResources {};
            materialResources.colorImage               = renderer->whiteImage;
            materialResources.colorSampler             = renderer->defaultSamplerLinear;
            materialResources.metallicRoughnessImage   = renderer->whiteImage;
            materialResources.metallicRoughnessSampler = renderer->defaultSamplerLinear;

            materialResources.dataBuffer       = file.materialDataBuffer.buffer;
            materialResources.dataBufferOffset = dataIndex * sizeof(GLTFMetallicRoughness::MaterialConstants);

            if (material.pbrData.baseColorTexture.has_value())
            {
                size_t image   = gltfAsset.textures[material.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
                size_t sampler = gltfAsset.textures[material.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

                materialResources.colorImage   = images[image];
                materialResources.colorSampler = file.samplers[sampler];
            }
            newMaterial->data = renderer->metallicRoughnessMaterial.WriteMaterial(renderer->device, passType, materialResources, file.descriptorPool);
            dataIndex++;
        }

        std::vector<uint32_t> indices;
        std::vector<Vertex> vertices;

        for (fastgltf::Mesh& mesh : gltfAsset.meshes)
        {
            auto newMesh = std::make_shared<MeshAsset>();
            meshes.push_back(newMesh);
            file.meshes[mesh.name.c_str()] = newMesh;
            newMesh->name                  = mesh.name;

            indices.clear();
            vertices.clear();

            for (auto&& primitives : mesh.primitives)
            {
                GeometrySurface newSurface;
                newSurface.startIndex = static_cast<uint32_t>(indices.size());
                newSurface.indexCount = static_cast<uint32_t>(gltfAsset.accessors[primitives.indicesAccessor.value()].count);

                size_t initialVertex = vertices.size();

                //Load indices
                {
                    fastgltf::Accessor& indexAccessor = gltfAsset.accessors[primitives.indicesAccessor.value()];
                    indices.reserve(indices.size() + indexAccessor.count);

                    fastgltf::iterateAccessor<std::uint32_t>(gltfAsset, indexAccessor, [&](std::uint32_t index) {
                        indices.push_back(index + initialVertex);
                    });
                }
                //Load Vertex Positions
                {
                    fastgltf::Accessor& positionAccessor = gltfAsset.accessors[primitives.findAttribute("POSITION")->second];
                    vertices.resize(vertices.size() + positionAccessor.count);

                    fastgltf::iterateAccessorWithIndex<float3>(gltfAsset, positionAccessor, [&](float3 vertex, size_t index) {
                        Vertex newVertex;
                        newVertex.position              = vertex;
                        newVertex.normal                = float3 {1.0f, 0.0f, 0.0f};
                        newVertex.color                 = float4 {1.0f};
                        newVertex.uv_x                  = 0.0f;
                        newVertex.uv_y                  = 0.0f;
                        vertices[initialVertex + index] = newVertex;
                    });
                }
                //Load Vertex Normals
                {
                    auto normals = primitives.findAttribute("NORMAL");
                    if (normals != primitives.attributes.end())
                    {
                        fastgltf::iterateAccessorWithIndex<float3>(gltfAsset, gltfAsset.accessors[normals->second], [&](float3 vertex, size_t index) {
                            vertices[initialVertex + index].normal = vertex;
                        });
                    }
                }
                //Load Vertex UVs
                {
                    auto uv = primitives.findAttribute("TEXCOORD_0");
                    if (uv != primitives.attributes.end())
                    {
                        fastgltf::iterateAccessorWithIndex<float2>(gltfAsset, gltfAsset.accessors[uv->second], [&](float2 vertex, size_t index) {
                            vertices[initialVertex + index].uv_x = vertex.x;
                            vertices[initialVertex + index].uv_y = vertex.y;
                        });
                    }
                }
                //Load Vertex Colors
                {
                    auto colors = primitives.findAttribute("COLOR_0");
                    if (colors != primitives.attributes.end())
                    {
                        fastgltf::iterateAccessorWithIndex<float4>(gltfAsset, gltfAsset.accessors[colors->second], [&](float4 vertex, size_t index) {
                            vertices[initialVertex + index].color = vertex;
                        });
                    }
                }

                if (primitives.materialIndex.has_value())
                {
                    newSurface.material = materials[primitives.materialIndex.value()];
                }
                else
                {
                    newSurface.material = materials[0];
                }

                //BoundingBoxes for Frustum Culling
                float3 minPosition = vertices[initialVertex].position;
                float3 maxPosition = vertices[initialVertex].position;
                for (size_t i = initialVertex; i < vertices.size(); i++)
                {
                    minPosition = glm::min(minPosition, vertices[i].position);
                    maxPosition = glm::max(maxPosition, vertices[i].position);
                }
                newSurface.bounds.origin       = (maxPosition + minPosition) / 2.0f;
                newSurface.bounds.extents      = (maxPosition - minPosition) / 2.0f;
                newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);
                newMesh->surfaces.push_back(newSurface);
            }
            newMesh->buffers = renderer->UploadMesh(indices, vertices);
        }

        for (fastgltf::Node& node : gltfAsset.nodes)
        {
            std::shared_ptr<Node> newNode;

            if (node.meshIndex.has_value())
            {
                newNode                                      = std::make_shared<MeshNode>();
                dynamic_cast<MeshNode*>(newNode.get())->mesh = meshes[*node.meshIndex];
            }
            else
            {
                newNode = std::make_shared<Node>();
            }

            nodes.push_back(newNode);
            file.nodes[node.name.c_str()];

            std::visit(
                fastgltf::visitor {
                    [&](const fastgltf::Node::TransformMatrix& matrix) {
                        memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
                    },
                    [&](const fastgltf::TRS& transform) {
                        float3 translation(transform.translation[0], transform.translation[1], transform.translation[2]);
                        glm::quat rotation(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
                        float3 scale(transform.scale[0], transform.scale[1], transform.scale[2]);

                        glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), translation);
                        glm::mat4 rotationMatrix  = glm::toMat4(rotation);
                        glm::mat4 scaleMatrix     = glm::scale(glm::mat4(1.0f), scale);

                        newNode->localTransform = transformMatrix * rotationMatrix * scaleMatrix;
                    }},
                node.transform);
        }
        for (int i = 0; i < gltfAsset.nodes.size(); i++)
        {
            fastgltf::Node& node             = gltfAsset.nodes[i];
            std::shared_ptr<Node>& sceneNode = nodes[i];

            for (auto& child : node.children)
            {
                sceneNode->children.push_back(nodes[child]);
                nodes[child]->parent = sceneNode;
            }
        }

        for (auto& node : nodes)
        {
            if (node->parent.lock() == nullptr)
            {
                file.topNodes.push_back(node);
                node->RefreshTransforms(glm::mat4 {1.0f});
            }
        }
        return scene;
    }

} // namespace lumina
