#pragma once

#include "Proton/Core/UUID.h"
#include "Proton/Graphics/Sprite.h"
#include "Proton/Graphics/ResizableSprite.h"
#include "Proton/Graphics/Camera.h"
#include "Proton/Graphics/SpriteAnimation.h"
#include "Proton/Graphics/Renderer/Font.h"
#include "Proton/Physics/PhysicsCommon.h"
#include "Proton/Network/Common.h"
#include "Proton/Network/NetSyncData.h"

#include "Proton/UI/UIText.h"
#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <box2d/b2_body.h>

namespace proton {

	constexpr uint32_t MAX_COMPONENTS = 128;

	enum EComponentType : size_t
	{
		ComponentType_ID =              1,
		ComponentType_Tag =             2,
		ComponentType_Transform =       3,
		ComponentType_Relationship =    4,
		ComponentType_Metadata =        5,
		ComponentType_Script =          6,
		ComponentType_Camera =          7,
		ComponentType_Velocity =        8,
		ComponentType_Sprite =          32,
		ComponentType_ResizableSprite = 33,
	 	ComponentType_SpriteAnimation = 34,
		ComponentType_CircleRenderer =  35,
		ComponentType_Text =            36,
		ComponentType_UI =              48,
		ComponentType_UIText =          49,
		ComponentType_Rigidbody =       64,
		ComponentType_BoxCollider =     65,
		ComponentType_CircleCollider =  66,
		ComponentType_Network =         128
	};

#define PROTON_COMPONENT_TYPE_ID(component_type) \
	static constexpr size_t TypeID() { return component_type; }

	struct IDComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_ID)

		UUID ID;
	};

	struct TagComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_Tag)

		std::string Tag;
	};

	struct MetadataComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_Metadata)
		// TODO: Implement
	};

	struct TransformComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_Transform)

		glm::vec3 WorldPosition { 0.0f, 0.0f, 0.0f };
		glm::vec3 LocalPosition { 0.0f, 0.0f, 0.0f };
		float Rotation { 0.0f };
		glm::vec2 Scale { 1.0f, 1.0f };
	};

	struct VelocityComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_Velocity)

		glm::vec2 LinearVelocity { 0.0f };
		float AngularVelocity = 0.0f;
	};

	struct RelationshipComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_Relationship)

		uint32_t ChildrenCount = 0;
		entt::entity First  { entt::null };
		entt::entity Prev   { entt::null };
		entt::entity Next   { entt::null };
		entt::entity Parent { entt::null };
	};

	struct CameraComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_Camera)

		Camera Camera;
		glm::vec2 PositionOffset{ 0.0f, 0.0f };
	};

	struct SpriteComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_Sprite)

		SpriteComponent() = default;
		SpriteComponent(const std::string& filepath)
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
		PROTON_COMPONENT_TYPE_ID(ComponentType_ResizableSprite)

		ResizableSprite ResizableSprite;
		// RGBA, range: 0.0f - 1.0f
		glm::vec4 Color { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct CircleRendererComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_CircleRenderer)

		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float Thickness = 1.0f;
		float Fade = 0.005f;
	};

	struct TextComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_Text)

		Shared<Font> FontAsset = Font::GetDefault();

		std::string TextString;
		glm::vec4 Color{ 1.0f };
		float Kerning = 0.0f;
		float LineSpacing = 0.0f;
		bool Hidden = false;
	};

	struct UIComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_UI)

		UIElement* Element;
	};

	struct UITextComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_UIText)

		UIText UIText;
	};

	struct SpriteAnimationComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_SpriteAnimation)

		SpriteAnimation SpriteAnimation;
	};

	class EntityScript; // forward declaration

	struct ScriptComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_Script)

		std::unordered_map<std::string, EntityScript*> Scripts;
	};

	struct RigidbodyComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_Rigidbody)

		b2Body* RuntimeBody = nullptr;
		b2BodyType Type = b2_staticBody;
		bool FixedRotation = false;
		bool AttachToParent = false; // Revolution Joint
	};

	struct BoxColliderComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_BoxCollider)

		glm::vec2 Size { 1.0f, 1.0f };
		glm::vec2 Offset { 0.0f, 0.0f };
		PhysicsMaterial Material;
		PhysicsContactCallback ContactCallback;
		b2Filter Filter;
		bool IsSensor = false;
		bool AttachToParent = false;
	};

	struct CircleColliderComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_CircleCollider)

		glm::vec2 Offset = { 0.0f, 0.0f };
		float Radius = 1.0f;
		PhysicsMaterial Material;
		PhysicsContactCallback ContactCallback;
		bool IsSensor = false;
		bool AttachToParent = false;
	};

	struct NetworkComponent
	{
		PROTON_COMPONENT_TYPE_ID(ComponentType_Network)

		float UpdateRate = 1.0f; 
		std::bitset<MAX_COMPONENTS> ComponentsToReplicate;
		std::unordered_map<EComponentType, uint32_t> ComponentChecksum;

		NetSyncParams SyncParams;
		NetSyncState SyncState;
		NetTransform PreviousTransform;
		NetTransform CurrentTransform;

		struct ReplicatedScript
		{
			struct ReplicatedField
			{
				void* Data = nullptr;
				uint64_t Size = 0;
				uint32_t Checksum = 0;
				std::function<void(Entity*)> NotifyFunction;
			};
			EntityScript* Script = nullptr;
			std::vector<ReplicatedField> ReplicatedFields;
		};
		std::vector<ReplicatedScript> ReplicatedScripts;
	};

}
