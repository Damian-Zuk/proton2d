#pragma once

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
	std::map<uint32_t, Entity> m_RemotePlayers;
	Entity m_LocalPlayer;
	uint32_t m_LocalPlayerID = 0;
};
