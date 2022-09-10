#include "pch.h"
#include "proton/Core/Utils.h"

#include <fstream>

namespace proton {

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

}