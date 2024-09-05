#include "ptpch.h"
#include "Proton/Scripting/GameModeBase.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/PrefabManager.h"

#include "Proton/Network/NetworkManager.h"
#include "Proton/Network/Client.h"
#include "Proton/Network/Server.h"

namespace proton {

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

    void GameModeBase::Client_SendCustomMessage(const NetworkStreamWriterDelegate& delegate)
    {
        GetNetworkManager()->Client_SendCustomMessage(delegate);
    }

    void GameModeBase::Server_SendCustomMessage(ClientID clientID, const NetworkStreamWriterDelegate& delegate)
    {
        GetNetworkManager()->Server_SendCustomMessage(clientID, delegate);
    }

    void GameModeBase::Server_SetClientEntity(ClientID clientID, Entity entity) const
    {
        GetNetworkManager()->GetServer()->SetClientEntity(clientID, entity);
    }

    Entity GameModeBase::Server_GetClientEntity(ClientID clientID) const
    {
        return GetNetworkManager()->Server_GetClientEntity(clientID);
    }

    bool GameModeBase::HasAuthority() const
    {
        NetMode netMode = GetScene()->GetGameInstance()->GetNetworkManager()->GetNetMode();
        return netMode != NetMode::Client;
    }

    bool GameModeBase::IsNetModeServer() const
    {
        NetMode netMode = GetScene()->GetGameInstance()->GetNetworkManager()->GetNetMode();
        return netMode == NetMode::ListenServer || netMode == NetMode::DedicatedServer;
    }

    bool GameModeBase::IsNetModeClient() const
    {
        return !HasAuthority();
    }

}
