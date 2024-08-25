#include "vk_buffer_utils.hpp"

namespace lumina
{
    AllocatedBuffer CreateBuffer(const VmaAllocator allocator, const size_t allocSize, const VkBufferUsageFlags usage, const VmaMemoryUsage memoryUsage)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = allocSize;
        bufferInfo.usage = usage;
        bufferInfo.pNext = nullptr;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = memoryUsage;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        AllocatedBuffer newBuffer;

        VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.allocationInfo));

        return newBuffer;
    }
    void DestroyBuffer(const VmaAllocator allocator, const AllocatedBuffer& buffer)
    {
        vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
    }
}


