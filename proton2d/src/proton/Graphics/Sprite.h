#pragma once
#include "Proton/Graphics/TextureAtlas.h"

namespace proton {

	class Sprite
	{
	public:
		Sprite() = default;
		Sprite(Shared<Texture> texture);
		Sprite(Shared<TextureAtlas> textureAtlas);

		void SetTextureFromPath(const std::string& filepath);
		void SetTexture(Shared<Texture> texture);
		void SetTextureAtlas(Shared<TextureAtlas> textureAtlas);

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

		const Shared<Texture>& GetTexture() const { return m_Texture; };
		const glm::uvec2& GetTileSize() const { return m_TileSize; }
		const glm::uvec2& GetTileIndex() const { return m_TileIndex; }
		const glm::uvec2& GetPixelSize() const { return m_PixelSize; }
		const glm::bvec2& GetMirrorFlip() const { return m_MirrorFlip; }

		float GetAspectRatio() const { return (float)m_PixelSize.x / (float)m_PixelSize.y; }

		operator bool() const { return m_Texture != nullptr; }

	private:
		void CalculateTextureCoords();
		const TextureCoords& GetTextureCoords() const;
		
	private:
		Shared<Texture> m_Texture;
		Shared<TextureAtlas> m_TextureAtlas;

		TextureCoords m_TextureCoords;
		
		glm::uvec2 m_PixelSize = { 0, 0 };
		glm::uvec2 m_TileIndex = { 0, 0 };
		glm::uvec2 m_TileSize = { 1, 1 };
		glm::bvec2 m_MirrorFlip = { false, false };

		friend class Renderer;
		friend class Scene;
		friend class Entity;
		friend class AssetManager;

		friend class InspectorPanel;
		friend class SceneSerializer;
	};

}
