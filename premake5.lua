workspace "Proton2D"
	architecture "x64"
    startproject "sandbox"

	configurations
	{
		"Debug",
		"Release",
		"Distribution"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "vendor/GLFW/include"
IncludeDir["glad"] = "vendor/glad/include"
IncludeDir["glm"] = "vendor/glm"
IncludeDir["ImGui"] = "vendor/imgui"
IncludeDir["stb"] = "vendor/stb"
IncludeDir["entt"] = "vendor/entt/src"
IncludeDir["json"] = "vendor/json"
IncludeDir["box2d"] = "vendor/box2d/include"

group "Dependencies"
	include "vendor/GLFW"
	include "vendor/glad"
	include "vendor/imgui"
	include "vendor/box2d"
group ""

project "proton2d"
	location "proton2d"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("obj/" .. outputdir .. "/%{prj.name}")

	pchheader "pch.h"
	pchsource "%{prj.location}/src/pch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"vendor/stb/**.h",
		"vendor/stb/**.cpp",
		"vendor/json/**.hpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.glad}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.json}",
		"%{IncludeDir.box2d}"
	}

	links
	{
		"glad",
		"GLFW",
		"ImGui",
		"opengl32.lib",
		"box2d"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"PROTON_PLATFORM_WINDOWS",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug"
		defines "PROTON_DEBUG"
		symbols "on"

	filter "configurations:Release"
		defines "PROTON_RELEASE"
		optimize "on"

	filter "configurations:Distribution"
		defines "PROTON_DISTRIBUTION"
		runtime "Release"
		optimize "on"


project "sandbox"
	location "sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("obj/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"proton2d/src",
		"vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.json}",
		"%{IncludeDir.box2d}"
	}

	links
	{
		"proton2d"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"PROTON_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "PROTON_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "PROTON_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Distribution"
		defines "PROTON_DISTRIBUTION"
		runtime "Release"
		optimize "on"
