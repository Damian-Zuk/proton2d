#pragma once
#include "Proton/Scene/Entity.h"

namespace proton {

	class Scene;
	class GameModeBase;

	class GameModeFactory
	{
	public:
		GameModeFactory();
		using InstantiateGameModeFunction = std::function<GameModeBase* (Scene* scene)>;

		static GameModeFactory& Get(); // Get singleton instance

		GameModeBase* InstantiateGameMode(Scene* scene, const std::string& className);
		bool RegisterGameMode(const InstantiateGameModeFunction& function, const std::string& className);

	private:
		std::unordered_map<std::string, InstantiateGameModeFunction> m_GameModeRegistry;

		friend class InspectorPanel;
	};

}
