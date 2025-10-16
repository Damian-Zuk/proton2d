#include "ptpch.h"
#include "Proton/Core/ProjectConfig.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Utils/Utils.h"
#include "Proton/Network/Common.h"

namespace proton {

	bool ProjectConfig::LoadConfig()
	{
		if (!std::filesystem::exists(m_Filepath))
		{
			PT_CORE_ERROR("'{}' file not found!", m_Filepath);
			return false;
		}

		json jsonObj = jsonObj.parse(Utils::ReadFile(m_Filepath));
		if (!jsonObj.contains("StartScene"))
		{
			PT_CORE_ERROR("'StartScene' missing in '{}'!", m_Filepath);
			return false;
		}

		StartScene = jsonObj["StartScene"];

		return true;
	}

	void ProjectConfig::WriteConfig()
	{
		json jsonObj;
		jsonObj["StartScene"] = StartScene;
		std::ofstream configFile(m_Filepath);
		configFile << jsonObj.dump(4);
		configFile.close();
	}

}
