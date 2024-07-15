#include "vk_loader.hpp"
#include "stb_image/stb_image.h"
#include "vk_renderer.hpp"
#include "vk_initializers.hpp"
#include "vk_types.hpp"
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/include/fastgltf/glm_element_traits.hpp>
#include <fastgltf/include/fastgltf/tools.hpp>
#include <fastgltf/include/fastgltf/core.hpp>

namespace lumina
{
    std::optional<std::vector<std::shared_ptr<lumina::MeshAsset>>> lumina::LoadGLTFMeshes(VulkanRenderer* renderer, const std::filesystem::path& path)
    {
        Log::Info("Loading GLTF file: " + path.string());

        fastgltf::GltfDataBuffer dataBuffer;
        dataBuffer.loadFromFile(path);

        constexpr auto gltfOption = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

        fastgltf::Asset gltf;
        fastgltf::Parser parser {};

        auto load = parser.loadGltfBinary(&dataBuffer, path.parent_path(), gltfOption);
        if (load)
        {
            gltf = std::move(load.get());
        }
        else
        {
            Log::Error("Failed to load GLTF file: {} \n", fastgltf::to_underlying(load.error()));
            return {};
        }

        std::vector<std::shared_ptr<MeshAsset>> meshes;

        std::vector<uint32_t> indices;
        std::vector<Vertex> vertices;

        // Load Meshes
        for (fastgltf::Mesh& mesh : gltf.meshes)
        {
            MeshAsset newMesh;

            newMesh.name = mesh.name;
            indices.clear();
            vertices.clear();

            // Load Primitives for each mesh
            for (auto&& primitive : mesh.primitives)
            {
                GeometrySurface newSurface;
                newSurface.startIndex = static_cast<uint32_t>(indices.size());
                newSurface.indexCount = static_cast<uint32_t>(gltf.accessors[primitive.indicesAccessor.value()].count);

                size_t initialVertex = vertices.size();

                // Load indices
                {
                    fastgltf::Accessor& indexAccessor = gltf.accessors[primitive.indicesAccessor.value()];
                    indices.reserve(indices.size() + indexAccessor.count);

                    fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAccessor,
                        [&](std::uint32_t index) {
                            indices.push_back(index + initialVertex);
                        });
                }

                // Load Vertex Positions
                {
                    fastgltf::Accessor& positionAccessor = gltf.accessors[primitive.findAttribute("POSITION")->second];
                    vertices.resize(vertices.size() + positionAccessor.count);

                    fastgltf::iterateAccessorWithIndex<float3>(gltf, positionAccessor,
                        [&](const float3 vertex, const size_t index) {
                            Vertex newVertex;
                            newVertex.position = vertex;
                            newVertex.normal = { 1.0f, 0.0f, 0.0f };
                            newVertex.color = float4 { 1.0f };
                            newVertex.uv_x = 0.0f;
                            newVertex.uv_y = 0.0f;
                            vertices[initialVertex + index] = newVertex;
                        });
                }

                // Load Normals
                if (auto normals = primitive.findAttribute("NORMAL"); normals != primitive.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<float3>(gltf, gltf.accessors[normals->second],
                        [&](const float3 vertex, const size_t index) {
                            vertices[initialVertex + index].normal = vertex;
                        });
                }

                // Load UVs
                if (auto uv = primitive.findAttribute("TEXCOORD_0"); uv != primitive.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<float2>(gltf, gltf.accessors[uv->second],
                        [&](const float2 vertex, const size_t index) {
                            vertices[initialVertex + index].uv_x = vertex.x;
                            vertices[initialVertex + index].uv_y = vertex.y;
                        });
                }

                // Load Colors
                if (auto colors = primitive.findAttribute("COLOR_0"); colors != primitive.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<float4>(gltf, gltf.accessors[colors->second],
                        [&](const float4 vertex, const size_t index) {
                            vertices[initialVertex + index].color = vertex;
                        });
                }

                newMesh.surfaces.push_back(newSurface);
            }

            //Display Vertex Normals
            constexpr bool overrideColors = true;
            if (overrideColors)
            {
                for (auto& [position, uv_x, normal, uv_y, color] : vertices)
                {
                    color = float4(normal, 1.0f);
                }
            }

            newMesh.buffers = renderer->UploadMesh(indices, vertices);
            meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newMesh)));
        }
        return meshes;        
    }
    
}// namespace lumina
