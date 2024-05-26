#pragma once

#include <vulkan/vulkan.h>

namespace vkutil
{
    void TransitionImage(VkCommandBuffer command, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
}
