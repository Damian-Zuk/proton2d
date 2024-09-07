#pragma once

#include "Proton/Core/UUID.h"
#include "Proton/Graphics/Sprite.h"
#include "Proton/Graphics/ResizableSprite.h"
#include "Proton/Graphics/Camera.h"
#include "Proton/Graphics/SpriteAnimation.h"
#include "Proton/Graphics/Renderer/Font.h"
#include "Proton/Physics/PhysicsCommon.h"
#include "Proton/Network/Common.h"
#include "Proton/Network/NetTransform.h"

#include "Proton/UI/UIText.h"
#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <box2d/b2_body.h>

namespace proton {

	struct IDComponent
	{
		UUID ID;
	};

	struct TagComponent
	{
		std::string Tag;
	};

	struct TransformComponent
	{
		glm::vec3 WorldPosition { 0.0f, 0.0f, 0.0f };
		glm::vec3 LocalPosition { 0.0f, 0.0f, 0.0f };
		float Rotation { 0.0f };
		glm::vec2 Scale { 1.0f, 1.0f };
	};

	struct PrefabComponent
	{
		UUID PrefabUUID;
	};

	struct RelationshipComponent
	{
		uint32_t ChildrenCount = 0;
		entt::entity First  { entt::null };
		entt::entity Prev   { entt::null };
		entt::entity Next   { entt::null };
		entt::entity Parent { entt::null };
	};

	struct CameraComponent
	{
		Camera Camera;
		glm::vec2 PositionOffset{ 0.0f, 0.0f };
	};

	struct SpriteComponent
	{
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
		ResizableSprite ResizableSprite;
		// RGBA, range: 0.0f - 1.0f
		glm::vec4 Color { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct CircleRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float Thickness = 1.0f;
		float Fade = 0.005f;
	};

	struct TextComponent
	{
		Shared<Font> FontAsset = Font::GetDefault();

		std::string TextString;
		glm::vec4 Color{ 1.0f };
		float Kerning = 0.0f;
		float LineSpacing = 0.0f;
		bool Hidden = false;
	};

	struct SpriteAnimationComponent
	{
		SpriteAnimation SpriteAnimation;
	};

	class EntityScript; // forward declaration

	struct ScriptComponent
	{
		std::unordered_map<std::string, EntityScript*> Scripts;
	};

	struct RigidbodyComponent
	{
		b2Body* RuntimeBody = nullptr;
		b2BodyType Type = b2_staticBody;
		float LinearDamping = 0.0f;
		float AngularDamping = 0.0f;
		float GravityScale = 1.0f;
		bool IsBullet = false;
		bool FixedRotation = false;
		bool AttachToParent = false; // Revolute Joint
	};

	struct BoxColliderComponent
	{
		b2Fixture* RuntimeFixture = nullptr;
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
		b2Fixture* RuntimeFixture = nullptr;
		glm::vec2 Offset = { 0.0f, 0.0f };
		float Radius = 1.0f;
		PhysicsMaterial Material;
		PhysicsContactCallback ContactCallback;
		bool IsSensor = false;
		bool AttachToParent = false;
	};

	struct NetworkComponent
	{
		bool SimulateOnClient = true;
		NetTransform NetTransform;
			
		struct ReplicatedScript
		{
			struct ReplicatedField
			{
				void* Data = nullptr;
				size_t Size = 0;
				std::function<void()> NotifyFunction;
				// Server-only (todo: store in serparate EnTT component)
				std::unordered_map<ClientID, uint32_t> ClientToChecksumMap;
			};
			EntityScript* Script = nullptr;
			std::vector<ReplicatedField> ReplicatedFields;
		};
		std::vector<ReplicatedScript> ReplicatedScripts;

		bool WasReplicated = false;

		// Server-only (todo: store in separate EnTT component)
		std::unordered_map<ClientID, NetTransform::SequencedValue> ServerDataMap;
	};

}
