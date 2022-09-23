#pragma once

#include "proton/Core/Core.h"

typedef unsigned int GLenum;

namespace proton {

	class Texture
	{
	public:
		Texture(uint32_t width, uint32_t height, bool fillData = false);
		Texture(const std::string& path);
		virtual ~Texture();

		uint32_t GetWidth() const { return m_Width;  }
		uint32_t GetHeight() const { return m_Height; }
		uint32_t GetOpenGL_ID() const { return m_OpenGL_ID; }
		const std::string& GetPath() const { return m_Path; }
		
		void Bind(uint32_t slot = 0) const;
		void SetData(void* data, size_t size);
		bool IsLoaded() const { return m_IsLoaded; }

		bool operator==(const Texture& other) const
		{
			return m_OpenGL_ID == other.GetOpenGL_ID();
		}

	private:
		std::string m_Path;
		bool m_IsLoaded = false;
		uint32_t m_Width, m_Height;
		uint32_t m_OpenGL_ID;
		GLenum m_InternalFormat;
		GLenum m_DataFormat;
	};

}
