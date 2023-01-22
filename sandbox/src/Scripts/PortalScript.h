#pragma once

class PortalScript : public proton::EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(PortalScript)

	virtual void OnCreate() override;
	virtual void OnRegisterFields() override;
private:
	proton::Shared<proton::SpriteAnimation> m_SpriteAnimation;
	int m_TargetLevel = 1;
};