#pragma once

#include <vulkan/vulkan.h>

namespace lumina
{
    namespace vkutil
    {
        void TransitionImage(VkCommandBuffer command, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

        void CopyImageToImage(VkCommandBuffer command, VkImage srcImage, VkImage dstImage, VkExtent2D srcSize, VkExtent2D dstSize);
    } // namespace vkutil
}
