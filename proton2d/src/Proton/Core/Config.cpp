#include "pch.h"
#include "Proton/Core/Config.h"
#include "Proton/Utils/Utils.h"

#include <nlohmann/json.hpp>

namespace proton {

	using json = nlohmann::json;

	ApplicationConfig::ApplicationConfig()
	{
		LoadConfig();
	}

	void ApplicationConfig::LoadConfig()
	{
		if (!std::filesystem::exists(m_Filepath))
		{
			PT_CORE_WARN("'{}' file not found! Creating with default values.", m_Filepath);
			WindowTitle = "Proton2D Engine";
			WriteConfig(WindowTitle, WindowWidth, WindowHeight, Fullscreen, VSync);
			return;
		}
		
		json jsonObj = jsonObj.parse(Utils::ReadFile(m_Filepath));
		WindowTitle = jsonObj["window_title"];
		WindowWidth = jsonObj["window_width"];
		WindowHeight = jsonObj["window_height"];
		Fullscreen = jsonObj["fullscreen"];
		VSync = jsonObj["vsync"];
		
	}

	void ApplicationConfig::WriteConfig(const std::string& windowTitle, int windowWidth, int windowHeight, bool fullscreen, bool vsync)
	{
		json jsonObj;
		jsonObj["window_title"] = windowTitle;;
		jsonObj["window_width"] = windowWidth;
		jsonObj["window_height"] = windowHeight;
		jsonObj["fullscreen"] = fullscreen;
		jsonObj["vsync"] = vsync;
		std::ofstream configFile(m_Filepath);
		configFile << jsonObj.dump(4);
		configFile.close();
	}

}
