// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "vk_types.hpp"

namespace vkinit
{
    //> init_cmd
    VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
    VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1);
    //< init_cmd

    VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
    VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd);

    VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0);

    VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

    VkSubmitInfo2 SubmitInfo(const VkCommandBufferSubmitInfo* cmd, const VkSemaphoreSubmitInfo* signalSemaphoreInfo, const VkSemaphoreSubmitInfo* waitSemaphoreInfo);
    VkPresentInfoKHR PresentInfo();

    VkRenderingAttachmentInfo AttachmentInfo(VkImageView view, const VkClearValue* clear, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

    VkRenderingAttachmentInfo DepthAttachmentInfo(VkImageView view, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

    VkRenderingInfo RenderingInfo(VkExtent2D renderExtent, const VkRenderingAttachmentInfo* colorAttachment, const VkRenderingAttachmentInfo* depthAttachment);

    VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);

    VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
    VkDescriptorSetLayoutBinding DescriptorsetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
    VkDescriptorSetLayoutCreateInfo DescriptorsetLayoutCreateInfo(const VkDescriptorSetLayoutBinding* bindings, uint32_t bindingCount);
    VkWriteDescriptorSet WriteDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, const VkDescriptorImageInfo* imageInfo, uint32_t binding);
    VkWriteDescriptorSet WriteDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet, const VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
    VkDescriptorBufferInfo BufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

    VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
    VkImageViewCreateInfo ImageviewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo();
    VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry = "main");
} // namespace vkinit
