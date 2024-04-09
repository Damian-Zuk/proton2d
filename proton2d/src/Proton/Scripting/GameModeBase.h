#pragma once
#include "Proton/Network/Common/Common.h"

namespace proton {

	class Scene;
	class GameInstance;
	
	class GameModeBase
	{
	public:
		virtual bool OnCreate() { return true; }
		virtual void OnUpdate(float ts) {}
		virtual void OnDestroy() {}

		// TODO: change clientID to something else
		virtual void Server_OnClientConnected(uint32_t clientID) {}
		virtual void Server_OnClientDisconnected(uint32_t clientID) {}

		void Server_SetOnRecvPlayerActionCallback(uint32_t clientID, OnRecvPlayerActionCallback function);
		void Client_SendPlayerAction(OnSendPlayerActionFunc function);
	
		bool HasAuthority() const;
		bool IsRunningServer() const;
		bool IsRunningClient() const;

		Scene* GetScene() const;

	private:
		Scene* m_Scene;

		friend class Scene;
		friend class Client;
		friend class Server;
	};

}
