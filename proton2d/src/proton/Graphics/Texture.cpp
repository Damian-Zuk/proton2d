#include "pch.h"
#include "proton/Graphics/Texture.h"

#include <glad/glad.h>
#include <stb_image.h>

namespace proton {

	Texture::Texture(uint32_t width, uint32_t height, bool fillData)
		: m_Width(width), m_Height(height)
	{
		m_InternalFormat = GL_RGBA8;
		m_DataFormat = GL_RGBA;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_OpenGL_ID);
		glTextureStorage2D(m_OpenGL_ID, 1, m_InternalFormat, m_Width, m_Height);

		glTextureParameteri(m_OpenGL_ID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(m_OpenGL_ID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTextureParameteri(m_OpenGL_ID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_OpenGL_ID, GL_TEXTURE_WRAP_T, GL_REPEAT);

		if (fillData) 
		{
			size_t pixels = (size_t)width * (size_t)height;
			uint32_t* data = new uint32_t[pixels];
			memset(data, 0xffffffff, pixels * sizeof(uint32_t));
			SetData(data, pixels * sizeof(uint32_t));
			delete[] data;
		}
	}

	Texture::Texture(const std::string& path)
		: m_Path(path)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* data = stbi_load((PROTON_ASSETS_DIR + path).c_str(), &width, &height, &channels, 0);
			
		if (data)
		{
			m_IsLoaded = true;
			m_Width = width;
			m_Height = height;

			if (channels == 4)
			{
				m_InternalFormat = GL_RGBA8;
				m_DataFormat = GL_RGBA;
			}
			else if (channels == 3)
			{
				m_InternalFormat = GL_RGB8;
				m_DataFormat = GL_RGB;
			}

			assert(m_InternalFormat & m_DataFormat && "Format not supported!");

			glCreateTextures(GL_TEXTURE_2D, 1, &m_OpenGL_ID);
			glTextureStorage2D(m_OpenGL_ID, 1, m_InternalFormat, m_Width, m_Height);

			glTextureParameteri(m_OpenGL_ID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(m_OpenGL_ID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glTextureParameteri(m_OpenGL_ID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_OpenGL_ID, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTextureSubImage2D(m_OpenGL_ID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);

			stbi_image_free(data);
		}
	}

	Texture::~Texture()
	{
		glDeleteTextures(1, &m_OpenGL_ID);
	}

	void Texture::SetData(void* data, size_t size)
	{
		uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
		assert(size == m_Width * m_Height * bpp && "Data must be entire texture!");
		glTextureSubImage2D(m_OpenGL_ID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
	}

	void Texture::Bind(uint32_t slot) const
	{
		glBindTextureUnit(slot, m_OpenGL_ID);
	}
}
