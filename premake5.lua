workspace "Proton2D"
	architecture "x64"
    startproject "sandbox"

	configurations
	{
		"Debug",
		"Release"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "vendor/GLFW/include"
IncludeDir["glad"] = "vendor/glad/include"

include "/vendor/GLFW"
include "/vendor/glad"

project "proton2d"
	location "proton2d"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("obj/" .. outputdir .. "/%{prj.name}")

	pchheader "pch.h"
	pchsource "%{prj.location}/src/pch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.glad}"
	}

	links
	{
		"glad",
		"GLFW",
		"opengl32.lib"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "on"
		systemversion "latest"

		defines
		{
			"PROTON_PLATFORM_WINDOWS",
			"PROTON_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

		postbuildcommands
		{
			("{COPY} \"%{cfg.buildtarget.relpath}\" \"../bin/" .. outputdir .. "/sandbox\"")
		}

	filter "configurations:Debug"
		defines "PROTON_DEBUG"
		symbols "On"
		buildoptions "/MDd"
        runtime "Debug"

	filter "configurations:Release"
		defines "PROTON_RELEASE"
		optimize "On"
		buildoptions "/MD"
        runtime "Release"


project "sandbox"
	location "sandbox"
	kind "ConsoleApp"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("obj/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"proton2d/src"
	}

	links
	{
		"proton2d"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "on"
		systemversion "latest"

		defines
		{
			"PROTON_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "PROTON_DEBUG"
		symbols "On"
		buildoptions "/MDd"
        runtime "Debug"

	filter "configurations:Release"
		defines "PROTON_RELEASE"
		optimize "On"
		buildoptions "/MD"
        runtime "Release"
