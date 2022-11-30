#pragma once

#include <Proton2D.h>

using namespace proton;

class GameLayer : public AppLayer
{
public:
	virtual void OnCreate() override;
	virtual void OnUpdate(float ts) override;

private:
	Shared<Scene> m_Scene;
	Entity m_Player;
	Entity m_Level;
};