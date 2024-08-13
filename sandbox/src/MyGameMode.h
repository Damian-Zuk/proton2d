#pragma once

class Player;

class MyGameMode : public GameModeBase
{
public:
	GAME_MODE_CLASS(MyGameMode)

	virtual bool OnCreate() override;
	virtual void OnEvent(Event& event) override;

	virtual void Server_OnClientConnected(ClientID clientID) override;
	virtual void Server_OnClientDisconnected(ClientID clientID) override;

	virtual void Client_OnConnected(ClientID clientID) override;

	uint32_t GetLocalPlayerID() const;

	void SpawnRandomBox(const glm::vec2& position);

private:
	std::map<uint32_t, Player*> m_RemotePlayers;
	Player* m_LocalPlayer;
	ClientID m_LocalClientID = 0;
	uint32_t m_NewColorIndex = 1;

	friend class Player;
};
