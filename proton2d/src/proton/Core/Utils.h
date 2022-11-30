#pragma once 

#include "proton/Core/Logger.h"

#include <string>
#include <fstream>

namespace proton {

	std::string ReadFileBinary(const std::string& filepath);
	
	std::vector<std::string> GetFilesFromDirectory(const std::string& directory);

}
