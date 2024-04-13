#pragma once

#include "Proton/Core/UUID.h"
#include "Proton/Graphics/Sprite.h"
#include "Proton/Graphics/ResizableSprite.h"
#include "Proton/Graphics/Camera.h"
#include "Proton/Graphics/SpriteAnimation.h"
#include "Proton/Physics/PhysicsCommon.h"

#include <entt/entity/entity.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <box2d/b2_body.h>

namespace proton {

	enum class ComponentTypeID : size_t
	{
		ID =              1,
		Tag =             2,
		Transform =       3,
		Relationship =    4,
		Metadata =        5,
		Script =          6,
		Camera =          7,
		Sprite =          32,
		ResizableSprite = 33,
	 	SpriteAnimation = 34,
		CircleRenderer =  35,
		Rigidbody =       64,
		BoxCollider =     65,
		CircleCollider =  66,
		Network =         128
	};

#define PT_COMPONENT_TYPE_ID(component_type) \
	static constexpr size_t TypeID() { return (size_t)ComponentTypeID::component_type; }

	struct IDComponent
	{
		PT_COMPONENT_TYPE_ID(ID)

		UUID ID;
	};

	struct TagComponent
	{
		PT_COMPONENT_TYPE_ID(Tag)

		std::string Tag;
	};

	struct MetadataComponent
	{
		PT_COMPONENT_TYPE_ID(Metadata)

		// TODO: Implement
	};

	// Use Entity::SetWorldPosition to modify world position manually
	struct TransformComponent
	{
		PT_COMPONENT_TYPE_ID(Transform)

		glm::vec3 WorldPosition { 0.0f, 0.0f, 0.0f };
		glm::vec3 LocalPosition { 0.0f, 0.0f, 0.0f };
		float Rotation { 0.0f };
		glm::vec2 Scale { 1.0f, 1.0f };
	};

	struct RelationshipComponent
	{
		PT_COMPONENT_TYPE_ID(Relationship)

		uint32_t ChildrenCount = 0;
		entt::entity First  { entt::null };
		entt::entity Prev   { entt::null };
		entt::entity Next   { entt::null };
		entt::entity Parent { entt::null };
	};

	struct CameraComponent
	{
		PT_COMPONENT_TYPE_ID(Camera)

		Camera Camera;
		glm::vec2 PositionOffset{ 0.0f, 0.0f };
	};

	struct SpriteComponent
	{
		PT_COMPONENT_TYPE_ID(Sprite)

		SpriteComponent(const std::string& filepath = std::string())
		{
			if (filepath.size())
				Sprite.SetTextureFromPath(filepath);
		}

		Sprite Sprite;
		// RGBA, range: 0.0f - 1.0f
		glm::vec4 Color { 1.0f, 1.0f, 1.0f, 1.0f };
		float TilingFactor = 1.0f;
	};

	struct ResizableSpriteComponent
	{
		PT_COMPONENT_TYPE_ID(ResizableSprite)

		ResizableSprite ResizableSprite;
		// RGBA, range: 0.0f - 1.0f
		glm::vec4 Color { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct CircleRendererComponent
	{
		PT_COMPONENT_TYPE_ID(CircleRenderer)

		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float Thickness = 1.0f;
		float Fade = 0.005f;
	};

	struct SpriteAnimationComponent
	{
		PT_COMPONENT_TYPE_ID(SpriteAnimation)

		SpriteAnimation SpriteAnimation;
	};

	class EntityScript; // forward declaration

	struct ScriptComponent
	{
		PT_COMPONENT_TYPE_ID(Script)

		std::unordered_map<std::string, EntityScript*> Scripts;
	};

	struct RigidbodyComponent
	{
		PT_COMPONENT_TYPE_ID(Rigidbody)

		b2Body* RuntimeBody = nullptr;
		b2BodyType Type = b2_staticBody;
		bool FixedRotation = false;
	};

	struct BoxColliderComponent
	{
		PT_COMPONENT_TYPE_ID(BoxCollider)

		glm::vec2 Size { 1.0f, 1.0f };
		glm::vec2 Offset { 0.0f, 0.0f };
		PhysicsMaterial Material;
		PhysicsContactCallback ContactCallback;
		b2Filter Filter;
		bool IsSensor = false;
	};

	struct CircleColliderComponent
	{
		PT_COMPONENT_TYPE_ID(CircleCollider)

		glm::vec2 Offset = { 0.0f, 0.0f };
		float Radius = 1.0f;
		PhysicsMaterial Material;
		PhysicsContactCallback ContactCallback;
		bool IsSensor = false;
	};

	struct NetworkComponent
	{
		PT_COMPONENT_TYPE_ID(Network)

		ComponentBitset ComponentBitset;
		//std::vector<ReplicatedScriptField> ReplicatedScriptFields;

		bool IsReplicated(ComponentTypeID typeID) const
		{
			return ComponentBitset.test((size_t)typeID);
		}
	};

}
