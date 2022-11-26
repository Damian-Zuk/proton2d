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

void GameLayer::OnImGuiRender()
{
	static int w = 1, h = 1;
	static float x = 1, y = 1;
	static bool edgeLeft = true, edgeRight = true;
	static Entity selectedPlatform;
	ImGui::Begin("Map tools");

	ImGui::DragFloat("X##aP_X", &x, 0.01f, 0.0f, 0.0f, "%.2f");
	ImGui::DragFloat("Y##aP_Y", &y, 0.01f, 0.0f, 0.0f, "%.2f");

	bool changedSize = false;
	if (ImGui::InputInt("Width", &w))
	{
		w = std::max(w, 1); changedSize = true;
	}
	if (ImGui::InputInt("Height", &h))
	{
		h = std::max(h, 1); changedSize = true;
	}

	if (ImGui::Checkbox("Left edge", &edgeLeft))
		changedSize = true;

	if (ImGui::Checkbox("Right edge", &edgeRight))
		changedSize = true;

	if (changedSize && selectedPlatform)
	{
		auto transform = selectedPlatform.GetComponent<TransformComponent>();
		selectedPlatform.Destroy();
		selectedPlatform = SpawnPlatform({ transform.Position.x, transform.Position.y }, w, h, edgeLeft, edgeRight);
	}

	ImGui::Dummy({ 0, 3.0f });

	if (selectedPlatform)
	{
		if (ImGui::Button("Apply##sprawn_platform"))
			selectedPlatform = Entity();
	}
	else
		if (ImGui::Button("Spawn platform"))
			selectedPlatform = SpawnPlatform({ x, y }, w, h, edgeLeft, edgeRight);

	ImGui::End();
}

Entity GameLayer::SpawnPlatform(glm::vec2 pos, uint32_t widthTiles, uint32_t heightTiles, bool edgeLeft, bool edgeRight)
{
	auto platform = m_Scene->CreateEntity("Platform " + std::to_string(widthTiles) + 'x' + std::to_string(heightTiles));
	auto& transform = platform.GetComponent<TransformComponent>();
	transform.Position = { pos.x, pos.y, 0.1f };
	transform.Scale = { 0.3f, 0.3f };

	auto& tilemap = platform.AddComponent<TilemapSpriteComponent>
		(AssetsManager::GetSpriteSheet("level-sheet-1.png"), widthTiles, heightTiles);

	for (uint32_t y = 0; y < heightTiles; y++)
	{
		for (uint32_t x = 0; x < widthTiles; x++)
		{
			uint32_t tileX = 1 + x % 2, tileY = 1 + y % 2;

			// bottom
			if (y == 0)
				tileY = 0;
			// top
			if (y == heightTiles - 1)
				tileY = 3;
			// left
			if (x == 0)
				tileX = 0;
			// right
			if (x == widthTiles - 1)
				tileX = 3;

			if (heightTiles == 1)
				tileY += 2;

			if (widthTiles == 1)
				tileX += 2;

			if (!edgeLeft && x == 0)
			{
				tileX = 1; tileY = 1;
			}

			if (!edgeRight && x == widthTiles - 1)
			{
				tileX = 1; tileY = 1;
			}

			tilemap.Tilemap[x][y] = { tileX, tileY };
		}
	}

	EditorOverlay::SetInspectorContext(platform);
	Entity level = m_Scene->FindByTag("Level");
	if (level)
		level.AddChildEntity(platform);

	return platform;
}