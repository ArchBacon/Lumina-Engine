workspace "Lumina"
    architecture "x86_64"
    configurations { "Debug", "Release" }
    startproject "Engine"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Engine"
    location "engine"
    kind "ConsoleApp"
    language "C++"
    staticruntime "on"
    cppdialect "C++17"

    warnings "High"
    targetdir ("binaries/" .. outputdir .. "/%{prj.name}")
    objdir ("intermediate/" .. outputdir .. "/%{prj.name}")

    files {
        "%{prj.name}/source/public/**.hpp",
        "%{prj.name}/source/private/**.hpp",
        "%{prj.name}/source/private/**.cpp",
        -- I don't want this included in the engine since it is an external file
        -- Consider generating another project to include in this solution
        "%{prj.name}/thirdparty/vkbootstrap/VkBootstrap.cpp",
        "%{prj.name}/thirdparty/imgui/include/**.h",
        "%{prj.name}/thirdparty/imgui/src/**.cpp",
    }

    includedirs {
        "%{prj.name}/source/public/",
        "$(VULKAN_SDK)/include/",
        "%{prj.name}/thirdparty/", -- Include third party libs that are not in an `include` directory
        "%{prj.name}/thirdparty/*/include",
    }

    libdirs {
        "$(VULKAN_SDK)/Lib/",
        "%{prj.name}/thirdparty/SDL/lib",
        "%{prj.name}/thirdparty/fastgltf/lib",
    }

    links {
        "vulkan-1.lib",
    }

    filter "configurations:Debug"
        defines { "_DEBUG", "GLM_FORCE_DEPTH_ZERO_TO_ONE" }
        runtime "Debug"
        symbols "On"

        links {
            "SDL2d.lib",
            "fastgltf_debug.lib",
        }

    filter "configurations:Release"
        defines { "_RELEASE", "GLM_FORCE_DEPTH_ZERO_TO_ONE" }
        runtime "Release"
        optimize "On"

        links {
            "SDL2.lib",
            "fastgltf_release.lib",
        }
