#pragma once

#include "core/span.hpp"
#include "vk_types.hpp"

struct DescriptorLayoutBuilder
{
    std::vector<VkDescriptorSetLayoutBinding> bindings {};

    void AddBinding(uint32_t binding, VkDescriptorType type);
    void Clear();
    VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shadeStages, const void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

struct DescriptorAllocator
{
    struct PoolSizeRatio
    {
        VkDescriptorType type;
        float ratio;
    };

    VkDescriptorPool pool;

    void InitializePool(VkDevice device, uint32_t maxSets, tcb::span<PoolSizeRatio> poolRatios);
    void ClearPool(VkDevice device) const;
    void DestroyPool(VkDevice device) const;

    VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout) const;
};

struct DescriptorAllocatorGrowable
{
    struct PoolSizeRatio
    {
        VkDescriptorType type;
        float ratio;
    };

    void InitializePool(VkDevice device, uint32_t initialSets, tcb::span<PoolSizeRatio> poolRatios);
    void ClearPools(VkDevice device);
    void DestroyPool(VkDevice device);

    VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout, const void* pNext = nullptr);

private:
    VkDescriptorPool GetPool(VkDevice device);
    static VkDescriptorPool CreatePool(VkDevice device, uint32_t setCount, tcb::span<PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio> ratios;
    std::vector<VkDescriptorPool> fullPools;
    std::vector<VkDescriptorPool> readyPools;
    uint32_t setsPerPool {};
};

struct DescriptorWriter
{
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;

    void WriteImage(int32_t binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    void WriteBuffer(int32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

    void Clear();
    void UpdateSet(VkDevice device, VkDescriptorSet set);
};
