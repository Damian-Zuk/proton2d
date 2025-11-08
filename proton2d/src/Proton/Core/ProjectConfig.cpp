#include "ptpch.h"
#include "Proton/Core/ProjectConfig.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Utils/Utils.h"
#include "Proton/Network/Common.h"
#include "Proton/Scripting/ScriptSerializer.h"
#include "Proton/Scripting/ScriptFactory.h"

namespace proton {

	bool ProjectConfig::LoadConfig()
	{
		if (!std::filesystem::exists(m_Filepath))
		{
			PT_CORE_ERROR("'{}' file not found!", m_Filepath);
			return false;
		}

		json j = j.parse(Utils::ReadFile(m_Filepath));
		if (!j.contains("StartScene"))
		{
			PT_CORE_ERROR("'StartScene' missing in '{}'!", m_Filepath);
			return false;
		}

		StartScene = j["StartScene"];
		AppScript* script = ScriptFactory::Get().AddScriptToGameInstance(Application::GetGameInstance(), j["AppScript"]["ClassName"]);
		ScriptSerializer::DeserializeScript((Script*)script, j);

		return true;
	}

	void ProjectConfig::WriteConfig()
	{
		json jsonObj;
		jsonObj["StartScene"] = StartScene;
		jsonObj["AppScript"] = ScriptSerializer::SerializeScript((Script*)Application::GetGameInstance()->GetAppScript());

		std::ofstream configFile(m_Filepath);
		configFile << jsonObj.dump(4);
		configFile.close();
	}

}
