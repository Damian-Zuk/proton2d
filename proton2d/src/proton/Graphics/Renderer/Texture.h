#pragma once
#include "Proton/Core/Base.h"

// Forward declaration
typedef unsigned int GLenum;
class Framebuffer;

namespace proton {

	enum class TextureFilterMode
	{
		Nearest = 0,
		Linear
	};

	enum class TextureWrapMode
	{
		Repeat = 0,
		ClampToBorder,
		ClampToEdge
	};

	enum class ImageFormat
	{
		None = 0,
		R8,
		RGB8,
		RGBA8,
		RGBA32F
	};

	struct TextureSpecification
	{
		uint32_t Width = 1;
		uint32_t Height = 1;
		ImageFormat Format = ImageFormat::RGBA8;
		bool GenerateMips = true;
	};

	class Texture
	{
	public:
		Texture(const TextureSpecification& specification);
		Texture(uint32_t width, uint32_t height, uint32_t objectID);
		Texture(uint32_t width, uint32_t height, bool fillDataWhitePixels = false);
		Texture(const std::string& path);
		virtual ~Texture();

		uint32_t GetOpenGL_ID() const { return m_Object_ID; }
		uint32_t GetWidth() const { return m_Width;  }
		uint32_t GetHeight() const { return m_Height; }
		const std::string& GetFilepath() const { return m_Filepath; }
		
		void Bind(uint32_t slot = 0) const;
		void SetData(void* data, size_t size);
		bool IsLoaded() const { return m_IsLoaded; }
		bool IsTextureAtlas() const { return m_IsTextureAtlas; }

		TextureFilterMode GetFilterMode() const { return m_FilterMode; }
		glm::vec<2, TextureWrapMode> GetWrapMode() const { return m_WrapMode; }

		void SetFilterMode(TextureFilterMode mode);
		void SetWrapMode(TextureWrapMode mode);
		void SetWrapMode(TextureWrapMode x_mode, TextureWrapMode y_mode);

		bool operator==(const Texture& other) const
		{
			return m_Object_ID == other.m_Object_ID;
		}

	protected:
		bool m_IsTextureAtlas = false;
		uint32_t m_Width = 0, m_Height = 0;

	private:
		uint32_t m_Object_ID = 0;

		TextureSpecification m_Specification;

		bool m_IsLoaded = false;
		bool m_IsFrameBufferTexture = false;
		
		std::string m_Filepath;
		
		GLenum m_InternalFormat = 0;
		GLenum m_DataFormat = 0;
		
		TextureFilterMode m_FilterMode = TextureFilterMode::Linear;
		glm::vec<2, TextureWrapMode> m_WrapMode { TextureWrapMode::Repeat };

		friend class SceneSerializer;
	};

}
