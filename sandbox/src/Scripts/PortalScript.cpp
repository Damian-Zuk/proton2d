#include "pch.h"
#include "PortalScript.h"
#include <Proton2D.h>

namespace proton {

	void PortalScript::OnRegisterFields()
	{
		RegisterField(ScriptFieldType::Int, "Target Level", &m_TargetLevel);
	}

	void PortalScript::OnCreate()
	{
		m_SpriteAnimation = AddComponent<SpriteAnimationComponent>().SpriteAnimation;
		m_SpriteAnimation->AddAnimation(0, 8);
		m_SpriteAnimation->SetAnimation(0);
		m_SpriteAnimation->SetFPS(10);

		auto& bc = GetComponent<BoxColliderComponent>();
		bc.ContactCallback.OnBeginContactFunction = [&](PhysicsContactInfo contact) {
			if (GetScene()->FindByID(contact.OtherUUID).GetTag() == "Player")
			{
				std::string level = "level_" + std::to_string(m_TargetLevel);
				if (!SceneManager::IsLoaded(level))
					SceneManager::Load(level);
				SceneManager::SetActiveScene(level)->BeginPlay();
			}
		};
	}


}