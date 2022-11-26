#pragma once

#include <Proton2D.h>

using namespace proton;

class GameLayer : public AppLayer
{
public:
	virtual void OnCreate() override;
	virtual void OnUpdate(float ts) override;
	virtual void OnImGuiRender() override;

	Entity SpawnPlatform(glm::vec2 pos, uint32_t widthTiles, uint32_t heightTiles, bool edgeLeft, bool edgeRight);

private:
	Shared<Scene> m_Scene;
	Entity m_Player;
	Entity m_Level;
};