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
        "%{prj.name}/source/**.hpp",
        "%{prj.name}/source/**.cpp",
    }

    includedirs {
        "%{prj.name}/source/",
        "$(VULKAN_SDK)/include/",
    }

    libdirs {
        "$(VULKAN_SDK)/Lib/",
    }

    links {
        "vulkan-1.lib",
    }

    filter "configurations:Debug"
        defines { "_DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "_RELEASE" }
        runtime "Release"
        optimize "On"
