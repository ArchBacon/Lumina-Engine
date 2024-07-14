﻿#include "vk_pipelines.hpp"
#include <fstream>
#include "vk_initializers.hpp"

bool vkutil::LoadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule)
{
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    size_t fileSize = file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        return false;
    }
    *outShaderModule = shaderModule;
    return true;    
}
