#include "TestLayer.h"
#include "Scripts/PlayerScript.h"
#include "Scripts/MoveUp.h"

using namespace proton;

void TestLayer::OnAttach()
{
	LOAD_SCRIPT(PlayerScript);
	LOAD_SCRIPT(MoveUp);

	// Create scene
	m_MainScene = CreateShared<Scene>();
	m_MainScene->SetPrimaryCamera(m_CameraController.GetCamera());
	Application::Get().SetEditorActiveScene(m_MainScene);

	// Load assets
	AssetsManager::Get().LoadSpriteSheet("skeleton-sheet.png", 150, 150);
	auto playerSpriteSheet = AssetsManager::Get().LoadSpriteSheet("player-sheet.png", 120, 80);
	
	// Create game objects

	SpawnPlatform({ 0, -0.75f, 0.1f }, 24, 8);
	SpawnPlatform({ 2.2f, 0, 0.1f }, 30, 10);

	m_Player = m_MainScene->CreateEntity("Player");
	{
		auto& transform    = m_Player.GetComponent<TransformComponent>();
		transform.Position = { -0.5f, 0.0f, -0.1f };
		transform.Scale    = { 1.5f, 1.0f };
		auto& sprite       = m_Player.AddComponent<SpriteComponent>();
		sprite.Sprite      = CreateShared<Sprite>(playerSpriteSheet, 0, 0);
	}

	auto box = m_MainScene->CreateEntity("Box");
	{
		auto& transform    = box.GetComponent<TransformComponent>();
		transform.Position = { 0.5f, -0.35f, 0.0f };
		transform.Scale    = { 0.2f, 0.2f };
		auto& sprite       = box.AddComponent<SpriteComponent>();
		sprite.Sprite      = CreateShared<Sprite>(AssetsManager::Get().GetTexture("box.png"));
	}
}

void TestLayer::OnDetach()
{
}

void TestLayer::OnUpdate(float ts)
{
	m_CameraController.OnUpdate(ts);
	m_MainScene->OnUpdate(ts);
}

void TestLayer::OnEvent(proton::Event& event)
{
	m_CameraController.OnEvent(event);
}

void TestLayer::SpawnPlatform(glm::vec3 pos, uint32_t widthTiles, uint32_t heightTiles)
{
	auto levelSheet = AssetsManager::Get().LoadSpriteSheet("level-build-1.png", 32, 32);

	for (uint32_t y = 1; y <= heightTiles; y++)
	{
		for (uint32_t x = 1; x <= widthTiles; x++)
		{
			uint32_t tileX = (rand() % 2) + 1, tileY = (rand() % 2) + 6;

			// bottom
			if (y == 1)
				tileY = 5;
			// top
			if (y == heightTiles)
				tileY = 8;
			// left
			if (x == 1)
				tileX = 0;
			// right
			if (x == widthTiles)
				tileX = 3;

			auto tile = m_MainScene->CreateEntity("Terrain tile");
			auto& transform = tile.GetComponent<TransformComponent>();
			glm::vec2 scale = { 0.07f, 0.07f };
			transform.Scale = scale;
			transform.Position = pos - glm::vec3{ scale.x * widthTiles / 2, scale.y * heightTiles / 2 , 0.1f };
			transform.Position += glm::vec3{ x * scale.x, y * scale.y, 0.1f };
			auto& sprite = tile.AddComponent<SpriteComponent>();
			sprite.Sprite = CreateShared<Sprite>(levelSheet, tileX, tileY);
		}

	}
}
