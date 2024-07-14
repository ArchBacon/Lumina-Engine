#pragma once

#include "vk_types.hpp"

namespace vkutil
{
    bool LoadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
};
