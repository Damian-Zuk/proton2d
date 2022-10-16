#pragma once

#include <Proton2D.h>

using namespace proton;

class TestLayer : public Layer
{
public:
	virtual void OnAttach() override;
	virtual void OnDetach() override;

	virtual void OnUpdate(float ts) override;
	virtual void OnEvent(Event& event);

	void SpawnPlatform(glm::vec2 pos, uint32_t widthTiles, uint32_t heightTiles);

private:
	Shared<Scene> m_MainScene;
	CameraController m_CameraController;
	Entity m_Player;
};