#pragma once

#include <proton/Scripting.h>
#include <proton/Graphics/Flipbook.h>

#include <box2d/include/box2d/b2_body.h>

using namespace proton;

enum PlayerOrientation : bool
{
	Right = 0, Left = 1
};

enum PlayerAnimation : uint32_t
{
	Idle = 0, Run, Attack, Jump
};

class PlayerScript: public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(PlayerScript)

	virtual void RegisterFields() override;
	virtual void OnCreate() override;
	virtual void OnUpdate(float ts) override;

private:
	Shared<Flipbook> m_Flipbook;
	PlayerOrientation m_Orientation = Right;

	float m_PlayerSpeed = 5.0f;
	float m_JumpForce = 5.0f;
	bool m_IsAttacking = false;

	// Field serialization test
	glm::vec2 test1;
	glm::vec3 test2;
	glm::vec4 test3;
	glm::vec2 test4;
	glm::vec3 test5;
	glm::vec4 test6;
	bool test7;

	b2Body* m_Body = nullptr;
	b2Body* m_FootSensor = nullptr;
};