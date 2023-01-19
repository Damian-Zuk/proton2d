#include "GameLayer.h"
#include "Scripts/PlayerScript.h"
#include "Scripts/RotationScript.h"
#include "Scripts/ParallaxBackground.h"

#include <random>

using namespace proton;

void GameLayer::OnCreate()
{

}

void GameLayer::OnUpdate(float ts)
{
	// Primary camera zooming
	Scene* scene = SceneManager::GetActiveScene();
	if (scene->GetSceneState() == SceneState::Play)
	{
		auto& camera = scene->GetPrimaryCamera();
		float zoomLevel = camera->GetZoomLevel();
		float zoomTargetDiff = glm::abs(m_ZoomLevelTarget - zoomLevel);
		float zoomOffset = glm::max(glm::round(zoomTargetDiff * ts * 10000.0f) / 1000.0f, 0.001f);

		if (m_ZoomLevelTarget > zoomLevel)
			camera->SetZoomLevel(glm::min(zoomLevel + zoomOffset, m_ZoomLevelTarget));

		else if (m_ZoomLevelTarget < zoomLevel)
			camera->SetZoomLevel(glm::max(zoomLevel - zoomOffset, m_ZoomLevelTarget));
	}
}

void GameLayer::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);

	Scene* scene = SceneManager::GetActiveScene();
	if (scene->GetSceneState() == SceneState::Play)
	{
		dispatcher.Dispatch<MouseScrolledEvent>([&](MouseScrolledEvent& event) -> bool
		{
			float zoomOffset = m_CameraZoomSpeed * -event.GetYOffset();
			m_ZoomLevelTarget += round(zoomOffset * round(m_ZoomLevelTarget * 10.0f) * 1000.0f) / 10000.0f;
			m_ZoomLevelTarget = glm::min(glm::max(m_ZoomLevelTarget, 0.2f), 10.0f);
			return true;
		});
	}

	dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& event)
		{
			if (event.GetKeyCode() == Key::R)
			{
				Entity e = PrefabManager::SpawnPrefab(scene, "Block");
				auto& t = e.GetTransform();
				auto m = scene->GetMouseWorldPosition();
				t.Position.x = m.x;
				t.Position.y = m.y;
			}

			if (event.GetKeyCode() == Key::T)
			{
				Entity e = PrefabManager::SpawnPrefab(scene, "Box");
				auto& t = e.GetTransform();
				auto m = scene->GetMouseWorldPosition();
				t.Position.x = m.x;
				t.Position.y = m.y;
			}
			//if (event.GetKeyCode() == Key::R)
			//{
			//	glm::vec2 cursorPos = scene->GetMouseWorldPosition();
			//	Entity entity = scene->CreateEntity("Box1");
			//	entity.AddComponent<RigidbodyComponent>().Type = b2_dynamicBody;
			//	entity.AddComponent<BoxColliderComponent>();
			//	auto& sprite = entity.AddComponent<SpriteComponent>();
			//	auto& transform = entity.GetComponent<TransformComponent>();
			//	transform.Position = { cursorPos.x, cursorPos.y, 0 };
			//	transform.Rotation = Random::Float(0.0f, 80.0f);
			//	sprite.Color.r = Random::Float(0.0f, 1.0f);
			//	sprite.Color.g = Random::Float(0.0f, 1.0f);
			//	sprite.Color.b = Random::Float(0.0f, 1.0f);
			//	sprite.Color.a = 50;
			//	sprite.Sprite = CreateShared<Sprite>(AssetsManager::GetTexture("box.png"));
			//	scene->CreateBox2DRuntimeBody(entity);
			//}
	return true;
		});
}
