#pragma once

#include "proton/Scene/EntityScript.h"

using namespace proton;

class ParallaxBackground : public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(ParallaxBackground)

	virtual void OnRegisterFields() override;
	virtual void OnCreate() override;
	virtual void OnUpdate(float ts) override;
private:
	float m_ParallaxFactor = 1.0f;
	float m_TilingFactor = 3.0f;
};
