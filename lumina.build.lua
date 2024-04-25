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
        --"%{prj.name}/source/**.hpp",
        --"%{prj.name}/source/**.cpp",
        "%{prj.name}/source/public/**.hpp",
        "%{prj.name}/source/private/**.hpp",
        "%{prj.name}/source/private/**.cpp",
    }

    includedirs {
        "%{prj.name}/source/public/",
        "$(VULKAN_SDK)/include/",
        "%{prj.name}/thirdparty/*/include",
    }

    libdirs {
        "$(VULKAN_SDK)/Lib/",
        "%{prj.name}/thirdparty/SDL/lib",
    }

    links {
        "vulkan-1.lib",
    }

    filter "configurations:Debug"
        defines { "_DEBUG" }
        runtime "Debug"
        symbols "On"

        links {
            "SDL2d.lib",
        }

    filter "configurations:Release"
        defines { "_RELEASE" }
        runtime "Release"
        optimize "On"

        links {
            "SDL2.lib",
        }
