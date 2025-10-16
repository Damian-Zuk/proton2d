#pragma once
#include "Proton/Graphics/TextureAtlas.h"

namespace proton {

	enum class SpriteType
	{
		None = 0,
		FullTexture,
		TextureAtlas
	};

	class Sprite
	{
	public:
		Sprite() = default;
		Sprite(const Shared<Texture>& texture);
		Sprite(const Shared<TextureAtlas>& textureAtlas);

		SpriteType GetType() const { return m_Type; }
		const Shared<Texture>& GetTexture() const { return m_Texture; };
		const Shared<TextureAtlas>& GetTextureAtlas() const { return m_TextureAtlas; }

		void SetTexture(const Shared<Texture>& texture);
		void SetTextureFromPath(const std::string& filepath);
		void SetTextureAtlas(const Shared<TextureAtlas>& textureAtlas);

		void SetTileIndex(glm::uvec2 index);
		void SetTileIndex(uint32_t x, uint32_t y);
		void SetTileIndexX(uint32_t x);
		void SetTileIndexY(uint32_t y);

		void SetTileCount(glm::uvec2 count);
		void SetTileCount(uint32_t width, uint32_t height);
		void SetTileCountX(uint32_t width);
		void SetTileCountY(uint32_t height);

		void SetMirrorFlip(glm::bvec2 flip);
		void SetMirrorFlip(bool flip_x, bool flip_y);
		void SetMirrorFlipX(bool flip_x);
		void SetMirrorFlipY(bool flip_y);

		glm::uvec2 GetTileSize() const { return m_TileCount; }
		glm::uvec2 GetTileIndex() const { return m_TileIndex; }
		glm::uvec2 GetPixelSize() const { return m_PixelSize; }
		const TextureCoords& GetTextureCoords() const { return m_TextureCoords; }

		float GetAspectRatio() const { return (float)m_PixelSize.x / (float)m_PixelSize.y; }

		operator bool() const { return m_Texture != nullptr; }

	private:
		void CalculateTextureCoords();
		
	private:
		SpriteType m_Type = SpriteType::None;
		Shared<Texture> m_Texture;
		Shared<TextureAtlas> m_TextureAtlas;

		TextureCoords m_TextureCoords = FULL_TEXTURE_COORDS;
		
		glm::uvec2 m_PixelSize = { 0, 0 };
		glm::uvec2 m_TileIndex = { 0, 0 };
		glm::uvec2 m_TileCount = { 1, 1 };
		glm::bvec2 m_MirrorFlip = { false, false };

		friend class Renderer;
		friend class Scene;
		friend class Entity;
		friend class AssetManager;

		friend class InspectorPanel;
		friend class SceneSerializer;
	};

}
