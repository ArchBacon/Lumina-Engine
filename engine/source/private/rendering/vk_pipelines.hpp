#pragma once

#include "vk_types.hpp"

namespace lumina
{
    namespace vkutil
    {
        bool LoadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
    };

    class PipelineBuilder
    {
    public:
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages {};

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState {};
        VkPipelineRasterizationStateCreateInfo rasterizationState {};
        VkPipelineColorBlendAttachmentState colorBlendAttachment {};
        VkPipelineMultisampleStateCreateInfo multisampleState {};
        VkPipelineLayout pipelineLayout {VK_NULL_HANDLE};
        VkPipelineDepthStencilStateCreateInfo depthStencilState {};
        VkPipelineRenderingCreateInfo renderingCreateInfo {};
        VkFormat colorAttachmentFormat {VK_FORMAT_UNDEFINED};

        PipelineBuilder() { Clear(); }

        void Clear();

        VkPipeline BuildPipeline(VkDevice device);
        void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
        void SetInputTopology(VkPrimitiveTopology topology);
        void SetPolygonMode(VkPolygonMode mode);
        void SetCullMode(VkCullModeFlags mode, VkFrontFace frontFace);
        void SetMultisamplingNone();
        void DisableBlending();
        void EnableBlendingAdditive();
        void EnableBlendingAlphaBlend();
        void SetColorAttachmentFormat(VkFormat format);
        void SetDepthFormat(VkFormat format);
        void DisableDepthTest();
        void EnableDepthTest(bool depthWriteEnable, VkCompareOp compareOp);
        
        
    };
}