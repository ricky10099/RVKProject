workspace "RVKProject"
	architecture "x64"
	
	configurations{
		"Debug",
		"Release",
	}

outputdir = "%{cfg.buildcfg}"

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["GLFW"] = "external/GLFW/include"
IncludeDir["spdlog"] = "external/spdlog/include"
IncludeDir["Assimp"] = "external/Assimp/include"

LibDir = {}
LibDir["GLFW"] = "external/GLFW/lib-vc2022"
LibDir["Assimp"] = "external/Assimp/lib"

project "RVKProject"
	location "RVKProject"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("bin/" ..outputdir.. "/%{prj.name}")
	objdir ("bin-int/" ..outputdir.. "/%{prj.name}")

	files{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}
	
	defines{
		"_CRT_SECURE_NO_WARNINGS",
	}

	includedirs {
		"%{prj.name}/src",
		"%{VULKAN_SDK}/Include",
		"external",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.Assimp}",
	}
	
	libdirs{
		"%{VULKAN_SDK}/Lib",
		"%{LibDir.GLFW}",
		"%{LibDir.Assimp}",
	}

	links{
		"vulkan-1.lib",
		"glfw3.lib",
		"ImGui",
		"assimp-vc143-mt.lib",
	}

	filter"system:windows"
		systemversion "latest"
		
		defines{
			"GLFW_INCLUDE_NONE",
		}

	filter "configurations:Debug"
		defines "VK_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "VK_RELEASE"
		runtime "Release"
		optimize "on"
		
group "external"
	include "external/ImGui.lua"