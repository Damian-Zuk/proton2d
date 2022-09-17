#pragma once
#include <Proton2D.h>

class TestLayer : public proton::Layer
{
public:
	TestLayer();
	~TestLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	virtual void OnUpdate(float ts) override;
	virtual void OnEvent(proton::Event& event);
private:
	proton::Shared<proton::Scene> m_MainScene;
	proton::Entity m_Player;
};