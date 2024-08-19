#pragma once
#include "Proton/Serialization/StreamReader.h"
#include "Proton/Serialization/StreamWriter.h"

namespace proton
{
	// BufferStream with networking in mind
	// Adapted from: Proton/Serialization/BufferStream.h
	// Writer: Prevents buffer overflow by updating buffer size with reallocation.
	// Network messages are written to preallocated buffer with fixed initial size (m_ScratchBuffer).

	class NetworkStreamWriter : public StreamWriter
	{
	public:
		NetworkStreamWriter(Buffer& targetBuffer, uint64_t position = 0);
		NetworkStreamWriter(const NetworkStreamWriter&) = delete;
		virtual ~NetworkStreamWriter() override = default;

		bool IsStreamGood() const final { return (bool)m_TargetBuffer; }
		uint64_t GetStreamPosition() override { return m_BufferPosition; }
		void SetStreamPosition(uint64_t position) override { m_BufferPosition = position; }
		bool WriteData(const char* data, size_t size) final;

		// Returns Buffer with currently written size
		Buffer GetBuffer() const { return Buffer(m_TargetBuffer, m_BufferPosition); }
		const Buffer& GetTargetBuffer() const { return m_TargetBuffer; }
		uint64_t GetRemainingBufferSize() const { return m_TargetBuffer.Size - m_BufferPosition; }
	private:
		Buffer& m_TargetBuffer;
		uint64_t m_BufferPosition = 0;
	};

	class NetworkStreamReader : public StreamReader
	{
	public:
		NetworkStreamReader(Buffer& targetBuffer, uint64_t position = 0);
		NetworkStreamReader(const NetworkStreamReader&) = delete;
		virtual ~NetworkStreamReader() override = default;

		bool IsStreamGood() const final { return (bool)m_TargetBuffer; }
		uint64_t GetStreamPosition() override { return m_BufferPosition; }
		void SetStreamPosition(uint64_t position) override { m_BufferPosition = position; }
		bool ReadData(char* destination, size_t size) override;

		// Returns Buffer with currently read size
		Buffer GetBuffer() const { return Buffer(m_TargetBuffer, m_BufferPosition); }
		const Buffer& GetTargetBuffer() const { return m_TargetBuffer; }
		uint64_t GetRemainingBufferSize() const { return m_TargetBuffer.Size - m_BufferPosition; }
	private:
		Buffer& m_TargetBuffer;
		uint64_t m_BufferPosition = 0;
	};

	using NetworkStreamReaderDelegate = std::function<void(NetworkStreamReader& stream)>;
	using NetworkStreamWriterDelegate = std::function<void(NetworkStreamWriter& stream)>;

}