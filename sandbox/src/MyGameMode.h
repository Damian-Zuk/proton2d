#pragma once

class Player;

class MyGameMode : public GameModeBase
{
public:
	GAME_MODE_CLASS(MyGameMode)

	virtual bool OnCreate() override;

	virtual void Server_OnClientConnected(uint32_t clientID) override;
	virtual void Server_OnClientDisconnected(uint32_t clientID) override;

	virtual void Client_OnConnected(uint32_t clientID) override;

	uint32_t GetLocalPlayerID() const;

private:
	std::map<uint32_t, Player*> m_RemotePlayers;
	Player* m_LocalPlayer;
	uint32_t m_LocalPlayerID = 0;
};
