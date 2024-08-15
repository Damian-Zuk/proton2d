#include "ptpch.h"
#include "Proton/Network/NetworkStream.h"

namespace proton {

	NetworkStreamWriter::NetworkStreamWriter(Buffer& targetBuffer, uint64_t position)
		: m_TargetBuffer(targetBuffer), m_BufferPosition(position)
	{
	}

	bool NetworkStreamWriter::WriteData(const char* data, size_t size)
	{
		if (m_BufferPosition + size > m_TargetBuffer.Size)
		{
			uint64_t newSize = m_TargetBuffer.Size * 2;
			while (m_BufferPosition + size > newSize)
				newSize *= 2;

			PT_CORE_WARN("Performing buffer reallocation (new_size={})", newSize);
			m_TargetBuffer.Reallocate(newSize);
		}

		memcpy(m_TargetBuffer.As<uint8_t>() + m_BufferPosition, data, size);
		m_BufferPosition += size;
		return true;
	}

	NetworkStreamReader::NetworkStreamReader(Buffer& targetBuffer, uint64_t position)
		: m_TargetBuffer(targetBuffer), m_BufferPosition(position)
	{
	}

	bool NetworkStreamReader::ReadData(char* destination, size_t size)
	{
		bool valid = m_BufferPosition + size <= m_TargetBuffer.Size;
		PT_CORE_VERIFY(valid);
		if (!valid)
			return false;

		memcpy(destination, m_TargetBuffer.As<uint8_t>() + m_BufferPosition, size);
		m_BufferPosition += size;
		return true;
	}

}
