#pragma once

#include "proton/Scene/EntityScript.h"

class ParallaxBackground : public proton::EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(ParallaxBackground)

	virtual void OnRegisterFields() override;
	virtual void OnCreate() override;
	virtual void OnUpdate(float ts) override;
private:
	float m_ParallaxFactor = 1.0f;
	float m_PositionOffset = 0.0f;

	float m_SpriteAspectRatio = 1.0f;
	uint32_t m_CopiesCount = 3;
	proton::TransformComponent* m_Transform;
	std::vector<proton::TransformComponent*> m_Copies;
	proton::Entity m_FirstCopy;
};
