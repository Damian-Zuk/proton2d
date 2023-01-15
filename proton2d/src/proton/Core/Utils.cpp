#include "pch.h"
#include "proton/Core/Utils.h"

#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>

namespace proton
{
	std::string ReadFileBinary(const std::string& filepath)
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			size_t size = in.tellg();
			if (size != -1)
			{
				result.resize(size);
				in.seekg(0, std::ios::beg);
				in.read(&result[0], size);
				in.close();
			}
			else
				LOG_ERROR("Could not read from file", filepath);
		}
		else
			LOG_ERROR("Could not open file", filepath);

		return result;
	}

	std::vector<std::string> GetFilesFromDirectory(const std::string& directory, const std::string& extension)
	{
		std::vector<std::string> files;
		for (const auto& entry : std::filesystem::directory_iterator(directory))
		{
			files.emplace_back(entry.path().stem().generic_string());
		}
		return files;
	}

	glm::mat4 GetTransform(const glm::vec3& position, const glm::vec2& scale, float rotation)
	{
		return glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { scale.x, scale.y, 1.0f });
	}
}