#pragma once
#include "Proton/Graphics/Renderer/Texture.h"

#include <glm/glm.hpp>

namespace proton {

	// Source texture coordinates
	// Range: 0.0 - 1.0
	// [0] - bottom left (0,0)
	// [1] - bottom right (1,0)
	// [2] - top right (1,1)
	// [3] - top left (0,1)
	using TextureCoords = std::array<glm::vec2, 4>;

	class TextureAtlas
	{
	public:
		TextureAtlas(const Shared<Texture>& texture, uint32_t tileWidth, uint32_t tileHeight);

		const Shared<Texture>& GetTexture() { return m_Texture; }
		const glm::uvec2& GetAtlasSizePx() const { return m_AtlasSizePx; }
		const glm::uvec2& GetTileSizePx() const { return m_TileSizePx; }
		const glm::uvec2& GetTileCount() const { return m_TileCount; }
		const glm::vec2& GetTileScale() const { return m_TileScale; }
		const TextureCoords& GetTextureCoords(uint32_t x, uint32_t y) const;

		operator bool() const { return m_Texture != nullptr; }

	private:
		Shared<Texture> m_Texture;
		std::vector<TextureCoords> m_TextureCoords;
		glm::uvec2 m_AtlasSizePx = { 0, 0 };
		glm::uvec2 m_TileSizePx = { 0, 0 }; 
		glm::uvec2 m_TileCount = { 0, 0 };
		glm::vec2 m_TileScale = { 0, 0 };

		friend class Sprite;
		friend class ResizableSprite;
		friend class Scene;

		friend class InspectorPanel;
	};

}
