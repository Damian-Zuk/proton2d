#include "ptpch.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Network/Common/NetworkManager.h"

namespace proton {

	GameInstance::GameInstance() : 
		m_SceneManager(MakeUnique<SceneManager>(this)),
		m_NetworkManager(MakeUnique<NetworkManager>(this, m_SceneManager.get()))
	{
	}

	GameInstance::~GameInstance()
	{
	}

	void GameInstance::Init(bool loadStartScene)
	{
		if (!m_ProjectSettings.LoadProjectSettings())
		{
			PT_CORE_ERROR("Project settings loading failed!");
		}
		else if (loadStartScene)
		{
			m_SceneManager->Load(m_ProjectSettings.m_StartScene);
			Scene* scene = m_SceneManager->SetActiveScene(m_ProjectSettings.m_StartScene);

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
		m_SceneManager->OnUpdate(ts);
		m_NetworkManager->OnUpdate(ts);
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
