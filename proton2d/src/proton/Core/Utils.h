#pragma once 

#include "proton/Core/Logger.h"

#include <string>
#include <fstream>
#include <glm/glm.hpp>

namespace proton {

	std::string ReadFileBinary(const std::string& filepath);
	
	std::vector<std::string> GetFilesFromDirectory(const std::string& directory);

	glm::mat4 GetTransform(const glm::vec3& position, const glm::vec2& scale, float rotation = 0.0f);
}
