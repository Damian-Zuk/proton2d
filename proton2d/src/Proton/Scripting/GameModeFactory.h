#pragma once
#include "Proton/Scene/Entity.h"

#define GAME_MODE_CLASS(game_mode_class) \
static inline const char __ClassName[] = #game_mode_class; \
static inline const bool __Registered = \
	proton::GameModeFactory::Get().RegisterGameMode([&](proton::Scene* scene) { \
		return scene->SetGameMode<game_mode_class>(); \
	}, #game_mode_class);

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
