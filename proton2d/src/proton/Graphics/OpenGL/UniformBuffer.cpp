/*
* ------------------------------------------------------------------------------------------
*  OpenGL uniform buffer object API
*  Based on Hazel Engine made by Cherno:
*  https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Platform/OpenGL/OpenGLUniformBuffer.cpp
* ------------------------------------------------------------------------------------------
*/

#include "pch.h"
#include "proton/Graphics/OpenGL/UniformBuffer.h"

#include <glad/glad.h>

namespace proton {

	UniformBuffer::UniformBuffer(uint32_t size, uint32_t binding)
	{
		glCreateBuffers(1, &m_OpenGL_ID);
		glNamedBufferData(m_OpenGL_ID, size, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_OpenGL_ID);
	}

	UniformBuffer::~UniformBuffer()
	{
		glDeleteBuffers(1, &m_OpenGL_ID);
	}

	void UniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		glNamedBufferSubData(m_OpenGL_ID, offset, size, data);
	}

}
