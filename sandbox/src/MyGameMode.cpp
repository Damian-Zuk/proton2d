#include <Proton.h>
using namespace proton;

#include "MyGameMode.h"

#include "Scripts/Player.h"
#include "Scripts/Network/DynamicExtrapolation.h"
#include "Scripts/Misc/Colors.h"

bool MyGameMode::OnCreate()
{
	if (HasAuthority() && !IsNetModeDedicatedServer())
	{
		// Check if player prefab is already on the scene
		if (Entity player = FindByTag("Player"))
			m_LocalPlayer = player.As<Player>();
		else
			m_LocalPlayer = SpawnPrefab("Player").As<Player>();
		
		// Spawn local player
		auto& spawnTransform = FindByTag("PlayerSpawn0").GetTransform();
		m_LocalPlayer->SetWorldPosition(spawnTransform.WorldPosition);
		m_LocalPlayer->SetPlayerColor(COLOR_RED);
		m_LocalPlayer->SetPlayerNick(GetNetworkManager()->GetLocalClientName());
	}
	return true;
}

void MyGameMode::OnUpdate(float ts)
{
}

void MyGameMode::Server_OnClientConnected(ClientID clientID)
{
	static constexpr glm::vec4 PlayerColors[] = {
		COLOR_RED, COLOR_GREEN, COLOR_ORANGE, COLOR_YELLOW, COLOR_CYAN,
		COLOR_PURPLE, COLOR_PINK, COLOR_BLUE, COLOR_WHITE, COLOR_BLACK,
	};
		
	// Spawn Player object for connected client
	Player* player = SpawnPrefab("Player").As<Player>();
	player->m_ClientID = clientID;
	player->m_IsLocalPlayer = false;

	// Get spawn point position
	uint32_t spawnPoint = (m_RemotePlayers.size() + 1) % 4;
	auto& spawnTransform = FindByTag("PlayerSpawn" + std::to_string(spawnPoint)).GetTransform();
	player->SetWorldPosition(spawnTransform.WorldPosition);

	// Set player color
	NetworkManager* networkManager = GetNetworkManager();
	player->SetPlayerColor(PlayerColors[m_NewPlayerColorIndex]);
	player->SetPlayerNick(networkManager->Server_GetClientInfo(clientID).ClientName);
	m_NewPlayerColorIndex = (m_NewPlayerColorIndex + 1) % 10;
	
	m_RemotePlayers[clientID] = player;
	Server_SetClientEntity(clientID, *player);
	PT_TRACE("client_id={}", clientID);
}

void MyGameMode::Server_OnClientDisconnected(ClientID clientID)
{
	Player* player = m_RemotePlayers.at(clientID);
	player->Destroy();
	m_RemotePlayers.erase(clientID);
	PT_TRACE("client_id={}", clientID);
}

void MyGameMode::Client_OnCustomMessage(NetworkStreamReader& stream)
{
}

void MyGameMode::Server_OnCustomMessage(ClientID clientID, NetworkStreamReader& stream)
{
	GameMessageType msgType;
	stream.ReadRaw(msgType);

	switch (msgType)
	{
	case GameMessageType::PlayerInput:
	{
		Player* remotePlayer = Server_GetClientEntity(clientID).As<Player>();
		if (!stream.ReadRaw(remotePlayer->m_InputState))
			PT_ERROR("Failed to read player input! (client_id={})", clientID);
		break;
	}
	}
}

void MyGameMode::OnEvent(Event& event)
{
	EventDispatcher(event).Dispatch<KeyPressedEvent>([&](auto& keyEvent)
		{
			if (!HasAuthority())
				return false;

			Scene* scene = GetScene();
			const auto& cursor = scene->GetCursorWorldPosition();

			// Spawn Ball on E key
			if (keyEvent.GetKeyCode() == Key::E)
			{
				Entity balls = scene->FindByTag("Balls");
				if (!balls) balls = scene->CreateEntity("Balls");

				Entity ball = scene->SpawnPrefab("Ball");
				ball.SetWorldPosition(cursor);
				balls.AddChildEntity(ball);
			}
			// Spawn Box on R key
			else if (keyEvent.GetKeyCode() == Key::R)
			{
				Entity boxes = scene->FindByTag("Boxes");
				if (!boxes) boxes = scene->CreateEntity("Boxes");

				Entity box = scene->SpawnPrefab("Wooden Box");
				box.SetWorldPosition(cursor);
				boxes.AddChildEntity(box);
			}
			return false;
		});
}

void MyGameMode::OnImGuiRender() // Debug
{
#ifdef PT_EDITOR
	Scene* scene = GetScene();

	ImGui::Text("Network");
	ImGui::Dummy({ 0, 1 });
	ImGui::SetNextItemWidth(200.0f);
	ImGui::Separator();
	ImGui::Dummy({ 0, 2 });
	
	static float cullDistance = 20.0f;
	ImGui::PushItemWidth(100.0f);
	ImGui::DragFloat("Cull Distance", &cullDistance); ImGui::SameLine();
	ImGui::PopItemWidth();
	if (ImGui::Button("Set##cd"))
	{
		auto view = scene->GetAllEntitiesWith<NetworkComponent>();
		for (auto e : view)
		{
			auto& net = view.get<NetworkComponent>(e);
			net.NetTransform.CullDistance = cullDistance;
		}
	}

	ImGui::Dummy({ 0, 5 });
	ImGui::Text("Dynamic Extrapolation");
	ImGui::Dummy({ 0, 1 });
	ImGui::SetNextItemWidth(200.0f);
	ImGui::Separator();
	ImGui::Dummy({ 0, 2 });

	static float interpolationThreshold = 0.05f;
	ImGui::PushItemWidth(100.0f);
	ImGui::DragFloat("Interpolation Threshold", &interpolationThreshold, 0.001f); ImGui::SameLine();
	ImGui::PopItemWidth();
	if (ImGui::Button("Set##it"))
	{
		for (auto entity : scene->FindAllScripts<DynamicExtrapolation>())
			entity->InterpolationThreshold = interpolationThreshold;
	}

	static float switchCooldownTime = 0.25f;
	ImGui::PushItemWidth(100.0f);
	ImGui::DragFloat("Switch Cooldown Time", &switchCooldownTime, 0.01f); ImGui::SameLine();
	ImGui::PopItemWidth();
	if (ImGui::Button("Set##cdt"))
	{
		for (auto entity : scene->FindAllScripts<DynamicExtrapolation>())
		{
			entity->SwitchCooldownTime = switchCooldownTime;
			PT_CORE_TRACE("{}", entity->GetTag());
		}
	}

	static bool enableDynamic = true;
	ImGui::PushItemWidth(100.0f);
	ImGui::Checkbox("Enable", &enableDynamic); ImGui::SameLine();
	ImGui::PopItemWidth();
	if (ImGui::Button("Set##en"))
	{
		for (auto entity : scene->FindAllScripts<DynamicExtrapolation>())
			entity->Enable = enableDynamic;
	}

	static bool alphaVisualize = true;
	ImGui::PushItemWidth(100.0f);
	ImGui::Checkbox("Alpha Visualize", &alphaVisualize); ImGui::SameLine();
	ImGui::PopItemWidth();
	if (ImGui::Button("Set##av"))
	{
		for (auto entity : scene->FindAllScripts<DynamicExtrapolation>())
			entity->AlphaVisualize = alphaVisualize;
	}
#endif
}
