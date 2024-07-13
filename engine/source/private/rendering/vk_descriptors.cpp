#include "vk_descriptors.hpp"


void DescriptorLayoutBuilder::AddBinding(uint32_t binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding newBinding {};
    newBinding.binding            = binding;
    newBinding.descriptorCount    = 1;
    newBinding.descriptorType     = type;

    bindings.push_back(newBinding);
}
void DescriptorLayoutBuilder::Clear()
{
    bindings.clear();
}
VkDescriptorSetLayout DescriptorLayoutBuilder::Build(VkDevice device, VkShaderStageFlags shadeStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
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
        
        poolSizes.push_back(VkDescriptorPoolSize{ratio.type, static_cast<uint32_t>(ratio.ratio) * maxSets});
    }
    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes    = poolSizes.data();
    poolInfo.maxSets       = maxSets;
    poolInfo.flags         = 0;

    vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool);
}
void DescriptorAllocator::ClearDescriptors(VkDevice device)
{
    vkResetDescriptorPool(device, pool, 0);
}
void DescriptorAllocator::DestroyPool(VkDevice device)
{
    vkDestroyDescriptorPool(device, pool, nullptr);
}
VkDescriptorSet DescriptorAllocator::Allocate(VkDevice device, VkDescriptorSetLayout layout)
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
