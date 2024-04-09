#pragma once

class MyGameMode : public GameModeBase
{
public:
	GAME_MODE_CLASS(MyGameMode)

	virtual bool OnCreate() override;
	virtual void OnUpdate(float ts) override;
	virtual void OnDestroy() override;

	virtual void Server_OnClientConnected(uint32_t clientID) override;
	virtual void Server_OnClientDisconnected(uint32_t clientID) override;

private:
	std::map<uint32_t, Entity> m_RemotePlayers;
};
