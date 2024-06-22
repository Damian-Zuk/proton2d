#pragma once
#include "Proton/Network/Common/Common.h"
#include "Proton/Scene/Entity.h"

namespace proton {

	class Scene;
	class GameInstance;
	class NetworkManager;
	
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
		TGameMode* CastTo()
		{
			return m_Scene->GameModeCastTo<TGameMode>();
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
