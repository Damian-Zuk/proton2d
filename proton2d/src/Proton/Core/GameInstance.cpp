#include "ptpch.h"
#include "GameInstance.h"

namespace proton {

	GameInstance::GameInstance()
		: m_SceneManager(this)
	{
	}

	GameInstance::~GameInstance()
	{
	}

	void GameInstance::Init()
	{
		if (!m_Project.LoadProjectSettings())
		{
			PT_CORE_ERROR("Project settings loading failed!");
		}
		else
		{
			m_SceneManager.Load(m_Project.m_StartScene);
			m_SceneManager.SetActiveScene(m_Project.m_StartScene);
		}
	}
}