#include "TestLayer.h"

using namespace proton;

// Script for testing
class RunningScript : public EntityScript
{
public:
	virtual void OnCreate() override
	{
		GetComponent<SpriteComponent>().Sprite->SetTile(0, 1);
	}

	virtual void OnUpdate(float ts) override
	{
		auto& transform = GetComponent<TransformComponent>();
		transform.Position.x += 0.1f * ts;


		m_SpriteChangeTime -= ts;
		if (m_SpriteChangeTime < 0.0f)
		{
			m_SpriteChangeTime = 0.07f;
			GetComponent<SpriteComponent>().Sprite->NextTile();
		}
	}
private:
	float m_SpriteChangeTime = 0.07f;
};

// Script for testing
class MovingUpScript : public EntityScript
{
public:
	virtual void OnUpdate(float ts) override
	{
		auto& transform = GetComponent<TransformComponent>();
		transform.Position.y += 0.05f * ts;
	}
};

void TestLayer::OnAttach()
{
	REGISTER_SCRIPT(RunningScript);
	REGISTER_SCRIPT(MovingUpScript);

	m_MainScene = CreateShared<Scene>();
	m_MainScene->SetPrimaryCamera(m_CameraController.GetCamera());
	Application::Get().SetEditorActiveScene(m_MainScene);

	AssetsManager::Get().LoadSpriteSheet("skeleton-sheet.png", 150, 150);
	auto playerSpriteSheet = AssetsManager::Get().LoadSpriteSheet("player-sheet.png", 120, 80);
	
	m_Player = m_MainScene->CreateEntity("Player");
	{
		auto& transform = m_Player.GetComponent<TransformComponent>();
		auto& sprite = m_Player.AddComponent<SpriteComponent>();
		transform.Position = { -0.5f, -0.2f, -0.1f };
		transform.Scale = { 1.5f, 1.0f };
		sprite.Sprite = CreateShared<Sprite>(playerSpriteSheet, 0, 0);
	}

	auto redSquare = m_MainScene->CreateEntity("Red Square");
	{
		auto& transform = redSquare.GetComponent<TransformComponent>();
		auto& sprite = redSquare.AddComponent<SpriteComponent>();
		transform.Position = { 0.5f, -0.2f, 0.0f };
		transform.Scale = { 0.2f, 0.2f };
		sprite.Color = { 0.7f, 0.25f, 0.15f, 1.0f };
	}

	auto blueSquare = m_MainScene->CreateEntity("Blue Square");
	{
		auto& transform = blueSquare.GetComponent<TransformComponent>();
		auto& sprite = blueSquare.AddComponent<SpriteComponent>();
		transform.Position = { -0.5f, 0.3f, 0.0f };
		transform.Scale = { 0.2f, 0.2f };
		sprite.Color = { 0.15f, 0.3f, 0.7f, 1.0f };
	}
}

void TestLayer::OnDetach()
{
}

void TestLayer::OnUpdate(float ts)
{
	m_CameraController.OnUpdate(ts);
	m_MainScene->OnUpdate(ts);
	//m_Player.HasScriptComponent<TestScript>();
}

void TestLayer::OnEvent(proton::Event& event)
{
	m_CameraController.OnEvent(event);
}
