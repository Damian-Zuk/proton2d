#pragma once

#include "proton/Graphics/Sprite.h"
#include "proton/Graphics/Camera.h"

#include <entt/entity/entity.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace proton {

	struct TagComponent
	{
		std::string Tag;
	};

	struct TransformComponent
	{
		glm::vec3 Position { 0.0f, 0.0f, 0.0f };
		float Rotation { 0.0f };
		glm::vec2 Scale { 0.25f, 0.25f };

		glm::mat4 GetTransform()
		{
			return glm::translate(glm::mat4(1.0f), Position)
				* glm::rotate(glm::mat4(1.0f), glm::radians(Rotation), {0.0f, 0.0f, 1.0f})
				* glm::scale(glm::mat4(1.0f), {Scale.x, Scale.y, 1.0f});
		}

		operator glm::mat4() { return GetTransform(); }
	};

	struct SpriteComponent
	{
		Shared<proton::Sprite> Sprite = nullptr;
		glm::vec4 Color{ 1.0f };
		float TilingFactor = 1.0f;
	};

	// Forward declaration
	class EntityScript;

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
			if (Scripts.find(scriptName) == Scripts.end())
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
		}
	};

	struct RelationshipComponent
	{
		std::uint32_t ChildrenCount = 0;
		entt::entity First  { entt::null };
		entt::entity Last   { entt::null };
		entt::entity Prev   { entt::null };
		entt::entity Next   { entt::null };
		entt::entity Parent { entt::null };
	};

	struct CameraComponent
	{
		Shared<Camera> Camera;
	};
}