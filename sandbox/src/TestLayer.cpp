#include "TestLayer.h"

using namespace proton;

TestLayer::TestLayer()
	: Layer("Testing layer")
{
}

TestLayer::~TestLayer()
{
}

void TestLayer::OnAttach()
{
	m_MainScene = CreateShared<Scene>();
	m_MainScene->SetPrimaryCamera(m_CameraController.GetCamera());
	Application::Get().SetEditorActiveScene(m_MainScene);

	m_PlayerTexture = CreateShared<Texture>("assets/Player.png");
	
	m_Player = m_MainScene->CreateEntity("Player");
	{
		auto& transform = m_Player.GetComponent<TransformComponent>();
		auto& sprite = m_Player.AddComponent<SpriteComponent>();
		transform.Scale = { 0.5f, 0.5f };
		sprite.Texture = m_PlayerTexture;
	}

	auto& redSquare = m_MainScene->CreateEntity("Red Square");
	{
		auto& transform = redSquare.GetComponent<TransformComponent>();
		auto& sprite = redSquare.AddComponent<SpriteComponent>();
		transform.Position = { 0.4f, -0.2f, 0.0f };
		transform.Scale = { 0.2f, 0.2f };
		sprite.Color = { 0.8f, 0.2f, 0.1f, 1.0f };
	}

	auto& blueSquare = m_MainScene->CreateEntity("Blue Square");
	{
		auto& transform = blueSquare.GetComponent<TransformComponent>();
		auto& sprite = blueSquare.AddComponent<SpriteComponent>();
		transform.Position = { -0.4f, 0.3f, 0.0f };
		transform.Scale = { 0.2f, 0.2f };
		sprite.Color = { 0.1f, 0.2f, 0.8f, 1.0f };
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
