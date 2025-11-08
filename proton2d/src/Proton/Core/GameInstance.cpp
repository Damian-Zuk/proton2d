#include "ptpch.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scripting/AppScript.h"
#include "Proton/Scripting/ScriptFactory.h"
#include "Proton/Network/NetworkManager.h"

namespace proton {

	GameInstance::GameInstance(const std::string& appScriptClassName)
		: m_SceneManager(MakeUnique<SceneManager>(this)),
		m_NetworkManager(MakeUnique<NetworkManager>(this))
	{
		SetAppScript(appScriptClassName);
	}

	GameInstance::~GameInstance()
	{
		if (m_AppScript && m_AppScript->m_Status == ScriptStatus::Initialized)
			m_AppScript->OnDestroy();
		delete m_AppScript;
	}

	void GameInstance::Init(bool loadStartScene)
	{
		if (!m_ProjectConfig.LoadConfig())
		{
			PT_CORE_ERROR("Project settings loading failed!");
			return;
		}

		if (m_IsMainInstance)
			m_NetworkManager->ReadConfig();

		if (m_AppScript->OnCreate())
			m_AppScript->m_Status = ScriptStatus::Initialized;
		else
			m_AppScript->m_Status = ScriptStatus::Failure;

		if (loadStartScene)
		{
			Scene* scene = m_SceneManager->Load(m_ProjectConfig.StartScene);
			m_SceneManager->SetActiveScene(scene);

		#ifdef PROTON_DISTRIBUTION
			scene->BeginPlay();
		#endif
		}
	}

	void GameInstance::OnSceneSimulationStart(Scene* scene)
	{
		m_NetworkManager->OnSceneSimulationStart(scene);
		m_SimulatedScenesCount++;
	}

	void GameInstance::OnSceneSimulationStop(Scene* scene)
	{
		m_NetworkManager->OnSceneSimulationStop(scene);
		m_SimulatedScenesCount--;
	}

	void GameInstance::OnUpdate(float ts)
	{
		m_AppScript->OnUpdate(ts);
		m_SceneManager->OnUpdate(ts);
		m_NetworkManager->OnUpdate(ts);
	}

	AppScript* GameInstance::SetAppScript(const std::string& className)
	{
		return ScriptFactory::Get().AddScriptToGameInstance(this, className);
	}

	Scene* GameInstance::GetActiveScene()
	{
		return m_SceneManager->GetActiveScene();
	}

	SceneManager* GameInstance::GetSceneManager()
	{
		return m_SceneManager.get();
	}

	NetworkManager* GameInstance::GetNetworkManager()
	{
		return m_NetworkManager.get();
	}

	void GameInstance::SetNetMode(NetMode mode)
	{
		return m_NetworkManager->SetNetMode(mode);
	}

	NetMode GameInstance::GetNetMode() const
	{
		return m_NetworkManager->GetNetMode();
	}
}
