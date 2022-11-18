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
}

void GameLayer::OnUpdate(float ts)
{
	m_Scene->OnUpdate(ts);
}

void GameLayer::OnImGuiRender()
{
	static int w = 1, h = 1;
	ImGui::Begin("Game tools");

	if (ImGui::InputInt("Width", &w))
		w = std::max(w, 1);

	if (ImGui::InputInt("Height", &h))
		h = std::max(h, 1);

	if (ImGui::Button("Spawn platform"))
		SpawnPlatform({ 0, 0 }, w, h);

	ImGui::End();
}

void GameLayer::SpawnPlatform(glm::vec2 pos, uint32_t widthTiles, uint32_t heightTiles)
{
	auto platform = m_Scene->CreateEntity("Platform " + std::to_string(widthTiles) + 'x' + std::to_string(heightTiles));
	{
		auto& transform = platform.GetComponent<TransformComponent>();
		transform.Position = { pos.x, pos.y, 0.1f };
		transform.Scale = { 1.0f, 1.0f };
	}
	
	if (heightTiles == 1)
		widthTiles++;
	
	for (uint32_t y = 1; y <= heightTiles; y++)
	{
		for (uint32_t x = 1; x <= widthTiles; x++)
		{
			uint32_t tileX = 1 + rand() % 2, tileY = 1 + rand() % 2;

			// bottom
			if (y == 1)
				tileY = 0;
			// top
			if (y == heightTiles)
				tileY = 3;
			// left
			if (x == 1)
				tileX = 0;
			// right
			if (x == widthTiles)
				tileX = 3;

			if (heightTiles == 1)
				tileX += 4;

			auto tile = m_Scene->CreateEntity("Terrain tile");
			auto& transform = tile.GetComponent<TransformComponent>();
			transform.Scale = { 0.3f, 0.3f };
			transform.Position = { 
				((float)x - (float)widthTiles  / 2 - 0.5f) * transform.Scale.x,
				((float)y - (float)heightTiles / 2 - 0.5f) * transform.Scale.y,
				0.1f 
			};
			auto& sprite = tile.AddComponent<SpriteComponent>().Sprite;
			sprite = CreateShared<Sprite>(AssetsManager::GetSpriteSheet("level-sheet-1.png"), tileX, tileY);
			
			platform.AddChildEntity(tile);
		}
	}
	m_Level.AddChildEntity(platform);
}
