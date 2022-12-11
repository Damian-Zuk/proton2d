#pragma once

#include "proton/Core/UUID.h"
#include "proton/Graphics/Sprite.h"
#include "proton/Graphics/TilemapSprite.h"
#include "proton/Graphics/Camera.h"

#include <entt/entity/entity.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define TILEMAP_BLANK_TILE glm::ivec2{ -1, -1 }

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
		glm::vec3 Position { 0.0f, 0.0f, 0.0f };
		float Rotation { 0.0f };
		glm::vec2 Scale { 1.0f, 1.0f };
	};

	struct SpriteComponent
	{
		Shared<Sprite> Sprite = nullptr;
		glm::vec4 Color { 1.0f };
	};

	struct TilemapSpriteComponent
	{
		TilemapSprite TilemapSprite;
		glm::vec4 Color{ 1.0f };
	};

	class EntityScript; // forward declaration

	struct ScriptComponent
	{
		struct ScriptData
		{
			EntityScript* ScriptInstance = nullptr;
			std::function<void()> CreateInstanceFunction;
			std::function<void()> DestroyInstanceFunction;
		};

		std::unordered_map<std::string, ScriptData> Scripts;

		template<typename T>
		void Bind(const std::string& scriptName)
		{
			ScriptData& script = Scripts[scriptName];

			script.CreateInstanceFunction = [&, scriptName]()
			{
				script.ScriptInstance = new T();
			};

			script.DestroyInstanceFunction = [&, scriptName]()
			{
				delete script.ScriptInstance;
				script.ScriptInstance = nullptr;
			};
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
		Shared<Camera> Camera;
	};
}