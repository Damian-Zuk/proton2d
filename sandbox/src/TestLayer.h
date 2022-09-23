#pragma once

#include <Proton2D.h>

using namespace proton;

class TestLayer : public Layer
{
public:
	TestLayer();
	~TestLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	virtual void OnUpdate(float ts) override;
	virtual void OnEvent(Event& event);
private:
	Shared<Scene> m_MainScene;
	CameraController m_CameraController;
	Shared<SpriteSheet> m_PlayerSpriteSheet;
	Shared<Sprite> m_PlayerSprite;
	Entity m_Player;
};