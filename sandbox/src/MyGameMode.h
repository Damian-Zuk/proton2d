#pragma once

class Player;

enum class GameMessageType : uint16_t
{
	PlayerInput = 0,
};

class MyGameMode : public GameScript
{
public:
	GAME_SCRIPT_CLASS(MyGameMode)

	virtual bool OnCreate() override;
	virtual void OnUpdate(float ts) override;
	virtual void OnEvent(Event& event) override;
	virtual void OnImGuiRender() override;

	virtual void Server_OnClientConnected(ClientID clientID) override;
	virtual void Server_OnClientDisconnected(ClientID clientID) override;

	virtual void Client_OnCustomMessage(NetworkStreamReader& stream) override;
	virtual void Server_OnCustomMessage(ClientID clientID, NetworkStreamReader& stream) override;

private:
	Player* m_LocalPlayer = nullptr;

	// Server stuff
	uint32_t m_NewPlayerColorIndex = 1;
	std::map<uint32_t, Player*> m_RemotePlayers;

	friend class Player;
};
