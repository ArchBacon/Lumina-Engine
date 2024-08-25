#pragma once
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include "vk_types.hpp"

namespace lumina
{
    AllocatedBuffer CreateBuffer(VmaAllocator allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);     
    void DestroyBuffer(VmaAllocator allocator, const AllocatedBuffer& buffer);
};
