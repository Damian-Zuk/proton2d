#include "ptpch.h"
#include "Proton/Scripting/GameScript.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/PrefabManager.h"
#include "Proton/Network/NetworkManager.h"
#include "Proton/Network/Client.h"
#include "Proton/Network/Server.h"

namespace proton {

	GameScript::GameScript()
        : Script(ScriptType::GameScript)
	{
        m_ScriptType = ScriptType::GameScript;
	}

    Entity GameScript::FindByTag(const std::string& tag) const
    {
        return m_Scene->FindByTag(tag);
    }

    Entity GameScript::SpawnPrefab(const std::string& prefab) const
    {
        return PrefabManager::Spawn(m_Scene, prefab);
    }

    Entity GameScript::FindByID(UUID uuid) const
    {
        return m_Scene->FindByID(uuid);
    }

    Scene* GameScript::GetScene() const
    {
        return m_Scene;
    }

    void GameScript::Client_SendCustomMessage(const NetworkStreamWriterDelegate& delegate) const
    {
        GetNetworkManager()->Client_SendCustomMessage(delegate);
    }

    void GameScript::Server_SendCustomMessage(ClientID clientID, const NetworkStreamWriterDelegate& delegate) const
    {
        GetNetworkManager()->Server_SendCustomMessage(clientID, delegate);
    }

    void GameScript::Server_SetClientEntity(ClientID clientID, Entity entity) const
    {
        GetNetworkManager()->GetServer()->SetClientEntity(clientID, entity);
    }

    Entity GameScript::Server_GetClientEntity(ClientID clientID) const
    {
        return GetNetworkManager()->Server_GetClientEntity(clientID);
    }

    bool GameScript::HasAuthority() const
    {
        NetMode netMode = GetNetworkManager()->GetNetMode();
        return netMode != NetMode::Client;
    }

    bool GameScript::IsNetModeServer() const
    {
        NetMode netMode = GetNetworkManager()->GetNetMode();
        return netMode == NetMode::ListenServer || netMode == NetMode::DedicatedServer;
    }

    bool GameScript::IsNetModeDedicatedServer() const
    {
        NetMode netMode = GetNetworkManager()->GetNetMode();
        return netMode == NetMode::DedicatedServer;
    }

    bool GameScript::IsNetModeClient() const
    {
        return !HasAuthority();
    }

}
