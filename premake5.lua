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
IncludeDir["PhysX"] = "external/PhysX/include"
IncludeDir["CRI"] = "external/CRI/include"

LibDir = {}
LibDir["GLFW"] = "external/GLFW/lib-vc2022"
LibDir["Assimp"] = "external/Assimp/lib"
LibDir["PhysX"] = "external/PhysX/lib"
LibDir["CRI"] = "external/CRI/lib"

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
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.CRI}",
	}
	
	libdirs{
		"%{VULKAN_SDK}/Lib",
		"%{LibDir.GLFW}",
		"%{LibDir.Assimp}/%{cfg.buildcfg}",
		"%{LibDir.PhysX}/%{cfg.buildcfg}",
		"%{LibDir.CRI}",
	}

	links{
		"vulkan-1.lib",
		"glfw3.lib",
		"ImGui",
		"PhysX_64.lib",
		"PhysXCommon_64.lib",
		"PhysXCooking_64.lib",
		"PhysXFoundation_64.lib",
		"PhysXPvdSDK_static_64.lib",
		"PhysXExtensions_static_64.lib",
		"cri_ware_pcx64_le_import",
	}

	postbuildcommands{
		"call $(SolutionDir)compileGLSLC.bat",
		"{COPY} ../%{LibDir.PhysX}/%{cfg.buildcfg}/*.dll \"../bin/" ..outputdir.. "/%{prj.name}/\"",
		"{COPY} ../%{LibDir.Assimp}/%{cfg.buildcfg}/*.dll \"../bin/" ..outputdir.. "/%{prj.name}/\"",
		"{COPY} ../%{LibDir.CRI}/*.dll \"../bin/" ..outputdir.. "/%{prj.name}/\"",
	}

	buildoptions{"/utf-8"}

	filter"system:windows"
		systemversion "latest"
		
		defines{
			"GLFW_INCLUDE_NONE",
		}		

	filter "configurations:Debug"
		defines "VK_DEBUG"
		runtime "Debug"
		symbols "on"

		links{			
			"assimp-vc143-mtd.lib",
		}

	filter "configurations:Release"
		defines "VK_RELEASE"
		runtime "Release"
		optimize "on"
		
		links{			
			"assimp-vc143-mt.lib",
		}

group "external"
	include "external/ImGui.lua"