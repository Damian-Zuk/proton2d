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

	constexpr TextureCoords FULL_TEXTURE_COORDS = { {{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }} };

	class TextureAtlas : public Texture
	{
	public:
		TextureAtlas() = delete;
		TextureAtlas(const std::string& filepath, uint32_t tileWidth, uint32_t tileHeight);
		virtual ~TextureAtlas() = default;

		const TextureCoords& GetTextureCoords(uint32_t x, uint32_t y) const;
		glm::uvec2 GetTileSizePx() const { return m_TileSizePx; }
		glm::uvec2 GetTileCount() const { return m_TileCount; }
		glm::vec2 GetTileScale() const { return m_TileScale; }

	private:
		std::vector<TextureCoords> m_TextureCoords;
		glm::uvec2 m_TileSizePx = { 0, 0 }; 
		glm::uvec2 m_TileCount = { 0, 0 };
		glm::vec2 m_TileScale = { 0.0f, 0.0f };

		friend class Sprite;
		friend class ResizableSprite;
		friend class Scene;

		friend class InspectorPanel;
	};

}
