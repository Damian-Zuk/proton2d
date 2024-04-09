#include "ptpch.h"
#include "Proton/Core/ProjectSettings.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Utils/Utils.h"
#include "Proton/Network/Common/Common.h"

#include <nlohmann/json.hpp>

namespace proton {

	using json = nlohmann::json;

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

		if (jsonObj.contains("server_ip"))
		{
			m_IpAddress = jsonObj.at("server_ip");
		}
		if (jsonObj.contains("port"))
		{
			m_Port = jsonObj.at("port");
		}
	#ifdef PROTON_DISTRIBUTION
		if (jsonObj.contains("net_mode"))
		{
			NetMode mode = StringToNetMode(jsonObj.at("net_mode"));
			Application::Get().GetGameInstance()->SetNetMode(mode);
		}
	#endif

		m_StartScene = jsonObj.at("start_scene");

		return true;
	}

	void ProjectSettings::WriteProjectSettings()
	{
		json jsonObj;
		jsonObj["start_scene"] = m_StartScene;
		NetMode netMode = Application::Get().GetGameInstance()->GetNetMode();
		jsonObj["net_mode"] = NetModeToString(netMode);
		jsonObj["server_ip"] = m_IpAddress;
		jsonObj["port"] = 8192;
		std::ofstream configFile(m_Filepath);
		configFile << jsonObj.dump(4);
		configFile.close();
	}

}
