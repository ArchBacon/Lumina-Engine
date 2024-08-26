#include "vk_renderer.hpp"

#include "../rendering/vk_images.hpp"
#include "../rendering/vk_initializers.hpp"
#include "../rendering/vk_types.hpp"
#include "../rendering/vk_buffer_utils.hpp"

#include <SDL/SDL.h>
#include <SDL/SDL_vulkan.h>

#include "imgui/include/imgui.h"
#include "imgui/include/imgui_impl_sdl2.h"
#include "imgui/include/imgui_impl_vulkan.h"

#define VMA_IMPLEMENTATION
#include "imgui_internal.h"
#include "vk_pipelines.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <vma/vk_mem_alloc.h>

#ifdef _DEBUG
constexpr bool USE_VALIDATION_LAYERS = true;
#else
constexpr bool USE_VALIDATION_LAYERS = false;
#endif


namespace lumina
{
    bool IsVisible(const RenderObject& object, const glm::mat4& viewProjection)
    {
        std::array<glm::vec3, 8> corners {
            glm::vec3 { 1, 1, 1 },
            glm::vec3 { 1, 1, -1 },
            glm::vec3 { 1, -1, 1 },
            glm::vec3 { 1, -1, -1 },
            glm::vec3 { -1, 1, 1 },
            glm::vec3 { -1, 1, -1 },
            glm::vec3 { -1, -1, 1 },
            glm::vec3 { -1, -1, -1 },
        };

        glm::mat4 matrix = viewProjection * object.transform;

        auto min = float3{1.5f, 1.5f, 1.5f};
        auto max = float3{-1.5f, -1.5f, -1.5f};

        for (int c = 0; c < 8; c++)
        {
            float4 v = matrix * float4(object.bounds.origin + (corners[c] * object.bounds.extents), 1.0f);

            v.x = v.x / v.w;
            v.y = v.y / v.w;
            v.z = v.z / v.w;

            min = glm::min(float3 { v.x, v.y, v.z }, min);
            max = glm::max(float3 { v.x, v.y, v.z }, max);
        }
        if (min.z > 1.0f || max.z < 0.0f || min.x > 1.0f || max.x < -1.0f || min.y > 1.0f || max.y < -1.0f)
        {
            return false;            
        }
        else
        {
            return true;
        }
    }
    
    VulkanRenderer::VulkanRenderer()
    {
        Initialize();
    }

    VulkanRenderer::~VulkanRenderer()
    {
        Shutdown();
    }

    void VulkanRenderer::Initialize()
    {
        SDL_Init(SDL_INIT_VIDEO);

        auto windowFlags = static_cast<SDL_WindowFlags>(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
        window = SDL_CreateWindow("Lumina Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowExtent.width, windowExtent.height, windowFlags);
        SDL_DisplayMode current;
        if (SDL_GetCurrentDisplayMode(0, &current) != 0)
        {
            Log::Error("SDL_GetCurrentDisplayMode failed: {}", SDL_GetError());
        }

        maxMonitorExtent.width = current.w;
        maxMonitorExtent.height = current.h;        
        
        InitVulkan();
        InitSwapchain();
        InitCommands();
        InitSyncStructures();

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;

        vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerNearest);

        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerLinear);

        mainDeletionQueue.PushFunction([&]() {
            vkDestroySampler(device, defaultSamplerNearest, nullptr);
            vkDestroySampler(device, defaultSamplerLinear, nullptr);
        });        
        
        InitDescriptors();
        InitPipelines();
        InitImGUI();        
        InitDefaultData();
    }
    void VulkanRenderer::Run()
    {
        
    }

    void VulkanRenderer::InitVulkan()
    {
        vkb::InstanceBuilder builder {};

        auto resultInstance = builder.set_app_name("Lumina Engine")
        .request_validation_layers(USE_VALIDATION_LAYERS)
        .use_default_debug_messenger()
        .require_api_version(1, 3,0)
        .build();

        vkb::Instance vkbInstance = resultInstance.value();
        instance = vkbInstance.instance;
        debugMessenger = vkbInstance.debug_messenger;

        SDL_Vulkan_CreateSurface(window, instance, &surface);

        VkPhysicalDeviceVulkan13Features features13 {};
        features13.dynamicRendering = true;
        features13.synchronization2 = true;

        VkPhysicalDeviceVulkan12Features features12 {};
        features12.bufferDeviceAddress = true;
        features12.descriptorIndexing = true;

        vkb::PhysicalDeviceSelector selector {vkbInstance};
        vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1,3).set_required_features_13(features13).set_required_features_12(features12).set_surface(surface).select().value();

        vkb::DeviceBuilder deviceBuilder {physicalDevice};
        vkb::Device vkbDevice = deviceBuilder.build().value();

        device = vkbDevice.device;
        chosenGPU = physicalDevice.physical_device;

        graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        VmaAllocatorCreateInfo allocatorInfo {};
        allocatorInfo.physicalDevice = chosenGPU;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vmaCreateAllocator(&allocatorInfo, &allocator);

        mainDeletionQueue.PushFunction([&]() {
            vmaDestroyAllocator(allocator);
        });
    }

    void VulkanRenderer::Draw()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        /*if (ImGui::Begin("Background"))
        {
            ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];
            ImGui::Text("Effect: %s", effect.name);
            
            ImGui::ColorEdit4("Top Color", reinterpret_cast<float*>(&effect.data.data1));
            ImGui::ColorEdit4("Bottom Color", reinterpret_cast<float*>(&effect.data.data2));
        }
        ImGui::End();*/

        //Enable Docking Space
        //ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);       
        ImGui::DockSpaceOverViewport();
        
        ImGui::Begin("Render Stats");
        ImGui::Text("FPS: %f", 1000.0f / stats.frameTime);
        ImGui::Text("Frame Time : %f ms", stats.frameTime);
        ImGui::Text("Draw Time:  %f ms", stats.drawTime);
        ImGui::Text("Scene Update Time: %f ms", stats.sceneUpdateTime);
        ImGui::Text("Triangles: %i", stats.triangleCount);
        ImGui::Text("Draw Calls: %i", stats.drawCallCount);
        ImGui::Checkbox("Opaque Sorting", &enableOpaqueSorting);
        if(ImGui::Checkbox("CPU Frustum Culling", &enableCPUFrustumCulling))
        {
            enableOpaqueSorting = true;
        }
        ImGui::End();

        ImGui::Begin("Vulkan Renderer");     

        ImVec2 windowSize = ImGui::GetContentRegionAvail();
        float aspectRatio = static_cast<float>(drawImage.imageExtent.width) / static_cast<float>(drawImage.imageExtent.height);
 
        ImVec2 imageSize;
        if (windowSize.x / aspectRatio <= windowSize.y) {
            imageSize.x = windowSize.x;
            imageSize.y = windowSize.x / aspectRatio;
        } else {
            imageSize.x = windowSize.y * aspectRatio;
            imageSize.y = windowSize.y;
        }
        ImGui::Image(imguiImageDescriptor, imageSize);        
        ImGui::End();
        
        ImGui::ShowDemoWindow();
        ImGui::Render();

        UpdateScene();
        
        constexpr uint32_t singleSecond = 1000000000;

        VK_CHECK(vkWaitForFences(device, 1, &GetCurrentFrame().renderFence, true, singleSecond));
        
        GetCurrentFrame().deletionQueue.Flush();
        GetCurrentFrame().frameDescriptors->ClearPools(device);
        
        uint32_t swapchainImageIndex {};
        VkResult result = vkAcquireNextImageKHR(device, swapchain, singleSecond, GetCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            resized = true;
            return;
        }
        
        VK_CHECK(vkResetFences(device, 1, &GetCurrentFrame().renderFence));

        const VkCommandBuffer command = GetCurrentFrame().commandBuffer;

        VK_CHECK(vkResetCommandBuffer(command, 0));

        const VkCommandBufferBeginInfo commandBeginInfo = vkinit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        drawExtent.width = std::min(swapchainExtent.width, drawImage.imageExtent.width) * 1.0f;
        drawExtent.height = std::min(swapchainExtent.height, drawImage.imageExtent.height) * 1.0f;
        
        VK_CHECK(vkBeginCommandBuffer(command, &commandBeginInfo));

        vkutil::TransitionImage(command, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        vkutil::TransitionImage(command, depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        DrawBackground(command);

        vkutil::TransitionImage(command, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        
        DrawGeometry(command);

        vkutil::TransitionImage(command, drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vkutil::TransitionImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vkutil::CopyImageToImage(command, drawImage.image, swapchainImages[swapchainImageIndex], drawExtent, swapchainExtent);

        vkutil::TransitionImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        //Transition Image for ImGUI Window Draw.
        vkutil::TransitionImage(command, drawImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
        vkutil::TransitionImage(command, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkutil::TransitionImage(command, imguiImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        DrawImGui(command, swapchainImageViews[swapchainImageIndex]);
        
        vkutil::TransitionImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        
        VK_CHECK(vkEndCommandBuffer(command));

        const VkCommandBufferSubmitInfo commandInfo = vkinit::CommandBufferSubmitInfo(command);

        const VkSemaphoreSubmitInfo waitInfo = vkinit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame().swapchainSemaphore);
        const VkSemaphoreSubmitInfo signalInfo = vkinit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame().renderSemaphore);

        const VkSubmitInfo2 submit = vkinit::SubmitInfo(&commandInfo, &signalInfo, &waitInfo);

        VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, GetCurrentFrame().renderFence));
        
        VkPresentInfoKHR presentInfo {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.swapchainCount = 1;

        presentInfo.pWaitSemaphores = &GetCurrentFrame().renderSemaphore;
        presentInfo.waitSemaphoreCount = 1;

        presentInfo.pImageIndices = &swapchainImageIndex;

        VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            resized = true;
        }

        ++frameNumber;
    }
    void VulkanRenderer::DrawBackground(VkCommandBuffer command)
    {
        ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];
        
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipelineLayout, 0, 1, &drawImageDescriptor, 0, nullptr);
    
        vkCmdPushConstants(command, gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
        
        vkCmdDispatch(command, static_cast<uint32_t>(std::ceil(static_cast<float>(drawExtent.width) / 16.0f)),
            static_cast<uint32_t>(std::ceil(static_cast<float>(drawExtent.height) / 16.0f)), 1);
    }
    void VulkanRenderer::DrawGeometry(VkCommandBuffer command)
    {
        stats.drawCallCount = 0;
        stats.triangleCount = 0;

        auto start = std::chrono::system_clock::now();
        std::vector<uint32_t> opaqueDraws;
        if (enableOpaqueSorting)
        {            
            opaqueDraws.reserve(mainDrawContext.opaqueSurfaces.size());

            for (uint32_t i = 0; i < mainDrawContext.opaqueSurfaces.size(); i++)
            {
                if (enableCPUFrustumCulling)
                {
                    if (IsVisible(mainDrawContext.opaqueSurfaces[i], sceneData.viewProj))
                    {
                        opaqueDraws.push_back(i);
                    }
                }               
                else
                {
                    opaqueDraws.push_back(i); 
                }
            }

            std::sort(opaqueDraws.begin(), opaqueDraws.end(), [&](const auto& iA, const auto& iB) {
                const RenderObject& a = mainDrawContext.opaqueSurfaces[iA];
                const RenderObject& b = mainDrawContext.opaqueSurfaces[iB];
                if (a.material == b.material)
                {
                    return a.indexBuffer < b.indexBuffer;
                }
                else
                {
                    return a.material < b.material;
                }
            });
        }        
        VkRenderingAttachmentInfo colorAttachment = vkinit::AttachmentInfo(drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
        VkRenderingAttachmentInfo depthAttachment = vkinit::DepthAttachmentInfo(depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);        
        VkRenderingInfo renderInfo = vkinit::RenderingInfo(drawExtent, &colorAttachment, &depthAttachment);
        
        vkCmdBeginRendering(command, &renderInfo);       

        AllocatedBuffer gpuSceneDataBuffer = CreateBuffer(allocator, sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        GetCurrentFrame().deletionQueue.PushFunction([gpuSceneDataBuffer, this]() {
            DestroyBuffer(allocator, gpuSceneDataBuffer);
        });

        auto* sceneDataUniform = static_cast<GPUSceneData*>(gpuSceneDataBuffer.allocation->GetMappedData());
        *sceneDataUniform = sceneData;
        
        VkDescriptorSet globalDescriptor = GetCurrentFrame().frameDescriptors->Allocate(device, gpuSceneDataDescriptorLayout);

        DescriptorWriter writer{};
        writer.WriteBuffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.UpdateSet(device, globalDescriptor);

        MaterialInstance* lastMaterial = nullptr;
        MaterialPipeline* lastPipeline = nullptr;
        VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
        
        auto draw = [&](const RenderObject& draw)
        {
            if (draw.material != lastMaterial)
            {
                lastMaterial = draw.material;
                if (draw.material->pipeline != lastPipeline)
                {
                    lastPipeline = draw.material->pipeline;
                    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->pipeline);
                    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->pipelineLayout, 0, 1, &globalDescriptor, 0, nullptr);

                    VkViewport viewport {};
                    viewport.x = 0;
                    viewport.y = 0;
                    viewport.width = static_cast<float>(drawExtent.width);
                    viewport.height = static_cast<float>(drawExtent.height);
                    viewport.minDepth = 0.0f;
                    viewport.maxDepth = 1.0f;

                    vkCmdSetViewport(command, 0, 1, &viewport);

                    VkRect2D scissor {};
                    scissor.offset = {0, 0};
                    scissor.extent = drawExtent;

                    vkCmdSetScissor(command, 0, 1, &scissor);
                }
                vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->pipelineLayout, 1, 1, &draw.material->materialSet, 0, nullptr);
            }
            if (draw.indexBuffer != lastIndexBuffer)
            {
                lastIndexBuffer = draw.indexBuffer;
                vkCmdBindIndexBuffer(command, draw.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            }            

            GPUDrawPushConstants pushConstants;
            pushConstants.vertexBufferDeviceAddress = draw.vertexBufferDeviceAddress;
            pushConstants.worldMatrix = draw.transform;
            vkCmdPushConstants(command, draw.material->pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &pushConstants);
            vkCmdDrawIndexed(command, draw.indexCount, 1, draw.firstIndex, 0, 0);

            stats.drawCallCount++;
            stats.triangleCount += draw.indexCount / 3;
        };

        if (enableOpaqueSorting)
        {
            for (auto& r : opaqueDraws)
            {
                draw(mainDrawContext.opaqueSurfaces[r]);
            }
        }
        else
        {
            for (auto& r : mainDrawContext.opaqueSurfaces)
            {
                draw(r);
            
            }
        }        
        
        for (auto& r : mainDrawContext.transparentSurfaces)
        {
            draw(r);
        }

        mainDrawContext.opaqueSurfaces.clear();
        mainDrawContext.transparentSurfaces.clear();
        
        vkCmdEndRendering(command);

        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        stats.drawTime = elapsed.count() / 1000.0f;
    }
    void VulkanRenderer::DrawImGui(VkCommandBuffer command, VkImageView targetImageView)
    {
        VkRenderingAttachmentInfo colorAttachment = vkinit::AttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
        VkRenderingInfo renderInfo = vkinit::RenderingInfo(swapchainExtent, &colorAttachment, nullptr);

        vkCmdBeginRendering(command, &renderInfo);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command);

        vkCmdEndRendering(command);
    }

    void VulkanRenderer::Shutdown()
    {
        vkDeviceWaitIdle(device);

        loadedScenes.clear();

        for (auto& frame : frames)
        {
            vkDestroyCommandPool(device, frame.commandPool, nullptr);

            // Destroy sync objects
            vkDestroyFence(device, frame.renderFence, nullptr);
            vkDestroySemaphore(device, frame.renderSemaphore, nullptr);
            vkDestroySemaphore(device, frame.swapchainSemaphore, nullptr);

            frame.deletionQueue.Flush();
        }

        metallicRoughnessMaterial.ClearResources(device);
        
        mainDeletionQueue.Flush();
        
        DestroySwapchain();

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);

        vkb::destroy_debug_utils_messenger(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
        SDL_DestroyWindow(window);            
    }
    void VulkanRenderer::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
    {
        VK_CHECK(vkResetFences(device, 1, &immediateFence));
        VK_CHECK(vkResetCommandBuffer(immediateCommandBuffer, 0));

        const VkCommandBuffer command = immediateCommandBuffer;

        const VkCommandBufferBeginInfo beginInfo = vkinit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VK_CHECK(vkBeginCommandBuffer(command, &beginInfo));

        function(command);

        VK_CHECK(vkEndCommandBuffer(command));

        const VkCommandBufferSubmitInfo submitInfo = vkinit::CommandBufferSubmitInfo(command);
        const VkSubmitInfo2 submit = vkinit::SubmitInfo(&submitInfo, nullptr, nullptr);

        VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, immediateFence));

        VK_CHECK(vkWaitForFences(device, 1, &immediateFence, true, 1000000000));
    }

    void VulkanRenderer::InitSwapchain()
    {
        CreateSwapchain(windowExtent.width, windowExtent.height);
        
        VkExtent3D drawImageExtent = {maxMonitorExtent.width, maxMonitorExtent.height, 1};

        drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        drawImage.imageExtent = drawImageExtent;

        VkImageUsageFlags drawImageUsages{};
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;


        VkImageCreateInfo rimgInfo = vkinit::ImageCreateInfo(drawImage.imageFormat, drawImageUsages, drawImage.imageExtent);

        VmaAllocationCreateInfo rimgAllocationInfo {};
        rimgAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        rimgAllocationInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vmaCreateImage(allocator, &rimgInfo, &rimgAllocationInfo, &drawImage.image, &drawImage.allocation, nullptr);

        VkImageViewCreateInfo viewInfo = vkinit::ImageviewCreateInfo(drawImage.imageFormat, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

        VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &drawImage.imageView));

        depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
        depthImage.imageExtent = drawImageExtent;
        VkImageUsageFlags depthImageUsages{};
        depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        VkImageCreateInfo dimgInfo = vkinit::ImageCreateInfo(depthImage.imageFormat, depthImageUsages, depthImage.imageExtent);

        vmaCreateImage(allocator, &dimgInfo, &rimgAllocationInfo, &depthImage.image, &depthImage.allocation, nullptr);

        VkImageViewCreateInfo dviewInfo = vkinit::ImageviewCreateInfo(depthImage.imageFormat, depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

        VK_CHECK(vkCreateImageView(device, &dviewInfo, nullptr, &depthImage.imageView));

        //Create ImGui Image
        imguiImage.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        imguiImage.imageExtent = drawImageExtent;

        VkImageUsageFlags imguiImageUsages{};
        imguiImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imguiImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        imguiImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;

        VkImageCreateInfo imguiInfo = vkinit::ImageCreateInfo(imguiImage.imageFormat, imguiImageUsages, imguiImage.imageExtent);
        vmaCreateImage(allocator, &imguiInfo, &rimgAllocationInfo, &imguiImage.image, &imguiImage.allocation, nullptr);
        VkImageViewCreateInfo imguiViewInfo = vkinit::ImageviewCreateInfo(imguiImage.imageFormat, imguiImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
        VK_CHECK(vkCreateImageView(device, &imguiViewInfo, nullptr, &imguiImage.imageView));
        
        mainDeletionQueue.PushFunction([&]() {
            vkDestroyImageView(device, drawImage.imageView, nullptr);
            vmaDestroyImage(allocator, drawImage.image, drawImage.allocation);

            vkDestroyImageView(device, depthImage.imageView, nullptr);
            vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);

            vkDestroyImageView(device, imguiImage.imageView, nullptr);
            vmaDestroyImage(allocator, imguiImage.image, imguiImage.allocation);
        });
    }

    void VulkanRenderer::InitCommands()
    {
        VkCommandPoolCreateInfo commandPoolInfo = vkinit::CommandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].commandPool));
            VkCommandBufferAllocateInfo commandAllocateInfo = vkinit::CommandBufferAllocateInfo(frames[i].commandPool, 1);
            VK_CHECK(vkAllocateCommandBuffers(device, &commandAllocateInfo, &frames[i].commandBuffer));
        }

        VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &immediateCommandPool));
        VkCommandBufferAllocateInfo commandAllocateInfo = vkinit::CommandBufferAllocateInfo(immediateCommandPool, 1);
        VK_CHECK(vkAllocateCommandBuffers(device, &commandAllocateInfo, &immediateCommandBuffer));

        mainDeletionQueue.PushFunction([&]() {
            vkDestroyCommandPool(device, immediateCommandPool, nullptr);
        });
    }

    void VulkanRenderer::InitSyncStructures()
    {
        VkFenceCreateInfo fenceCreateInfo = vkinit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::SemaphoreCreateInfo();

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));
            
            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].swapchainSemaphore));
            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));
        }

        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &immediateFence));
        mainDeletionQueue.PushFunction([&](){ vkDestroyFence(device, immediateFence, nullptr); });
    }
    void VulkanRenderer::InitDescriptors()
    {
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes =
            {
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
               { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
            };

        globalDescriptorAllocator.InitializePool(device, 10, sizes);

        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            drawImageDescriptorLayout = builder.Build(device, VK_SHADER_STAGE_COMPUTE_BIT);
        }
        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            gpuSceneDataDescriptorLayout = builder.Build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }
        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            imguiImageDescriptorLayout = builder.Build(device, VK_SHADER_STAGE_FRAGMENT_BIT);
        }
        drawImageDescriptor = globalDescriptorAllocator.Allocate(device, drawImageDescriptorLayout);
        imguiImageDescriptor = globalDescriptorAllocator.Allocate(device, imguiImageDescriptorLayout);
        
        DescriptorWriter writer{};
        writer.WriteImage(0, drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.UpdateSet(device, drawImageDescriptor);

        writer = {};
        writer.WriteImage(0, drawImage.imageView, defaultSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.UpdateSet(device, imguiImageDescriptor);

        mainDeletionQueue.PushFunction([&]() {
            globalDescriptorAllocator.DestroyPool(device);
            vkDestroyDescriptorSetLayout(device, drawImageDescriptorLayout, nullptr);
            vkDestroyDescriptorSetLayout(device, gpuSceneDataDescriptorLayout, nullptr);
            vkDestroyDescriptorSetLayout(device, imguiImageDescriptorLayout, nullptr);
        });

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frameSizes =
                {
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
                };

            frames[i].frameDescriptors = std::make_unique<DescriptorAllocatorGrowable>();
            frames[i].frameDescriptors->InitializePool(device, 1000, frameSizes);

            mainDeletionQueue.PushFunction([&, i]() {
                frames[i].frameDescriptors->DestroyPool(device);
            });
            
        }
    }
    void VulkanRenderer::InitPipelines()
    {
        InitBackgroundPipelines();
        metallicRoughnessMaterial.BuildPipelines(this);
    }
    void VulkanRenderer::InitBackgroundPipelines()
    {
        VkPipelineLayoutCreateInfo computeLayout {};
        computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        computeLayout.pNext = nullptr;
        computeLayout.setLayoutCount = 1;
        computeLayout.pSetLayouts = &drawImageDescriptorLayout;

        VkPushConstantRange pushConstant {};
        pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstant.offset = 0;
        pushConstant.size = sizeof(ComputePushConstants);

        computeLayout.pPushConstantRanges = &pushConstant;
        computeLayout.pushConstantRangeCount = 1;
        
        VK_CHECK(vkCreatePipelineLayout(device, &computeLayout, nullptr, &gradientPipelineLayout));

        VkShaderModule gradientShader;
        if (!vkutil::LoadShaderModule("assets/shaders/gradient_color.comp.spv", device, &gradientShader))
        {
            Log::Error("Error when building Background Compute Shader\n");    
        }

        VkPipelineShaderStageCreateInfo stageInfo {};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.pNext = nullptr;
        stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stageInfo.module = gradientShader;
        stageInfo.pName = "main";

        VkComputePipelineCreateInfo pipelineInfo {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = stageInfo;
        pipelineInfo.layout = gradientPipelineLayout;
        pipelineInfo.pNext = nullptr;

        ComputeEffect gradientEffect {};
        gradientEffect.pipelineLayout = gradientPipelineLayout;
        gradientEffect.name = "Gradient";
        gradientEffect.data = {};
        
        gradientEffect.data.data1 = float4(1.0, 0.0, 0.0, 1.0);
        gradientEffect.data.data2 = float4(0.0, 0.0, 1.0, 1.0);
        
        VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &gradientEffect.pipeline));

        backgroundEffects.push_back(gradientEffect);
        
        vkDestroyShaderModule(device, gradientShader, nullptr);

        mainDeletionQueue.PushFunction([&]() {
            vkDestroyPipelineLayout(device, gradientPipelineLayout, nullptr);
            for (auto& effect : backgroundEffects)
            {
                vkDestroyPipeline(device, effect.pipeline, nullptr);
            }
        });
    }    
    
    void VulkanRenderer::InitImGUI()
    {
        VkDescriptorPoolSize poolSize[] =
            { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }};

        VkDescriptorPoolCreateInfo poolInfo {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000;
        poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSize));
        poolInfo.pPoolSizes = poolSize;
        
        VkDescriptorPool imGUIPool;
        VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &imGUIPool));

        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForVulkan(window);

        ImGui_ImplVulkan_InitInfo initInfo {};
        initInfo.Instance = instance;
        initInfo.PhysicalDevice = chosenGPU;
        initInfo.Device = device;
        initInfo.Queue = graphicsQueue;
        initInfo.QueueFamily = graphicsQueueFamily;
        initInfo.DescriptorPool = imGUIPool;
        initInfo.MinImageCount = 3;
        initInfo.ImageCount = 3;
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.UseDynamicRendering = true;
        initInfo.Allocator = nullptr;
        initInfo.CheckVkResultFn = nullptr;
        
        initInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainImageFormat;

        ImGui_ImplVulkan_Init(&initInfo);
        ImGui_ImplVulkan_CreateFontsTexture();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        mainDeletionQueue.PushFunction([this, imGUIPool]() {
            ImGui_ImplVulkan_Shutdown();
            vkDestroyDescriptorPool(device, imGUIPool, nullptr);
        });
    }
    void VulkanRenderer::InitDefaultData()
    {
        mainCamera.velocity = float3{0.0f};
        mainCamera.position = float3{0.0f, 0.0f, 5.0f};
        mainCamera.pitch = 0.0f;
        mainCamera.yaw = 0.0f;        
        
        std::array<Vertex, 4> rectangleVertices;

        rectangleVertices[0].position = {0.5f, -0.5f, 0.0f};
        rectangleVertices[1].position = {0.5f, 0.5f, 0.0f};
        rectangleVertices[2].position = {-0.5f, -0.5f, 0.0f};
        rectangleVertices[3].position = {-0.5f, 0.5f, 0.0f};

        rectangleVertices[0].color = { 0.0f, 0.0f, 0.0f, 1.0f};
        rectangleVertices[1].color = { 0.5f, 0.5f, 0.5f, 1.0f};
        rectangleVertices[2].color = { 1.0f, 0.0f, 0.0f, 1.0f};
        rectangleVertices[3].color = { 0.0f, 1.0f, 0.0f, 1.0f};

        std::array<uint32_t, 6> rectangleIndices = {0, 1, 2, 2, 1, 3};        

        uint32_t white = glm::packUnorm4x8(float4(1, 1, 1, 1));
        whiteImage = CreateImage(&white, VkExtent3D{1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        uint32_t gray = glm::packUnorm4x8(float4(0.66f, 0.66f, 0.66f, 1));
        greyImage = CreateImage(&gray, VkExtent3D{1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        uint32_t black = glm::packUnorm4x8(float4(0, 0, 0, 0));
        blackImage = CreateImage(&black, VkExtent3D{1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        uint32_t magenta = glm::packUnorm4x8(float4(1, 0, 1, 1));
        std::array<uint32_t, 16 * 16> pixels;
        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
            }
        }

        errorCheckerboardImage = CreateImage(pixels.data(), VkExtent3D{16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        /*VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;

        vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerNearest);

        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerLinear);*/

        mainDeletionQueue.PushFunction([&]() {
            //vkDestroySampler(device, defaultSamplerNearest, nullptr);
            //vkDestroySampler(device, defaultSamplerLinear, nullptr);

            DestroyImage(whiteImage);
            DestroyImage(greyImage);
            DestroyImage(blackImage);
            DestroyImage(errorCheckerboardImage);
        });

        GLTFMetallicRoughness::MaterialResources materialResources;
        materialResources.colorImage = whiteImage;
        materialResources.colorSampler = defaultSamplerLinear;
        materialResources.metallicRoughnessImage = whiteImage;
        materialResources.metallicRoughnessSampler = defaultSamplerLinear;

        AllocatedBuffer materialConstants = CreateBuffer(allocator,sizeof(GLTFMetallicRoughness::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        auto sceneUniformData = static_cast<GLTFMetallicRoughness::MaterialConstants*>(materialConstants.allocation->GetMappedData());
        sceneUniformData->colorFactors = float4{1.0f, 1.0f, 1.0f, 1.0f};
        sceneUniformData->metallicRoughnessFactors = float4{1.0f, 0.5f, 0.0f, 0.0f};

        mainDeletionQueue.PushFunction([this, materialConstants]() {
            DestroyBuffer(allocator, materialConstants);
        });

        materialResources.dataBuffer = materialConstants.buffer;
        materialResources.dataBufferOffset = 0;

        defaultData.data = metallicRoughnessMaterial.WriteMaterial(device, MaterialPass::MainColor, materialResources, globalDescriptorAllocator);
        
        std::string structure = {"assets/models/structure_mat.glb"};
        auto structureFile = LoadGLTF(this, structure);
        assert(structureFile.has_value());
        loadedScenes["structure"] = *structureFile;
    }

    void VulkanRenderer::CreateSwapchain(uint32_t width, uint32_t height)
    {
        vkb::SwapchainBuilder swapchainBuilder {chosenGPU, device, surface};
        swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        vkb::Swapchain vkbSwapchain = swapchainBuilder.set_desired_format(VkSurfaceFormatKHR {swapchainImageFormat, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();
        swapchainExtent = vkbSwapchain.extent;
        swapchain = vkbSwapchain.swapchain;
        swapchainImages = vkbSwapchain.get_images().value();
        swapchainImageViews = vkbSwapchain.get_image_views().value();
    }
    void VulkanRenderer::ResizeSwapchain()
    {
        vkDeviceWaitIdle(device);
        DestroySwapchain();

        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        windowExtent.width = width;
        windowExtent.height = height;
        
        CreateSwapchain(windowExtent.width, windowExtent.height);
        RebuildDrawImage(windowExtent);
        resized = false;
    }

    void VulkanRenderer::DestroySwapchain()
    {
        vkDestroySwapchainKHR(device, swapchain, nullptr);

        for (const auto& imageView : swapchainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }
    }
    AllocatedImage VulkanRenderer::CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
    {
        AllocatedImage newImage;
        newImage.imageFormat = format;
        newImage.imageExtent = size;

        VkImageCreateInfo imageInfo = vkinit::ImageCreateInfo(format, usage, size);
        if (mipmapped)
        {
            imageInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
        }

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vmaCreateImage(allocator, &imageInfo, &allocInfo, &newImage.image, &newImage.allocation, nullptr));

        VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
        if (format == VK_FORMAT_D32_SFLOAT)
        {
            aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        VkImageViewCreateInfo viewInfo = vkinit::ImageviewCreateInfo(format, newImage.image, aspectFlag);
        viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;

        VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &newImage.imageView));

        return newImage;
    }
    AllocatedImage VulkanRenderer::CreateImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
    {
        size_t dataSize = size.width * size.height * size.depth * 4;

        AllocatedBuffer stagingBuffer = CreateBuffer(allocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        memcpy(stagingBuffer.allocationInfo.pMappedData, data, dataSize);

        AllocatedImage newImage = CreateImage(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, mipmapped);

        ImmediateSubmit([&](VkCommandBuffer command) {
            vkutil::TransitionImage(command, newImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBufferImageCopy copyRegion {};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;

            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.imageExtent = size;

            vkCmdCopyBufferToImage(command, stagingBuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

            if (mipmapped)
            {
                vkutil::GenerateMipMaps(command, newImage.image, VkExtent2D{newImage.imageExtent.width, newImage.imageExtent.height});
            }
            else
            {
                vkutil::TransitionImage(command, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }            
        });

        DestroyBuffer(allocator, stagingBuffer);

        return newImage;        
    }
    void VulkanRenderer::DestroyImage(const AllocatedImage& image)
    {
        vkDestroyImageView(device, image.imageView, nullptr);
        vmaDestroyImage(allocator, image.image, image.allocation);
    }
    void VulkanRenderer::RebuildDrawImage(VkExtent2D newExtent)
    {
        vkDestroyImageView(device, drawImage.imageView, nullptr);
        vmaDestroyImage(allocator, drawImage.image, drawImage.allocation);

        VkExtent3D drawImageExtent = {newExtent.width, newExtent.height, 1};

        drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        drawImage.imageExtent = drawImageExtent;

        VkImageUsageFlags drawImageUsages = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                            VK_IMAGE_USAGE_STORAGE_BIT |
                                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                            VK_IMAGE_USAGE_SAMPLED_BIT;

        VkImageCreateInfo rimgInfo = vkinit::ImageCreateInfo(drawImage.imageFormat, drawImageUsages, drawImage.imageExtent);

        VmaAllocationCreateInfo rimgAllocationInfo {};
        rimgAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        rimgAllocationInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vmaCreateImage(allocator, &rimgInfo, &rimgAllocationInfo, &drawImage.image, &drawImage.allocation, nullptr);

        VkImageViewCreateInfo viewInfo = vkinit::ImageviewCreateInfo(drawImage.imageFormat, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

        VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &drawImage.imageView));

        DescriptorWriter writer{};
        writer.WriteImage(0, drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.UpdateSet(device, drawImageDescriptor);

        writer = {};
        writer.WriteImage(0, drawImage.imageView, defaultSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.UpdateSet(device, imguiImageDescriptor);

    }
    GPUMeshBuffers VulkanRenderer::UploadMesh(tcb::span<uint32_t> indices, tcb::span<Vertex> vertices)
    {
        const size_t vertexBufferSize = sizeof(Vertex) * vertices.size();
        const size_t indexBufferSize = sizeof(uint32_t) * indices.size();

        GPUMeshBuffers newSurface;

        newSurface.vertexBuffer = CreateBuffer(allocator, vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        VkBufferDeviceAddressInfo addressInfo {};
        addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        addressInfo.buffer = newSurface.vertexBuffer.buffer;

        newSurface.vertexBufferDeviceAddress = vkGetBufferDeviceAddress(device, &addressInfo);

        newSurface.indexBuffer = CreateBuffer(allocator, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        const AllocatedBuffer stagingBuffer = CreateBuffer(allocator, vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        void* data = stagingBuffer.allocationInfo.pMappedData;

        memcpy(data, vertices.data(), vertexBufferSize);
        memcpy(static_cast<char*>(data) + vertexBufferSize, indices.data(), indexBufferSize);

        ImmediateSubmit([&](VkCommandBuffer command) {
            VkBufferCopy vertexCopy {};
            vertexCopy.dstOffset = 0;
            vertexCopy.srcOffset = 0;
            vertexCopy.size = vertexBufferSize;

            vkCmdCopyBuffer(command, stagingBuffer.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

            VkBufferCopy indexCopy {};
            indexCopy.dstOffset = 0;
            indexCopy.srcOffset = vertexBufferSize;
            indexCopy.size = indexBufferSize;

            vkCmdCopyBuffer(command, stagingBuffer.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
        });

        DestroyBuffer(allocator, stagingBuffer);

        return newSurface;        
    }
    void VulkanRenderer::UpdateScene()
    {
        auto start = std::chrono::system_clock::now();

        loadedScenes["structure"]->Draw(glm::mat4{1.0f}, mainDrawContext);
                
        mainCamera.Update(deltaTime);
        sceneData.view = mainCamera.GetViewMatrix();
        sceneData.proj = glm::perspective(glm::radians(70.0f), static_cast<float>(windowExtent.width) / static_cast<float>(windowExtent.height), 10000.0f, 0.1f);

        sceneData.proj[1][1] *= -1;
        sceneData.viewProj = sceneData.proj * sceneData.view;

        sceneData.ambientColor = float4(0.1f, 0.1f, 0.1f, 1.0f);
        sceneData.sunlightColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
        sceneData.sunlightDirection = float4(0.0f, 1.0f, 0.5f, 1.0f);

        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        stats.sceneUpdateTime = elapsed.count() / 1000.0f;
    }
    void GLTFMetallicRoughness::BuildPipelines(VulkanRenderer* renderer)
    {
        VkShaderModule meshVertexShader;
        if (!vkutil::LoadShaderModule("assets/shaders/mesh.vert.spv", renderer->device, &meshVertexShader))
        {
            Log::Error("Error when building Mesh Vertex Shader\n");
        }
        VkShaderModule meshFragmentShader;
        if (!vkutil::LoadShaderModule("assets/shaders/mesh.frag.spv", renderer->device, &meshFragmentShader))
        {
            Log::Error("Error when building Mesh Fragment Shader\n");
        }

        VkPushConstantRange matrixRange{};
        matrixRange.offset = 0;
        matrixRange.size = sizeof(GPUDrawPushConstants);
        matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        DescriptorLayoutBuilder layoutBuilder;
        layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        materialSetLayout = layoutBuilder.Build(renderer->device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

        VkDescriptorSetLayout setLayouts[] = {renderer->gpuSceneDataDescriptorLayout, materialSetLayout};

        VkPipelineLayoutCreateInfo meshLayoutInfo = vkinit::PipelineLayoutCreateInfo();
        meshLayoutInfo.setLayoutCount = 2;
        meshLayoutInfo.pSetLayouts = setLayouts;
        meshLayoutInfo.pushConstantRangeCount = 1;
        meshLayoutInfo.pPushConstantRanges = &matrixRange;

        VkPipelineLayout newLayout;
        VK_CHECK(vkCreatePipelineLayout(renderer->device, &meshLayoutInfo, nullptr, &newLayout));

        opaquePipeline.pipelineLayout = newLayout;
        transparentPipeline.pipelineLayout = newLayout;

        PipelineBuilder pipelineBuilder;
        pipelineBuilder.SetShaders(meshVertexShader, meshFragmentShader);
        pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipelineBuilder.SetMultisamplingNone();
        pipelineBuilder.DisableBlending();
        pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

        pipelineBuilder.SetColorAttachmentFormat(renderer->drawImage.imageFormat);
        pipelineBuilder.SetDepthFormat(renderer->depthImage.imageFormat);

        pipelineBuilder.pipelineLayout = newLayout;
        opaquePipeline.pipeline = pipelineBuilder.BuildPipeline(renderer->device);

        pipelineBuilder.EnableBlendingAdditive();
        pipelineBuilder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
        transparentPipeline.pipeline = pipelineBuilder.BuildPipeline(renderer->device);

        vkDestroyShaderModule(renderer->device, meshVertexShader, nullptr);
        vkDestroyShaderModule(renderer->device, meshFragmentShader, nullptr);
    }

    void GLTFMetallicRoughness::ClearResources(VkDevice device)
    {
        vkDestroyDescriptorSetLayout(device, materialSetLayout, nullptr);
        vkDestroyPipelineLayout(device, transparentPipeline.pipelineLayout, nullptr);        

        vkDestroyPipeline(device, transparentPipeline.pipeline, nullptr);
        vkDestroyPipeline(device, opaquePipeline.pipeline, nullptr);
    }

    MaterialInstance GLTFMetallicRoughness::WriteMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator)
    {
        MaterialInstance materialData;
        materialData.passType = pass;
        if (pass == MaterialPass::Transparent)
        {
            materialData.pipeline = &transparentPipeline;
        }
        else
        {
            materialData.pipeline = &opaquePipeline;
        }

        materialData.materialSet = descriptorAllocator.Allocate(device, materialSetLayout);

        writer.Clear();
        writer.WriteBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.WriteImage(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.WriteImage(2, resources.metallicRoughnessImage.imageView, resources.metallicRoughnessSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        writer.UpdateSet(device, materialData.materialSet);

        return materialData;
    }
    void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& context)
    {
        glm::mat4 nodeMatrix = topMatrix * worldTransform;

        for (auto& surface : mesh->surfaces)
        {
            RenderObject def;
            def.indexCount = surface.indexCount;
            def.firstIndex = surface.startIndex;
            def.indexBuffer = mesh->buffers.indexBuffer.buffer;
            def.material = &surface.material->data;
            def.bounds = surface.bounds;
            def.transform = nodeMatrix;
            def.vertexBufferDeviceAddress = mesh->buffers.vertexBufferDeviceAddress;

            if (surface.material->data.passType == MaterialPass::Transparent)
            {
                context.transparentSurfaces.push_back(def);
            }
            else
            {
                context.opaqueSurfaces.push_back(def);
            }            
        }       
        Node::Draw(topMatrix, context);            
    }
}
