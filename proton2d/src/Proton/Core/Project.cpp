#include "ptpch.h"
#include "Proton/Core/Project.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Utils/Utils.h"
#include "Proton/Network/Common/Network.h"

#include <nlohmann/json.hpp>

namespace proton {

	using json = nlohmann::json;

	bool Project::LoadProjectSettings()
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

	static std::string NetModeToString(NetMode netMode)
	{
		switch (netMode)
		{
		case NetMode::Standalone:
			return "Standalone";
		case NetMode::ListenServer:
			return "ListenServer";
		case NetMode::DedicatedServer:
			return "DedicatedServer";
		case NetMode::Client:
			return "Client";
		}
		return "Standalone";
	}

	static NetMode StringToNetMode(const std::string& netModeStr)
	{
		if (netModeStr == "ListenServer")
			return NetMode::ListenServer;
		if (netModeStr == "DedicatedServer")
			return NetMode::DedicatedServer;
		if (netModeStr == "Client")
			return NetMode::Client;
		return NetMode::Standalone;
	}

	void Project::WriteProjectSettings()
	{
		json jsonObj;
		jsonObj["start_scene"] = m_StartScene;
		jsonObj["net_mode"] = Application::Get().GetGameInstance()->GetNetMode();
		std::ofstream configFile(m_Filepath);
		configFile << jsonObj.dump(4);
		configFile.close();
	}

}
