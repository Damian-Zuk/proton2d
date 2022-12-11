/*
* ------------------------------------------------------------------------------------------
*  OpenGL vertex buffer and index buffer object API
*  From Hazel Engine made by Cherno
*  https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Platform/OpenGL/OpenGLBuffer.cpp
* ------------------------------------------------------------------------------------------
*/

#include "pch.h"
#include "proton/Graphics/OpenGL/Buffer.h"

#include <glad/glad.h>

namespace proton {

	VertexBuffer::VertexBuffer(uint32_t size)
	{
		glCreateBuffers(1, &m_Object_ID);
		glBindBuffer(GL_ARRAY_BUFFER, m_Object_ID);
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	}

	VertexBuffer::VertexBuffer(float* vertices, uint32_t size)
	{
		glCreateBuffers(1, &m_Object_ID);
		glBindBuffer(GL_ARRAY_BUFFER, m_Object_ID);
		glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_DYNAMIC_DRAW);
	}

	VertexBuffer::~VertexBuffer()
	{
		glDeleteBuffers(1, &m_Object_ID);
	}

	void VertexBuffer::Bind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_Object_ID);
	}

	void VertexBuffer::Unbind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void VertexBuffer::SetData(const void* data, uint32_t size)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_Object_ID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	}

	IndexBuffer::IndexBuffer(uint32_t* indices, uint32_t count)
		: m_Count(count)
	{
		glCreateBuffers(1, &m_Object_ID);
		glBindBuffer(GL_ARRAY_BUFFER, m_Object_ID);
		glBufferData(GL_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
	}

	IndexBuffer::~IndexBuffer()
	{
		glDeleteBuffers(1, &m_Object_ID);
	}

	void IndexBuffer::Bind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Object_ID);
	}

	void IndexBuffer::Unbind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

}
