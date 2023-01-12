#pragma once

#include <proton/Scene/EntityScript.h>
#include <proton/Graphics/Flipbook.h>
#include <proton/Core/UUID.h>

#include <box2d/include/box2d/b2_body.h>

using namespace proton;

enum PlayerDirection : bool
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

	virtual void OnRegisterFields() override;
	virtual void OnCreate() override;
	virtual void OnUpdate(float ts) override;

private:
	Shared<Flipbook> m_Flipbook;
	PlayerDirection m_Direction = Right;

	float m_PlayerSpeed = 5.0f;
	float m_JumpForce = 5.0f;
	bool m_IsAttacking = false;

	// Field serialization test
	//glm::vec2 test1;
	//glm::vec3 test2;
	//glm::vec4 test3;
	//glm::vec2 test4;
	//glm::vec3 test5;
	//glm::vec4 test6;
	//bool test7;

	Entity m_FootSensor;
	uint32_t m_ContactCount = 0;
	b2Body* m_Body = nullptr;
};