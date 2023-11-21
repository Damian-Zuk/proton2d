#include "pcheader.h"
#include "GameLayer.h"

// Header-only scripts must be compiled somewhere
#include "Scripts/RotationScript.h" 
#include "Scripts/TestScript.h"


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

#if PROTON_EDITOR
		if (ImGui::GetIO().WantCaptureKeyboard)
			return true;
#endif

		if (event.GetKeyCode() == Key::R)
		{
			
			Entity entity = scene->CreateEntity("Random Box");
			entity.AddComponent<RigidbodyComponent>().Type = b2_dynamicBody;
			entity.AddComponent<BoxColliderComponent>();
			
			glm::vec2 cursorPos = scene->GetCursorWorldPosition();
			auto& transform = entity.GetComponent<TransformComponent>();
			transform.Position = { cursorPos.x, cursorPos.y, 0 };
			transform.Rotation = Random::Float(0.0f, 80.0f);
			float scale = Random::Float(1.0f, 1.5f);
			transform.Scale = { scale, scale };

			auto& sprite = entity.AddComponent<SpriteComponent>();
			sprite.Color.r = Random::Float(0.0f, 1.0f);
			sprite.Color.g = Random::Float(0.0f, 1.0f);
			sprite.Color.b = Random::Float(0.0f, 1.0f);
			sprite.Sprite.SetTexture(AssetManager::GetTexture("box.png"));
		}

		if (event.GetKeyCode() == Key::L)
		{
			for (auto entity : scene->FindAllByTag("Small platform"))
			{
				entity.GetComponent<TagComponent>().Tag = "Platform";
			}
		}

		return true;
	});
}
