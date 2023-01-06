#pragma once

#include "proton/Scene/EntityScript.h"

using namespace proton;

class ParallaxBackground : public EntityScript
{
	ENTITY_SCRIPT_CLASS(ParallaxBackground)
public:
	virtual void RegisterFields() override;
	virtual void OnCreate() override;
	virtual void OnUpdate(float ts) override;
private:
	float m_ParallaxFactor = 1.0f;
};