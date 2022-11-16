#include "GameLayer.h"
#include "Scripts/PlayerScript.h"
#include "Scripts/RotationScript.h"
#include "proton/Assets/SceneSerializer.h"

using namespace proton;

void GameLayer::OnCreate()
{
	// Create scene
	m_Scene = CreateShared<Scene>("Sample scene");

	// Load assets
	AssetsManager::LoadSpriteSheet("skeleton-sheet.png", 150, 150);
	auto playerSpriteSheet = AssetsManager::LoadSpriteSheet("player-sheet.png", 120, 80);

	// Create game objects
	m_Level = m_Scene->CreateEntity("Level");

	SpawnPlatform({ 3.6f, -1.17f }, 32, 5);
	SpawnPlatform({ -1.8f,  0.0f }, 4, 20);
	SpawnPlatform({ -0.2f,  0.7f }, 1, 1);
	SpawnPlatform({  0.6f,  0.7f }, 2, 1);
	SpawnPlatform({  1.7f,  0.7f }, 3, 1);
	SpawnPlatform({  2.8f,  0.7f }, 2, 2);
	SpawnPlatform({  3.8f,  0.7f }, 3, 3);
	SpawnPlatform({  5.0f,  0.7f }, 4, 4);
	SpawnPlatform({  6.8f,  0.7f }, 5, 5);

	m_Player = m_Scene->CreateEntity("Player");
	{
		auto& transform    = m_Player.GetComponent<TransformComponent>();
		auto& sprite       = m_Player.AddComponent<SpriteComponent>();
		transform.Position = { -0.3f, -0.1f, -0.1f };
		transform.Scale    = { 1.0f, 1.0f };
		sprite.Sprite      = CreateShared<Sprite>(playerSpriteSheet, 0, 0);
		m_Player.AddScript<PlayerScript>("PlayerScript");
	}

	REGISTER_SCRIPT(PlayerScript);
	REGISTER_SCRIPT(RotationScript);
}

void GameLayer::OnUpdate(float ts)
{
	m_Scene->OnUpdate(ts);
}

void GameLayer::OnImGuiRender()
{
	ImGui::Begin("Serializer");
	if (ImGui::Button("Serialize"))
		SceneSerializer::Serialize(m_Scene);
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
	auto levelSheet = AssetsManager::LoadSpriteSheet("level-sheet-1.png", 32, 32);
	
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
			sprite = CreateShared<Sprite>(levelSheet, tileX, tileY);
			
			platform.AddChildEntity(tile);
		}
	}
	m_Level.AddChildEntity(platform);
}
