#include "pch.h"
#include "proton/Core/Utils.h"

#include <filesystem>

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

	std::vector<std::string> GetFilesFromDirectory(const std::string& directory)
	{
		std::vector<std::string> directoryScenes;
		for (const auto& entry : std::filesystem::directory_iterator(directory))
			directoryScenes.emplace_back(entry.path().stem().generic_string());
		return directoryScenes;
	}
}