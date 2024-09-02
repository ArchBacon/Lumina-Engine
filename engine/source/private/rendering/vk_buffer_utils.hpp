#pragma once
#include "vk_types.hpp"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace lumina
{
    AllocatedBuffer CreateBuffer(VmaAllocator allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    void DestroyBuffer(VmaAllocator allocator, const AllocatedBuffer& buffer);
}; // namespace lumina
