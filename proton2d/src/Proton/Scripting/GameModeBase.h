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

		// TODO: change clientID to something else
		virtual void Server_OnClientConnected(uint32_t clientID) {}
		virtual void Server_OnClientDisconnected(uint32_t clientID) {}

		virtual void Client_OnConnected(uint32_t clientID) {}
		virtual void Client_OnDisconnected() {}

		void Server_SetOnRecvPlayerActionCallback(uint32_t clientID, OnRecvPlayerActionCallback function);
		void Server_OnEntityCreated(Entity entity, uint32_t specificClientID = 0);
		void Server_OnEntityDestroyed(Entity entity);
		
		void Client_SendPlayerAction(OnSendPlayerActionFunc function);
	
		bool HasAuthority() const;
		bool IsRunningServer() const;
		bool IsRunningClient() const;

		Scene* GetScene() const;
		NetworkManager* GetNetworkManager() const;

		template<typename TGameMode>
		TGameMode* CastTo()
		{
			return m_Scene->GameModeCastTo<TGameMode>();
		}

	public:
		static inline const char __ClassName[] = "GameModeBase";

	private:
		Scene* m_Scene;

		friend class Scene;
		friend class Client;
		friend class Server;
	};

}
