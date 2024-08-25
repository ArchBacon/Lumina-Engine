#pragma once
#include "vk_descriptors.hpp"
#include "vk_loader.hpp"
#include "core/types.hpp"
#include <memory>
#include <vkbootstrap/VkBootstrap.h>
#include <deque>
#include <functional>
#include <vma/vk_mem_alloc.h>
#include "vk_types.hpp"
#include "camera.hpp"

struct SDL_Window;

namespace lumina
{
    class Engine;

    struct DeletionQueue
    {
        std::deque<std::function<void()>> deletors {};

        void PushFunction(std::function<void()>&& function)
        {
            deletors.push_back(function);
        }

        void Flush()
        {
            for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
            {
                (*it)();
            }
            deletors.clear();
        }
    };
    
    struct FrameData
    {
        VkCommandPool commandPool {};
        VkCommandBuffer commandBuffer {};
        VkSemaphore swapchainSemaphore {};
        VkSemaphore renderSemaphore {};
        VkFence renderFence {};
        DeletionQueue deletionQueue {};
        std::unique_ptr<DescriptorAllocatorGrowable> frameDescriptors{};
    };

    struct ComputePushConstants
    {
        float4 data1;
        float4 data2;
        float4 data3;
        float4 data4;
    };

    struct GPUSceneData
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewProj;
        float4 ambientColor;
        float4 sunlightDirection;
        float4 sunlightColor;
    };

    struct ComputeEffect
    {
        const char* name;
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;

        ComputePushConstants data;
    };

    struct GLTFMetallicRoughness
    {
        MaterialPipeline opaquePipeline;
        MaterialPipeline transparentPipeline;

        VkDescriptorSetLayout materialSetLayout;

        struct MaterialConstants
        {
            float4 colorFactors;
            float4 metallicRoughnessFactors;
            float4 padding[14];
        };

        struct MaterialResources
        {
            AllocatedImage colorImage;
            VkSampler colorSampler;
            AllocatedImage metallicRoughnessImage;
            VkSampler metallicRoughnessSampler;
            VkBuffer dataBuffer;
            uint32_t dataBufferOffset;
        };

        DescriptorWriter writer;

        void BuildPipelines(VulkanRenderer* renderer);
        void ClearResources(VkDevice device);

        MaterialInstance WriteMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
    };

    struct MeshNode : public Node
    {
        std::shared_ptr<MeshAsset> mesh;

        void Draw(const glm::mat4& topMatrix, DrawContext& context) override;
    };

    struct RendererStats
    {
        float frameTime{};
        float sceneUpdateTime{};
        float drawTime{};
        int triangleCount{};
        int drawCallCount{};
    };

    constexpr uint8_t FRAME_OVERLAP = 2;
    
    class VulkanRenderer
    {
        friend class Engine;
        
    public:

        VulkanRenderer();
        ~VulkanRenderer();

        VulkanRenderer(const VulkanRenderer&)            = delete;
        VulkanRenderer(VulkanRenderer&&)                 = delete;
        VulkanRenderer& operator=(const VulkanRenderer&) = delete;
        VulkanRenderer& operator=(VulkanRenderer&&)      = delete;
        
        VkInstance instance {};
        VkDebugUtilsMessengerEXT debugMessenger {};
        VkPhysicalDevice chosenGPU {};
        VkDevice device {};
        VkSurfaceKHR surface {};
        VkSwapchainKHR swapchain {};
        VkFormat swapchainImageFormat {};

        float deltaTime{};

        std::vector<VkImage> swapchainImages {};
        std::vector<VkImageView> swapchainImageViews {};
        VkExtent2D swapchainExtent {};

        uint32_t frameNumber {0};
        FrameData frames[FRAME_OVERLAP];
        VkQueue graphicsQueue {};
        uint32_t graphicsQueueFamily {};

        DeletionQueue mainDeletionQueue {};

        VmaAllocator allocator {};
        DescriptorAllocatorGrowable globalDescriptorAllocator {};
        VkDescriptorSet drawImageDescriptor {};
        VkDescriptorSetLayout drawImageDescriptorLayout {};
        VkDescriptorSetLayout gpuSceneDataDescriptorLayout {};
        
        VkPipeline gradientPipeline {};
        VkPipelineLayout gradientPipelineLayout {};
        
        std::vector<ComputeEffect> backgroundEffects;
        int currentBackgroundEffect {0};
        
        //Draw Resources
        AllocatedImage drawImage;
        AllocatedImage depthImage;

        //Textures
        AllocatedImage whiteImage;
        AllocatedImage blackImage;
        AllocatedImage greyImage;
        AllocatedImage errorCheckerboardImage;

        VkSampler defaultSamplerLinear;
        VkSampler defaultSamplerNearest;
        
        VkExtent2D drawExtent;
        VkExtent2D maxMonitorExtent;
        
        VkExtent2D windowExtent {1080, 720};
        SDL_Window* window {nullptr};
        bool running {true};
        bool stopRendering {false};
        bool resized {false};

        bool enableOpaqueSorting{false};
        bool enableCPUFrustumCulling{false};

        void Initialize();
        void Run();
        void Draw();
        void Shutdown();

        void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
        GPUMeshBuffers UploadMesh(tcb::span<uint32_t> indices, tcb::span<Vertex> vertices);
        void UpdateScene();

        AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);        
        AllocatedImage CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
        AllocatedImage CreateImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
        void DestroyBuffer(const AllocatedBuffer& buffer);
        void DestroyImage(const AllocatedImage& image);

        
        VkFence immediateFence {};
        VkCommandBuffer immediateCommandBuffer {};
        VkCommandPool immediateCommandPool {};

        GPUSceneData sceneData{};

        GLTFMaterial defaultData;
        GLTFMetallicRoughness metallicRoughnessMaterial;

        DrawContext mainDrawContext;
        std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;

        std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;

        Camera mainCamera;

        RendererStats stats;

    private:
        void InitVulkan();
        void InitSwapchain();
        void InitCommands();
        void InitSyncStructures();
        void InitDescriptors();
        void InitPipelines();
        void InitBackgroundPipelines();
        void InitImGUI();

        void InitDefaultData();
        
        void DrawBackground(VkCommandBuffer command);
        void DrawGeometry(VkCommandBuffer command);
        void DrawImGui(VkCommandBuffer command, VkImageView targetImageView);

        void CreateSwapchain(uint32_t width, uint32_t height);
        void ResizeSwapchain();
        void DestroySwapchain();
        
        FrameData& GetCurrentFrame()
        {
            return frames[frameNumber % FRAME_OVERLAP];
        }    
    };
}
