#pragma once
#include "Proton/Scripting/Script.h"

namespace proton {

	class GameScript : public Script
	{
	public:
		GameScript();
		virtual ~GameScript() = default;

		virtual bool OnCreate() override { return true; }
		virtual void OnDestroy() override {};
		virtual void OnUpdate(float ts) {} ;
		virtual void OnFixedUpdate(float ts) override {};
		virtual void OnRegisterFields() override {};
		virtual void OnEvent(Event& event) override {};
		virtual void OnImGuiRender() override {};

		Scene* GetScene() const;
		Entity FindByTag(const std::string& tag) const;
		Entity FindByID(UUID uuid) const;
		Entity SpawnPrefab(const std::string& prefab) const;

		// Network
		bool HasAuthority() const;
		bool IsNetModeClient() const;
		bool IsNetModeServer() const;
		bool IsNetModeDedicatedServer() const;

		virtual void Client_OnConnected() {}
		virtual void Server_OnClientConnected(ClientID clientID) {}

		virtual void Client_OnDisconnected(ConnectionEndCode code) {}
		virtual void Server_OnClientDisconnected(ClientID clientID) {}

		virtual void Client_OnCustomMessage(NetworkStreamReader& stream) {};
		virtual void Server_OnCustomMessage(ClientID clientID, NetworkStreamReader& stream) {};

		void Client_SendCustomMessage(const NetworkStreamWriterDelegate& delegate) const;
		void Server_SendCustomMessage(ClientID clientID, const NetworkStreamWriterDelegate& delegate) const;

		void Server_SetClientEntity(ClientID clientID, Entity entity) const;
		Entity Server_GetClientEntity(ClientID clientID) const;

	private:
		Scene* m_Scene = nullptr;

		friend class Scene;
	};

}