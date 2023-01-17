#include "pch.h"
#include "proton/Core/Utils.h"

#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>

#ifdef PROTON_PLATFORM_WINDOWS
#define DIR_SLASH "\\"
#else
#define DIR_SLASH "/"
#endif


namespace proton { namespace Utils {

	std::string ReadFile(const std::string& filepath)
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

	std::vector<std::string> GetFilesFromDirectory(const std::string& directory,
		std::initializer_list<std::string> extensionsFilter, bool returnExtensions)
	{
		std::vector<std::string> files;
		for (const auto& entry : std::filesystem::directory_iterator(directory))
		{
			const auto& path = entry.path();
			auto extension = path.extension().string();
			for (const std::string& ext : extensionsFilter)
			{
				if (extension == ext)
				{
					std::string filepath = path.string();
					if (!returnExtensions)
						filepath = filepath.substr(0, filepath.size() - extension.size());
					files.push_back(filepath.substr(filepath.find_first_of(DIR_SLASH) + 1));
					break;
				}
			}
		}
		return files;
	}

	std::vector<std::string> GetFilesFromDirectoryRecursive(const std::string& directory,
		std::initializer_list<std::string> extensionsFilter, bool returnExtensions)
	{
		std::vector<std::string> files;
		for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
			if (entry.is_regular_file()) 
			{
				const auto& path = entry.path();
				auto extension = path.extension().string();
				for (const std::string& ext : extensionsFilter)
				{
					if (extension == ext) 
					{
						std::string filepath = path.string();
						if (!returnExtensions)
							filepath = filepath.substr(0, filepath.size() - extension.size());
						files.push_back(filepath.substr(filepath.find_first_of(DIR_SLASH) + 1));
						break;
					}
				}
			}
		}
		return files;
	}

} 
	glm::mat4 Math::GetTransform(const glm::vec3& position, const glm::vec2& scale, float rotation)
	{
		return glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { scale.x, scale.y, 1.0f });
	}
}