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
	
	// Scene Game Mode Script Base Class
	class GameModeBase
	{
	public:
		static inline const char __ClassName[] = "GameModeBase";

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

		// ----------------- Networking Methods -----------------
		
		virtual void Client_OnConnected() {}
		virtual void Server_OnClientConnected(ClientID clientID) {}

		virtual void Client_OnConnectionFailed(NetConnectionEndCode code) {};

		virtual void Client_OnDisconnected() {}
		virtual void Server_OnClientDisconnected(ClientID clientID) {}
		
		virtual void Client_OnCustomMessage(NetworkStreamReader& stream) {};
		virtual void Server_OnCustomMessage(ClientID clientID, NetworkStreamReader& stream) {};

		void Client_SendCustomMessage(const NetworkStreamWriterDelegate& delegate);
		void Server_SendCustomMessage(ClientID clientID, const NetworkStreamWriterDelegate& delegate);

		void Server_SetClientEntity(ClientID clientID, Entity entity) const;
		Entity Server_GetClientEntity(ClientID clientID) const;
	
		bool HasAuthority() const;
		bool IsNetModeServer() const;
		bool IsNetModeClient() const;

	private:
		Scene* m_Scene;

		friend class Scene;
		friend class Client;
		friend class Server;
	};

}
