﻿#include "vk_initializers.hpp"

//> init_cmd
VkCommandPoolCreateInfo vkinit::CommandPoolCreateInfo(
    uint32_t queueFamilyIndex,
    const VkCommandPoolCreateFlags flags /*= 0*/
)
{
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;

    info.flags = flags;
    return info;
}

VkCommandBufferAllocateInfo vkinit::CommandBufferAllocateInfo(
    const VkCommandPool pool,
    const uint32_t count /*= 1*/
)
{
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext = nullptr;

    info.commandPool = pool;
    info.commandBufferCount = count;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    return info;
}
//< init_cmd

//> init_cmd_draw
VkCommandBufferBeginInfo vkinit::CommandBufferBeginInfo(
    const VkCommandBufferUsageFlags flags /*= 0*/
)
{
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext = nullptr;

    info.pInheritanceInfo = nullptr;
    info.flags = flags;
    return info;
}
//< init_cmd_draw

//> init_sync
VkFenceCreateInfo vkinit::FenceCreateInfo(
    const VkFenceCreateFlags flags /*= 0*/
)
{
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;

    info.flags = flags;

    return info;
}

VkSemaphoreCreateInfo vkinit::SemaphoreCreateInfo(
    const VkSemaphoreCreateFlags flags /*= 0*/
)
{
    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;
    return info;
}
//< init_sync

//> init_submit
VkSemaphoreSubmitInfo vkinit::SemaphoreSubmitInfo(
    const VkPipelineStageFlags2 stageMask,
    const VkSemaphore semaphore
)
{
    VkSemaphoreSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.semaphore = semaphore;
    submitInfo.stageMask = stageMask;
    submitInfo.deviceIndex = 0;
    submitInfo.value = 1;

    return submitInfo;
}

VkCommandBufferSubmitInfo vkinit::CommandBufferSubmitInfo(
    const VkCommandBuffer cmd
)
{
    VkCommandBufferSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info.pNext = nullptr;
    info.commandBuffer = cmd;
    info.deviceMask = 0;

    return info;
}

VkSubmitInfo2 vkinit::SubmitInfo(
    const VkCommandBufferSubmitInfo* cmd,
    const VkSemaphoreSubmitInfo* signalSemaphoreInfo,
    const VkSemaphoreSubmitInfo* waitSemaphoreInfo
)
{
    VkSubmitInfo2 info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info.pNext = nullptr;

    info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
    info.pWaitSemaphoreInfos = waitSemaphoreInfo;

    info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
    info.pSignalSemaphoreInfos = signalSemaphoreInfo;

    info.commandBufferInfoCount = 1;
    info.pCommandBufferInfos = cmd;

    return info;
}
//< init_submit

VkPresentInfoKHR vkinit::PresentInfo()
{
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.pNext = nullptr;

    info.swapchainCount = 0;
    info.pSwapchains = nullptr;
    info.pWaitSemaphores = nullptr;
    info.waitSemaphoreCount = 0;
    info.pImageIndices = nullptr;

    return info;
}

//> color_info
VkRenderingAttachmentInfo vkinit::AttachmentInfo(
    const VkImageView view,
    const VkClearValue* clear,
    const VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/
)
{
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.pNext = nullptr;

    colorAttachment.imageView = view;
    colorAttachment.imageLayout = layout;
    colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    if (clear)
    {
        colorAttachment.clearValue = *clear;
    }

    return colorAttachment;
}
//< color_info

//> depth_info
VkRenderingAttachmentInfo vkinit::DepthAttachmentInfo(
    const VkImageView view,
    const VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/
)
{
    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.pNext = nullptr;

    depthAttachment.imageView = view;
    depthAttachment.imageLayout = layout;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil.depth = 0.f;

    return depthAttachment;
}
//< depth_info

//> render_info
VkRenderingInfo vkinit::RenderingInfo(
    const VkExtent2D renderExtent,
    const VkRenderingAttachmentInfo* colorAttachment,
    const VkRenderingAttachmentInfo* depthAttachment
)
{
    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.pNext = nullptr;

    renderInfo.renderArea = VkRect2D{VkOffset2D{0, 0}, renderExtent};
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = colorAttachment;
    renderInfo.pDepthAttachment = depthAttachment;
    renderInfo.pStencilAttachment = nullptr;

    return renderInfo;
}
//< render_info

//> subresource
VkImageSubresourceRange vkinit::ImageSubresourceRange(
    const VkImageAspectFlags aspectMask
)
{
    VkImageSubresourceRange subImage{};
    subImage.aspectMask = aspectMask;
    subImage.baseMipLevel = 0;
    subImage.levelCount = VK_REMAINING_MIP_LEVELS;
    subImage.baseArrayLayer = 0;
    subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return subImage;
}
//< subresource

VkDescriptorSetLayoutBinding vkinit::DescriptorsetLayoutBinding(
    const VkDescriptorType type,
    const VkShaderStageFlags stageFlags,
    const uint32_t binding
)
{
    VkDescriptorSetLayoutBinding setbind = {};
    setbind.binding = binding;
    setbind.descriptorCount = 1;
    setbind.descriptorType = type;
    setbind.pImmutableSamplers = nullptr;
    setbind.stageFlags = stageFlags;

    return setbind;
}

VkDescriptorSetLayoutCreateInfo vkinit::DescriptorsetLayoutCreateInfo(
    const VkDescriptorSetLayoutBinding* bindings,
    const uint32_t bindingCount
)
{
    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;

    info.pBindings = bindings;
    info.bindingCount = bindingCount;
    info.flags = 0;

    return info;
}

VkWriteDescriptorSet vkinit::WriteDescriptorImage(
    const VkDescriptorType type,
    const VkDescriptorSet dstSet,
    const VkDescriptorImageInfo* imageInfo,
    const uint32_t binding
)
{
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;

    write.dstBinding = binding;
    write.dstSet = dstSet;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = imageInfo;

    return write;
}

VkWriteDescriptorSet vkinit::WriteDescriptorBuffer(
    const VkDescriptorType type,
    const VkDescriptorSet dstSet,
    const VkDescriptorBufferInfo* bufferInfo,
    const uint32_t binding
)
{
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;

    write.dstBinding = binding;
    write.dstSet = dstSet;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = bufferInfo;

    return write;
}

VkDescriptorBufferInfo vkinit::BufferInfo(
    const VkBuffer buffer,
    const VkDeviceSize offset,
    const VkDeviceSize range
)
{
    VkDescriptorBufferInfo binfo{};
    binfo.buffer = buffer;
    binfo.offset = offset;
    binfo.range = range;
    return binfo;
}

//> image_set
VkImageCreateInfo vkinit::ImageCreateInfo(
    const VkFormat format,
    const VkImageUsageFlags usageFlags,
    const VkExtent3D extent
)
{
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;

    info.imageType = VK_IMAGE_TYPE_2D;

    info.format = format;
    info.extent = extent;

    info.mipLevels = 1;
    info.arrayLayers = 1;

    //for MSAA. we will not be using it by default, so default it to 1 sample per pixel.
    info.samples = VK_SAMPLE_COUNT_1_BIT;

    //optimal tiling, which means the image is stored on the best gpu format
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usageFlags;

    return info;
}

VkImageViewCreateInfo vkinit::ImageviewCreateInfo(
    const VkFormat format,
    const VkImage image,
    const VkImageAspectFlags aspectFlags
)
{
    // build a image-view for the depth image to use for rendering
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;

    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.image = image;
    info.format = format;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.aspectMask = aspectFlags;

    return info;
}
//< image_set

VkPipelineLayoutCreateInfo vkinit::PipelineLayoutCreateInfo()
{
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;

    // empty defaults
    info.flags = 0;
    info.setLayoutCount = 0;
    info.pSetLayouts = nullptr;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = nullptr;
    return info;
}

VkPipelineShaderStageCreateInfo vkinit::PipelineShaderStageCreateInfo(
    const VkShaderStageFlagBits stage,
    const VkShaderModule shaderModule,
    const char* entry
)
{
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = nullptr;

    // shader stage
    info.stage = stage;
    // module containing the code for this shader stage
    info.module = shaderModule;
    // the entry point of the shader
    info.pName = entry;
    return info;
}
