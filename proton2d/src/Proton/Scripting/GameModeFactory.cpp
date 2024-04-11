#include "ptpch.h"
#include "Proton/Scripting/GameModeFactory.h"
#include "Proton/Scripting/GameModeBase.h"

namespace proton {

	GameModeFactory::GameModeFactory()
	{
		m_GameModeRegistry["GameModeBase"] = [&](Scene* scene) {
				return scene->SetGameMode<GameModeBase>();
			};
	}

	GameModeFactory& GameModeFactory::Get()
	{
		static GameModeFactory instance;
		return instance;
	}

	GameModeBase* GameModeFactory::InstantiateGameMode(Scene* scene, const std::string& className)
	{
		if (m_GameModeRegistry.find(className) == m_GameModeRegistry.end())
		{
			PT_CORE_ERROR("GameMode '{}' not found!", className);
			return nullptr;
		}
		InstantiateGameModeFunction& instantiateFunction = m_GameModeRegistry.at(className);
		GameModeBase* gameMode = instantiateFunction(scene);
		return gameMode;
	}

	bool GameModeFactory::RegisterGameMode(const InstantiateGameModeFunction& function, const std::string& className)
	{
		if (m_GameModeRegistry.find(className) != m_GameModeRegistry.end())
		{
			PT_CORE_ERROR("GameMode '{}' already exists", className);
			return false;
		}
		m_GameModeRegistry[className] = function;
		return true;
	}

}
