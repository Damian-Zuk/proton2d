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
IncludeDir["glm"] = "vendor/glm"
IncludeDir["ImGui"] = "vendor/imgui"
IncludeDir["stb"] = "vendor/stb"
IncludeDir["entt"] = "vendor/entt/src"

group "Dependencies"
	include "vendor/GLFW"
	include "vendor/glad"
	include "vendor/imgui"
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
		"vendor/stb/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.glad}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.entt}"
	}

	links
	{
		"glad",
		"GLFW",
		"ImGui",
		"opengl32.lib"
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
			"PROTON_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

		postbuildcommands
		{
			"{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/sandbox/\""
		}

	filter "configurations:Debug"
		defines "PROTON_DEBUG"
		symbols "on"

	filter "configurations:Release"
		defines "PROTON_RELEASE"
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
		"%{IncludeDir.entt}"
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
