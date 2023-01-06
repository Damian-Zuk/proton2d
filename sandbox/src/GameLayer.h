#pragma once

#include <Proton2D.h>

using namespace proton;

class GameLayer : public AppLayer
{
public:
	virtual void OnCreate() override;
	virtual void OnUpdate(float ts) override;
	virtual void OnEvent(Event& e);

private:
	Scene* m_Scene;
	Entity m_Player;
	Entity m_Level;

	float m_ZoomLevelTarget = 1.0f;
	float m_CameraZoomSpeed = 0.10f;
};