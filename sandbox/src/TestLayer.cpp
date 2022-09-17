#include "TestLayer.h"

TestLayer::TestLayer()
	: proton::Layer("Testing layer")
{
}

TestLayer::~TestLayer()
{
}

void TestLayer::OnAttach()
{
	m_MainScene = proton::CreateShared<proton::Scene>();
	m_Player = m_MainScene->CreateEntity("Player");
	m_Player.GetComponent<proton::TransformComponent>().Position = { 0.2f, 0.2f, 0.1f };
	m_Player.AddComponent<proton::SpriteComponent>();
}

void TestLayer::OnDetach()
{
}

void TestLayer::OnUpdate(float ts)
{
	m_MainScene->OnUpdate(ts);
}

void TestLayer::OnEvent(proton::Event& event)
{
}
