// From: https://github.com/StudioCherno/Walnut/blob/dev/Walnut/Source/Walnut/Serialization/StreamReader.cpp
#include "ptpch.h"
#include "Proton/Serialization/StreamReader.h"

#define MAX_STRING_SIZE 1024 * 1024
#define MAX_BUFFER_SIZE 1024 * 1024

namespace proton {

    bool StreamReader::ReadBuffer(Buffer& buffer, uint32_t size)
    {
	    buffer.Size = size;
	    if (size == 0)
	    {
		    if (!ReadData(reinterpret_cast<char*>(&buffer.Size), sizeof(uint32_t)))
			    return false;
	    }

        if (buffer.Size > MAX_BUFFER_SIZE)
        {
            PT_CORE_ASSERT(false, "Buffer size limit exceeded")
            return false;
        }

	    buffer.Allocate(buffer.Size);
	    return ReadData(reinterpret_cast<char*>(buffer.Data), buffer.Size);
    }

    bool StreamReader::ReadString(std::string& string)
    {
        size_t size;
        if (!ReadData(reinterpret_cast<char*>(&size), sizeof(size_t)))
            return false;

        if (size > MAX_STRING_SIZE)
        {
            PT_CORE_ASSERT(false, "String size limit exceeded")
            return false;
        }

        string.resize(size);
        return ReadData(reinterpret_cast<char*>(string.data()), size);
    }

}
