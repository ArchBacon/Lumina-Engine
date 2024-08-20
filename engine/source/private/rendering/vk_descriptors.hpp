#pragma once

#include "vk_types.hpp"
#include "core/span.hpp"

struct DescriptorLayoutBuilder
{
    std::vector<VkDescriptorSetLayoutBinding> bindings {};

    void AddBinding(uint32_t binding, VkDescriptorType type);
    void Clear();
    VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shadeStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
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
    void ClearPool(VkDevice device);
    void DestroyPool(VkDevice device);

    VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout);
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

    VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext = nullptr);

private:
    VkDescriptorPool GetPool(VkDevice device);
    VkDescriptorPool CreatePool(VkDevice device, uint32_t setCount, tcb::span<PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio> ratios;
    std::vector<VkDescriptorPool> fullPools;
    std::vector<VkDescriptorPool> readyPools;
    uint32_t setsPerPool{};
    
};
