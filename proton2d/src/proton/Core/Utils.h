#pragma once 

#include "proton/Core/Logger.h"

#include <string>
#include <fstream>
#include <glm/glm.hpp>

namespace proton { namespace Utils {

	std::string ReadFile(const std::string& filepath);

	std::vector<std::string> GetFilesFromDirectory(const std::string& directory,
		std::initializer_list<std::string> extensionsFilter = {}, bool returnExtensions = true);
	std::vector<std::string> GetFilesFromDirectoryRecursive(const std::string& directory,
		std::initializer_list<std::string> extensionsFilter = {}, bool returnExtensions = true);

} 

namespace FileDialogs {

	// Implementation per platform
	std::string OpenFile(const char* filter);
	std::string SaveFile(const char* filter);

}

namespace Math {

	glm::mat4 GetTransform(const glm::vec3& position, const glm::vec2& scale, float rotation = 0.0f);

}
}
