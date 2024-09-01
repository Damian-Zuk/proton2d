#include "ptpch.h"
#include "Proton/Core/ProjectSettings.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Utils/Utils.h"
#include "Proton/Network/Common.h"

#include <nlohmann/json.hpp>

namespace proton {

	using json = nlohmann::json;

	bool ProjectSettings::LoadProjectSettings()
	{
		if (!std::filesystem::exists(m_Filepath))
		{
			PT_CORE_ERROR("'{}' file not found!", m_Filepath);
			return false;
		}

		json jsonObj = jsonObj.parse(Utils::ReadFile(m_Filepath));
		if (!jsonObj.contains("start_scene"))
		{
			PT_CORE_ERROR("'start_scene' missing in '{}'!", m_Filepath);
			return false;
		}

		m_StartScene = jsonObj.at("start_scene");

		return true;
	}

	void ProjectSettings::WriteProjectSettings()
	{
		json jsonObj;
		jsonObj["start_scene"] = m_StartScene;
		std::ofstream configFile(m_Filepath);
		configFile << jsonObj.dump(4);
		configFile.close();
	}

}
