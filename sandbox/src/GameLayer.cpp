#include "GameLayer.h"
#include "Scripts/PlayerScript.h"
#include "Scripts/RotationScript.h"

using namespace proton;

GameLayer::GameLayer()
	: m_Scene("Sample scene")
{
}

void GameLayer::OnCreate()
{
	AssetsManager::LoadSpriteSheet("skeleton-sheet.png", 150, 150);
	AssetsManager::LoadSpriteSheet("player-sheet.png", 120, 80);
	AssetsManager::LoadSpriteSheet("level-sheet-1.png", 32, 32);

	REGISTER_SCRIPT(PlayerScript);
	REGISTER_SCRIPT(RotationScript);
}

void GameLayer::OnUpdate(float ts)
{
	m_Scene.OnUpdate(ts);
}
