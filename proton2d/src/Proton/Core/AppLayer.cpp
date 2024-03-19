#include "ptpch.h"
#include "Proton/Core/AppLayer.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"

namespace proton {

	SceneManager* AppLayer::GetSceneManager()
	{
		return Application::GetGameInstance()->GetSceneManager();
	}
}
