#pragma once

#include "proton/Core/UUID.h"
#include "proton/Graphics/Sprite.h"
#include "proton/Graphics/TilemapSprite.h"
#include "proton/Graphics/Camera.h"
#include "proton/Graphics/Flipbook.h"
#include "proton/Scene/PhysicsMaterial.h"

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
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		float Rotation = { 0.0f };
		glm::vec2 Scale = { 1.0f, 1.0f };
	};

	struct SpriteComponent
	{
		Shared<Sprite> Sprite = nullptr;
		// RGBA, range: 0.0f - 1.0f
		glm::vec4 Color { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct TilemapSpriteComponent
	{
		TilemapSprite TilemapSprite;
		// RGBA, range: 0.0f - 1.0f
		glm::vec4 Color { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	class EntityScript; // forward declaration

	struct ScriptComponent
	{
		struct ScriptData
		{
			EntityScript* ScriptInstance = nullptr;
			std::function<void()> DestroyInstanceFunction;
			bool Initialized = false;
		};

		std::unordered_map<std::string, ScriptData> Scripts;

		template<typename TScriptClass>
		EntityScript* Attach(const std::string& scriptName)
		{
			ScriptData& script = Scripts[scriptName];
			
			script.ScriptInstance = new TScriptClass();
			script.ScriptInstance->RegisterFields();

			script.DestroyInstanceFunction = [&, scriptName]()
			{
				delete (TScriptClass*)script.ScriptInstance;
				script.ScriptInstance = nullptr;
			};

			return script.ScriptInstance;
		}
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
		Shared<Camera> Camera = nullptr;
	};

	struct RigidbodyComponent
	{
		b2BodyType Type = b2_staticBody;
		bool FixedRotation = false;
	};

	struct BoxColliderComponent
	{
		glm::vec2 Size = { 1.0f, 1.0f };
		glm::vec2 Offset = { 0.0f, 0.0f };
		PhysicsMaterial Material;
		bool IsSensor = false;
	};

	struct FlipbookAnimationComponent
	{
		Shared<Flipbook> Flipbook = nullptr;
	};
}