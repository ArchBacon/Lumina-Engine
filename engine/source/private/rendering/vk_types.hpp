// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.
#pragma once

#include <array>
#include <core/log.hpp>
#include <deque>
#include <functional>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#define VK_CHECK(x)                                                               \
    do                                                                            \
    {                                                                             \
        VkResult err = x;                                                         \
        if (err)                                                                  \
        {                                                                         \
            lumina::Log::Info("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                              \
        }                                                                         \
    } while (0)
