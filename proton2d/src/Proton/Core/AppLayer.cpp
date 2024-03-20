#include "ptpch.h"
#include "Proton/Core/AppLayer.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"

namespace proton {

	GameInstance* AppLayer::GetGameInstance()
	{
		return m_GameInstance;
	}
	SceneManager* AppLayer::GetSceneManager()
	{
		return GetGameInstance()->GetSceneManager();
	}

}
