#include "TestLayer.h"
#include "Scripts/PlayerScript.h"
#include "Scripts/MoveUp.h"
#include "Scripts/RotateScript.h"

using namespace proton;

void TestLayer::OnAttach()
{
	LOAD_SCRIPT(PlayerScript);
	LOAD_SCRIPT(MoveUp);
	LOAD_SCRIPT(RotateScript);

	// Create scene
	m_MainScene = CreateShared<Scene>();
	m_MainScene->SetPrimaryCamera(m_CameraController.GetCamera());
	Application::Get().SetEditorActiveScene(m_MainScene);

	// Load assets
	AssetsManager::Get().LoadSpriteSheet("skeleton-sheet.png", 150, 150);
	auto playerSpriteSheet = AssetsManager::Get().LoadSpriteSheet("player-sheet.png", 120, 80);
	
	// Create game objects
	SpawnPlatform({ 1.0f, -1.0f }, 32, 5);
	SpawnPlatform({ -2.0f, 0.0f }, 4, 20);
	SpawnPlatform({ -0.8f, 0.3f }, 1, 1);
	SpawnPlatform({ -0.2f, 0.3f }, 2, 1);
	SpawnPlatform({ 0.4f, 0.3f }, 2, 2);
	SpawnPlatform({ 1.0f, 0.3f }, 3, 3);
	SpawnPlatform({ 1.8f, 0.3f }, 4, 4);
	SpawnPlatform({ 2.7f, 0.3f }, 5, 5);

	auto box = m_MainScene->CreateEntity("Box");
	{
		auto& transform    = box.GetComponent<TransformComponent>();
		transform.Position = { 0.5f, -0.52f, 0.0f };
		transform.Scale    = { 0.2f, 0.2f };
		auto& sprite       = box.AddComponent<SpriteComponent>();
		sprite.Sprite      = CreateShared<Sprite>(AssetsManager::Get().GetTexture("box.png"));
	}

	m_Player = m_MainScene->CreateEntity("Player");
	{
		auto& transform    = m_Player.GetComponent<TransformComponent>();
		transform.Position = { -0.3f, -0.13f, -0.1f };
		transform.Scale    = { 1.5f, 1.0f };
		auto& sprite       = m_Player.AddComponent<SpriteComponent>();
		sprite.Sprite      = CreateShared<Sprite>(playerSpriteSheet, 0, 0);
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

// TODO: create SpriteGridFlexBlock in engine
void TestLayer::SpawnPlatform(glm::vec2 pos, uint32_t widthTiles, uint32_t heightTiles)
{
	auto platform = m_MainScene->CreateEntity("Platform " + std::to_string(widthTiles) + 'x' + std::to_string(heightTiles));
	auto levelSheet = AssetsManager::Get().LoadSpriteSheet("level-sheet-1.png", 32, 32);
	
	if (heightTiles == 1)
		widthTiles++; // edge blocks are half blocks
	
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

			auto tile = m_MainScene->CreateEntity("Terrain tile");
			auto& transform = tile.GetComponent<TransformComponent>();
			glm::vec2 scale = { 0.15f, 0.15f };
			transform.Scale = scale;
			transform.Position = glm::vec3{ pos.x - scale.x * widthTiles / 2, pos.y - scale.y * heightTiles / 2 , 0.1f };
			transform.Position += glm::vec3{ x * scale.x, y * scale.y, 0.1f }; // tile offset
			auto& sprite = tile.AddComponent<SpriteComponent>();
			sprite.Sprite = CreateShared<Sprite>(levelSheet, tileX, tileY);
			platform.AddChildEntity(tile);
		}

	}
}
