#include "ptpch.h"
#include "Proton/Core/AppConfig.h"
#include "Proton/Utils/Utils.h"

namespace proton {

	void AppConfig::LoadConfig()
	{
		if (!std::filesystem::exists(m_Filepath))
		{
			PT_CORE_WARN("'{}' file not found! Creating with default values.", m_Filepath);
			WindowTitle = "Proton2D Engine";
			WriteConfig();
			return;
		}
		
		json jsonObj = jsonObj.parse(Utils::ReadFile(m_Filepath));
		WindowTitle = jsonObj["WindowTitle"];
		WindowWidth = jsonObj["WindowWidth"];
		WindowHeight = jsonObj["WindowHeight"];
		Fullscreen = jsonObj["Fullscreen"];
		VSync = jsonObj["VSync"];
		
	}

	void AppConfig::WriteConfig()
	{
		json jsonObj;
		jsonObj["WindowTitle"] = WindowTitle;;
		jsonObj["WindowWidth"] = WindowWidth;
		jsonObj["WindowHeight"] = WindowHeight;
		jsonObj["Fullscreen"] = Fullscreen;
		jsonObj["VSync"] = VSync;
		std::ofstream configFile(m_Filepath);
		configFile << jsonObj.dump(4);
		configFile.close();
	}

}
