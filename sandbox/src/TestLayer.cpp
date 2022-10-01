#include "TestLayer.h"
#include "Scripts/PlayerRunRight.h"
#include "Scripts/MoveUp.h"

using namespace proton;

void TestLayer::OnAttach()
{
	LOAD_SCRIPT(PlayerRunRight);
	LOAD_SCRIPT(MoveUp);

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
