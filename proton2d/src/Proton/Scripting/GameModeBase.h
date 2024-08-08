#pragma once
#include "Proton/Network/Common.h"
#include "Proton/Scene/Entity.h"

#define GAME_MODE_CLASS(game_mode_class) \
static inline const char __ClassName[] = #game_mode_class; \
static inline const bool __Registered = \
	proton::ScriptFactory::Get().RegisterGameMode([&](proton::Scene* scene) { \
		return scene->SetGameMode<game_mode_class>(); \
	}, #game_mode_class);

namespace proton {

	class Scene;
	class GameInstance;
	class NetworkManager;
	
	// Scene Game Mode Script Base
	class GameModeBase
	{
	public:
		virtual bool OnCreate() { return true; }
		virtual void OnUpdate(float ts) {}
		virtual void OnDestroy() {}
		virtual void OnEvent(Event& event) {}

		Entity FindByTag(const std::string& tag);
		Entity SpawnPrefab(const std::string& prefab);

		Scene* GetScene() const;
		SceneManager* GetSceneManager() const;
		NetworkManager* GetNetworkManager() const;

		template<typename TGameMode>
		TGameMode* As()
		{
			return dynamic_cast<TGameMode*>(this);
		}

		// Networking methods

		virtual void Server_OnClientConnected(uint32_t clientID) {}
		virtual void Server_OnClientDisconnected(uint32_t clientID) {}

		virtual void Client_OnConnected(uint32_t clientID) {}
		virtual void Client_OnDisconnected() {}
		
		void Server_SetPlayerActionCallback(uint32_t clientID, StreamReaderDelegate function);
		void Client_SendPlayerAction(StreamWriterDelegate function);
	
		bool HasAuthority() const;
		bool IsRunningServer() const;
		bool IsRunningClient() const;

	public:
		static inline const char __ClassName[] = "GameModeBase";

	private:
		Scene* m_Scene;

		friend class Scene;
		friend class Client;
		friend class Server;
	};

}
