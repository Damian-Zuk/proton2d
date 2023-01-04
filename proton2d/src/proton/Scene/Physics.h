#pragma once
#include "proton/Core/UUID.h"

#include <box2d/b2_contact.h>
#include <box2d/b2_world_callbacks.h>
#include <functional>

namespace proton {

	struct PhysicsMaterial
	{
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;
		float Density = 0.5f;
	};

	class Scene;

	class PhysicsContactListener : public b2ContactListener
	{
	public:
		PhysicsContactListener(Scene* scene);

		virtual void BeginContact(b2Contact* contact) override;
		virtual void EndContact(b2Contact* contact) override;
		virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;
		virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override;

	private:
		Scene* m_Scene;
	};

	struct PhysicsContactInfo
	{
		UUID OtherUUID;
		b2Contact* Contact;
	};

	struct PhysicsContactCallback
	{
		std::function<void(PhysicsContactInfo info)> OnBeginContactFunction = nullptr;
		std::function<void(PhysicsContactInfo info)> OnEndContactFunction = nullptr;
		std::function<void(PhysicsContactInfo info, const b2Manifold* oldManifold)> OnPreSolveFunction = nullptr;
		std::function<void(PhysicsContactInfo info, const b2ContactImpulse* impulse)> OnPostSolveFunction = nullptr;
	};

}