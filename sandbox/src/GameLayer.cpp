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
	//AssetsManager::LoadSpriteSheet("skeleton-sheet.png", 150, 150);
	AssetsManager::LoadSpriteSheet("player-sheet.png", 120, 80);
	AssetsManager::LoadSpriteSheet("level-sheet-1.png", 32, 32);
	AssetsManager::LoadTexture("box.png");
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
	EventDispatcher dispather(e);
	dispather.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& event)
	{
		if (event.GetKeyCode() == Key::R)
		{
			glm::vec2 cursorPos = m_Scene.GetMouseWorldPosition();
			Entity entity = m_Scene.CreateEntity("Box1");
			entity.AddComponent<RigidbodyComponent>().Type = b2_dynamicBody;
			entity.AddComponent<BoxColliderComponent>();
			auto& sprite = entity.AddComponent<SpriteComponent>();
			auto& transform = entity.GetComponent<TransformComponent>();
			transform.Position = { cursorPos.x, cursorPos.y, 0 };
			transform.Rotation = Random::Float(0.0f, 80.0f);
			sprite.Color.r = Random::Float(0.0f, 1.0f);
			sprite.Color.g = Random::Float(0.0f, 1.0f);
			sprite.Color.b = Random::Float(0.0f, 1.0f);
			sprite.Color.a = 50;
			sprite.Sprite = CreateShared<Sprite>(AssetsManager::GetTexture("box.png"));
			m_Scene.CreateBox2DRuntimeBody(entity);
		}
		return true;
	});

}
