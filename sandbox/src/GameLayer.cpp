#include "pcheader.h"
#include "GameLayer.h"
#include "Scripts/RotationScript.h" // script with only header file must be compiled somewhere

using namespace proton;

void GameLayer::OnCreate()
{
#if PROTON_DISTRIBUTION
	SceneManager::Load("level_1");
	SceneManager::SetActiveScene("level_1")->BeginPlay();
#else
	SceneManager::Load("level_1");
	SceneManager::SetActiveScene("level_1");
#endif
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
			Entity entity = scene->CreateEntity("Random Box");
			entity.AddComponent<RigidbodyComponent>().Type = b2_dynamicBody;
			entity.AddComponent<BoxColliderComponent>();
			auto& sprite = entity.AddComponent<SpriteComponent>();
			auto& transform = entity.GetComponent<TransformComponent>();
			transform.Position = { cursorPos.x, cursorPos.y, 0 };
			transform.Rotation = Random::Float(0.0f, 80.0f);
			float scale = Random::Float(1.0f, 1.5f);
			transform.Scale = { scale, scale };
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
