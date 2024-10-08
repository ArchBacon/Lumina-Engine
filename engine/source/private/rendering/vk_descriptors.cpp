﻿#include "vk_descriptors.hpp"

void DescriptorLayoutBuilder::AddBinding(uint32_t binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding newBinding {};
    newBinding.binding         = binding;
    newBinding.descriptorCount = 1;
    newBinding.descriptorType  = type;

    bindings.push_back(newBinding);
}

void DescriptorLayoutBuilder::Clear()
{
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::Build(VkDevice device, VkShaderStageFlags shadeStages, const void* pNext, VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto& binding : bindings)
    {
        binding.stageFlags |= shadeStages;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();
    layoutInfo.pNext        = pNext;
    layoutInfo.flags        = flags;

    VkDescriptorSetLayout setLayout;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &setLayout));

    return setLayout;
}

void DescriptorAllocator::InitializePool(VkDevice device, uint32_t maxSets, tcb::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios)
    {
        poolSizes.push_back(VkDescriptorPoolSize {ratio.type, static_cast<uint32_t>(ratio.ratio) * maxSets});
    }
    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes    = poolSizes.data();
    poolInfo.maxSets       = maxSets;
    poolInfo.flags         = 0;

    vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool);
}

void DescriptorAllocator::ClearPool(VkDevice device) const
{
    vkResetDescriptorPool(device, pool, 0);
}

void DescriptorAllocator::DestroyPool(VkDevice device) const
{
    vkDestroyDescriptorPool(device, pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::Allocate(VkDevice device, VkDescriptorSetLayout layout) const
{
    VkDescriptorSetAllocateInfo allocInfo {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &layout;
    allocInfo.pNext              = nullptr;

    VkDescriptorSet descriptorSet;
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

    return descriptorSet;
}

void DescriptorAllocatorGrowable::InitializePool(VkDevice device, uint32_t initialSets, tcb::span<PoolSizeRatio> poolRatios)
{
    ratios.clear();

    for (auto ratio : poolRatios)
    {
        ratios.push_back(ratio);
    }

    VkDescriptorPool newPool = CreatePool(device, initialSets, poolRatios);

    setsPerPool = initialSets * 2;

    readyPools.push_back(newPool);
}

void DescriptorAllocatorGrowable::ClearPools(VkDevice device)
{
    for (const auto p : readyPools)
    {
        vkResetDescriptorPool(device, p, 0);
    }
    for (const auto p : fullPools)
    {
        vkResetDescriptorPool(device, p, 0);
        readyPools.push_back(p);
    }
    fullPools.clear();
}

void DescriptorAllocatorGrowable::DestroyPool(VkDevice device)
{
    for (const auto p : readyPools)
    {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    readyPools.clear();
    for (const auto p : fullPools)
    {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    fullPools.clear();
}

VkDescriptorSet DescriptorAllocatorGrowable::Allocate(VkDevice device, VkDescriptorSetLayout layout, const void* pNext)
{
    VkDescriptorPool poolToUse = GetPool(device);

    VkDescriptorSetAllocateInfo allocInfo {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = poolToUse;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &layout;
    allocInfo.pNext              = pNext;

    VkDescriptorSet descriptorSet;
    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
        fullPools.push_back(poolToUse);

        poolToUse                = GetPool(device);
        allocInfo.descriptorPool = poolToUse;

        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
    }
    readyPools.push_back(poolToUse);
    return descriptorSet;
}

VkDescriptorPool DescriptorAllocatorGrowable::GetPool(VkDevice device)
{
    VkDescriptorPool newPool;
    if (!readyPools.empty())
    {
        newPool = readyPools.back();
        readyPools.pop_back();
    }
    else
    {
        newPool     = CreatePool(device, setsPerPool, ratios);
        setsPerPool = setsPerPool * 2;
        if (setsPerPool > 4092)
        {
            setsPerPool = 4092;
        }
    }

    return newPool;
}

VkDescriptorPool DescriptorAllocatorGrowable::CreatePool(VkDevice device, uint32_t setCount, tcb::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios)
    {
        poolSizes.push_back(VkDescriptorPoolSize {ratio.type, static_cast<uint32_t>(ratio.ratio) * setCount});
    }

    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes    = poolSizes.data();
    poolInfo.maxSets       = setCount;
    poolInfo.flags         = 0;

    VkDescriptorPool newPool;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &newPool);

    return newPool;
}

void DescriptorWriter::WriteImage(int32_t binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
    VkDescriptorImageInfo& imageInfo = imageInfos.emplace_back(VkDescriptorImageInfo {sampler, image, layout});

    VkWriteDescriptorSet write {};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding      = binding;
    write.dstSet          = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pImageInfo      = &imageInfo;

    writes.push_back(write);
}

void DescriptorWriter::WriteBuffer(int32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
    VkDescriptorBufferInfo& bufferInfo = bufferInfos.emplace_back(VkDescriptorBufferInfo {buffer, offset, size});

    VkWriteDescriptorSet write {};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding      = binding;
    write.dstSet          = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pBufferInfo     = &bufferInfo;

    writes.push_back(write);
}

void DescriptorWriter::Clear()
{
    imageInfos.clear();
    bufferInfos.clear();
    writes.clear();
}

void DescriptorWriter::UpdateSet(VkDevice device, VkDescriptorSet set)
{
    for (auto& write : writes)
    {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
