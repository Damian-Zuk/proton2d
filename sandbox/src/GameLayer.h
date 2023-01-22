#pragma once

class GameLayer : public proton::AppLayer
{
public:
	virtual void OnCreate() override;
	virtual void OnUpdate(float ts) override;
	virtual void OnEvent(proton::Event& e);
};