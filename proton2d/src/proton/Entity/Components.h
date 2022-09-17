#pragma once

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
		glm::vec2 Scale { 1.0f, 1.0f };

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
		glm::vec4 Color{ 1.0f };
	};
}