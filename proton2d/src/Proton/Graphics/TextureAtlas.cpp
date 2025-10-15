#include "ptpch.h"
#include "Proton/Graphics/TextureAtlas.h"

namespace proton {

	TextureAtlas::TextureAtlas(const Shared<Texture>& texture, uint32_t tileWidth, uint32_t tileHeight)
		: m_AtlasSizePx({ texture->GetWidth(), texture->GetHeight() }),
		m_TileSizePx(tileWidth, tileHeight), m_Texture(texture)
	{
		m_TileCount = m_AtlasSizePx / m_TileSizePx;
		m_TileScale = glm::vec2{ 1.0f } / (glm::vec2)m_TileCount;
		const size_t tileCount = m_TileCount.x * m_TileCount.y;
		m_TextureCoords.resize(tileCount);

		for (size_t i = 0; i < tileCount; i++)
		{
			const uint32_t x = (uint32_t)i / m_TileCount.y;
			const uint32_t y = (uint32_t)i % m_TileCount.y;
			m_TextureCoords[i][0] = { x * m_TileScale.x, y * m_TileScale.y };
			m_TextureCoords[i][1] = { (x + 1) * m_TileScale.x, y * m_TileScale.y };
			m_TextureCoords[i][2] = { (x + 1) * m_TileScale.x, (y + 1) * m_TileScale.y };
			m_TextureCoords[i][3] = { x * m_TileScale.x, (y + 1) * m_TileScale.y };
		}
	}

	const TextureCoords& TextureAtlas::GetTextureCoords(uint32_t x, uint32_t y) const
	{
		PT_CORE_ASSERT(x < m_TileCount.x && y < m_TileCount.y, "Tile position out of bounds!");
		return m_TextureCoords[(x % m_TileCount.x) * m_TileCount.y + (y % m_TileCount.y)];
	}

}
