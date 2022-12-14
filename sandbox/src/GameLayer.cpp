#include "GameLayer.h"
#include "Scripts/PlayerScript.h"
#include "Scripts/RotationScript.h"

#include <random>

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
	AssetsManager::LoadTexture("box.png");

	REGISTER_SCRIPT(PlayerScript);
	REGISTER_SCRIPT(RotationScript);
}

void GameLayer::OnUpdate(float ts)
{
	m_Scene.OnUpdate(ts);

	for (Entity entity : m_Scene.FindAllByTag("Box1"))
	{
		auto& transform = entity.GetComponent<TransformComponent>();
		if (transform.Position.y < -60.0f)
			entity.Destroy();
	}
}

void GameLayer::OnEvent(Event& e)
{
	static std::random_device rd;
	static std::mt19937 eng(rd());
	static std::uniform_real_distribution<> dist1080(10, 80);
	static std::uniform_real_distribution<> dist01(0, 1);

	EventDispatcher dispather(e);
	dispather.Dispatch< KeyPressedEvent>([&](KeyPressedEvent& event) 
	{
		if (event.GetKeyCode() == Key::R)
		{
			glm::vec2 cursorPos = m_Scene.GetMouseWorldPosition();
			Entity entity = m_Scene.CreateEntity("Box1");
			entity.AddComponent<RigidBodyComponent>().Type = b2_dynamicBody;
			entity.AddComponent<BoxColliderComponent>();
			auto& sprite = entity.AddComponent<SpriteComponent>();
			auto& transform = entity.GetComponent<TransformComponent>();
			transform.Position = { cursorPos.x, cursorPos.y, 0 };
			transform.Rotation = (float)dist1080(eng);
			sprite.Color.r = (float)dist01(eng);
			sprite.Color.g = (float)dist01(eng);
			sprite.Color.b = (float)dist01(eng);
			sprite.Color.a = 50;
			sprite.Sprite = CreateShared<Sprite>(AssetsManager::GetTexture("box.png"));
			m_Scene.CreateBox2DRuntimeBody(entity);
		}
		return true;
	});
}
