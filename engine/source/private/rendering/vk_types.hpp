﻿#pragma once

#include "core/types.hpp"

#include <array>
#include <core/log.hpp>
#include <deque>
#include <functional>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

namespace lumina
{
    #define VK_CHECK(x)                                                               \
        do                                                                            \
        {                                                                             \
        VkResult err = x;                                                         \
        if (err)                                                                  \
        {                                                                         \
        lumina::Log::Info("Detected Vulkan error: {}", string_VkResult(err)); \
        abort();                                                              \
        }                                                                         \
        } while (0)


    struct AllocatedImage
    {
        VkImage image;
        VkImageView imageView;
        VmaAllocation allocation;
        VkExtent3D imageExtent;
        VkFormat imageFormat;
    };

    struct AllocatedBuffer
    {
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
    };

    struct Vertex
    {
        float3 position;
        float uv_x;
        float3 normal;
        float uv_y;
        float4 color;
    };

    struct GPUMeshBuffers
    {
        AllocatedBuffer indexBuffer;
        AllocatedBuffer vertexBuffer;
        VkDeviceAddress vertexBufferDeviceAddress;
    };

    struct GPUDrawPushConstants
    {
        glm::mat4 worldMatrix;
        VkDeviceAddress vertexBufferDeviceAddress;
    };
    
}