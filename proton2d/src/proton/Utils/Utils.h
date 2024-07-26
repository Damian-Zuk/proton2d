#pragma once 

#include <string>
#include <fstream>
#include <glm/glm.hpp>

namespace proton { 
	
	namespace Utils {

		std::string ReadFile(const std::string& filepath);

		namespace Graphics {
			glm::vec4 RGBAtoHSV(const glm::vec4& rgba);
			glm::vec4 HSVtoRGBA(const glm::vec4& hsv);
		}
	
	} 

	namespace FileDialogs {

		// Implementation per platform
		std::string OpenFile(const char* filter);
		std::string SaveFile(const char* filter);

	}

	namespace Math {

		glm::mat4 GetTransform(const glm::vec3& position, const glm::vec2& scale, float rotation = 0.0f);

		bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
	}
}
