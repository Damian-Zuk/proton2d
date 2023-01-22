#pragma once
#include "proton/Scene/EntityScript.h"
#include "proton/Graphics/SpriteAnimation.h"

namespace proton {

	class PortalScript : public EntityScript
	{
	public:
		ENTITY_SCRIPT_CLASS(PortalScript)

		virtual void OnCreate() override;
		virtual void OnRegisterFields() override;
	private:
		Shared<SpriteAnimation> m_SpriteAnimation;
		int m_TargetLevel = 1;
	};
}