#pragma once

#include "proton/Graphics/Sprite.h"
#include "proton/Graphics/Camera.h"

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
		std::unordered_map<std::string, EntityScript*> ScriptInstances;
		std::unordered_map<std::string, std::function<void()>> CreateInstanceFunctions;

		template<typename T>
		void BindScript(const std::string& scriptName)
		{
			if (CreateInstanceFunctions.find(scriptName) == CreateInstanceFunctions.end())
			{
				CreateInstanceFunctions[scriptName] = [&, scriptName]()
				{
					ScriptInstances[scriptName] = new T();
				};
			}
			else
				LOG_WARN("Tried to bound script", scriptName, "to entity which already has this script!");
		}
	};

	struct CameraComponent
	{
		Shared<Camera> Camera;
	};
}