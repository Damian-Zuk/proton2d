#include "ptpch.h"
#include "Proton/Scripting/GameModeBase.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/PrefabManager.h"

#include "Proton/Network/Common/NetworkManager.h"
#include "Proton/Network/Client/Client.h"
#include "Proton/Network/Server/Server.h"

namespace proton {

    void GameModeBase::Server_SetPlayerActionCallback(uint32_t clientID, OnRecvPlayerActionCallback function)
    {
        Server* server = m_Scene->m_GameInstance->GetNetworkManager()->GetServer();
        if (!server)
        {
            PT_CORE_ERROR("Server instance is not running");
            return;
        }
        server->SetPlayerActionCallback(clientID, function);
    }

    void GameModeBase::Client_SendPlayerAction(OnSendPlayerActionFunc function)
    {
        Client* client = m_Scene->m_GameInstance->GetNetworkManager()->GetClient();
        if (!client)
        {
            PT_CORE_ERROR("Client instance is not running");
            return;
        }
        client->SendPlayerAction(function);
    }

    void GameModeBase::Server_OnEntityCreated(Entity entity, uint32_t specificClientID)
    {
        GetNetworkManager()->GetServer()->OnEntityCreated(entity, specificClientID);
    }

    void GameModeBase::Server_OnEntityDestroyed(Entity entity, uint32_t specificClientID)
    {
        GetNetworkManager()->GetServer()->OnEntityDestroyed(entity, specificClientID);
    }

    void GameModeBase::Server_OnEntityCreated(EntityScript* script, uint32_t specificClientID)
    {
        Server_OnEntityCreated(*(Entity*)script, specificClientID);
    }

    void GameModeBase::Server_OnEntityDestroyed(EntityScript* script, uint32_t specificClientID)
    {
        Server_OnEntityDestroyed(*(Entity*)script, specificClientID);
    }

    bool GameModeBase::HasAuthority() const
    {
        NetMode netMode = GetScene()->GetOwningGameInstance()->GetNetworkManager()->GetNetMode();
        return netMode != NetMode::Client;
    }

    bool GameModeBase::IsRunningServer() const
    {
        NetMode netMode = GetScene()->GetOwningGameInstance()->GetNetworkManager()->GetNetMode();
        return netMode == NetMode::ListenServer || netMode == NetMode::DedicatedServer;
    }

    bool GameModeBase::IsRunningClient() const
    {
        return !HasAuthority();
    }

    Entity GameModeBase::FindByTag(const std::string& tag)
    {
        return m_Scene->FindByTag(tag);
    }

    Entity GameModeBase::SpawnPrefab(const std::string& prefab)
    {
        return PrefabManager::Spawn(m_Scene, prefab);
    }

    Scene* GameModeBase::GetScene() const
    {
        return m_Scene;
    }

    SceneManager* GameModeBase::GetSceneManager() const
    {
        return m_Scene->m_GameInstance->GetSceneManager();
    }

    NetworkManager* GameModeBase::GetNetworkManager() const
    {
        return m_Scene->m_GameInstance->GetNetworkManager();
    }

}
