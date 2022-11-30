#include "GameLayer.h"
#include "Scripts/PlayerScript.h"
#include "Scripts/RotationScript.h"

using namespace proton;

void GameLayer::OnCreate()
{
	AssetsManager::LoadSpriteSheet("skeleton-sheet.png", 150, 150);
	AssetsManager::LoadSpriteSheet("player-sheet.png", 120, 80);
	AssetsManager::LoadSpriteSheet("level-sheet-1.png", 32, 32);

	REGISTER_SCRIPT(PlayerScript);
	REGISTER_SCRIPT(RotationScript);

	m_Scene = CreateShared<Scene>("Sample scene");
	m_Scene->CreateEntity("Level");
}

void GameLayer::OnUpdate(float ts)
{
	m_Scene->OnUpdate(ts);
}
