#include "GameLayer.h"
#include "Scripts/PlayerScript.h"
#include "Scripts/RotationScript.h"
#include "Scripts/ParallaxBackground.h"

#include <random>

using namespace proton;

void GameLayer::OnCreate()
{
	SceneManager::Load("level_1");
	SceneManager::SetActiveScene("level_1");
}

void GameLayer::OnUpdate(float ts)
{
}

void GameLayer::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);

	Scene* scene = SceneManager::GetActiveScene();
	SceneState state = scene->GetSceneState();

	dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& event)
	{
		if (event.GetKeyCode() == Key::R)
		{
			glm::vec2 cursorPos = scene->GetMouseWorldPosition();
			Entity entity = scene->CreateEntity("Box");
			entity.AddComponent<RigidbodyComponent>().Type = b2_dynamicBody;
			entity.AddComponent<BoxColliderComponent>();
			auto& sprite = entity.AddComponent<SpriteComponent>();
			auto& transform = entity.GetComponent<TransformComponent>();
			transform.Position = { cursorPos.x, cursorPos.y, 0 };
			transform.Rotation = Random::Float(0.0f, 80.0f);
			float scale = Random::Float(1.0f, 1.5f);
			transform.Scale.x = scale;
			transform.Scale.y = scale;
			sprite.Color.r = Random::Float(0.0f, 1.0f);
			sprite.Color.g = Random::Float(0.0f, 1.0f);
			sprite.Color.b = Random::Float(0.0f, 1.0f);
			sprite.Sprite.SetTexture(AssetManager::GetTexture("box.png"));
			if (state == SceneState::Play)
				scene->CreateBox2DRuntimeBody(entity);
		}
		return true;
	});
}
