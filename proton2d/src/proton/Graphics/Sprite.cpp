#include "pch.h"
#include "proton/Graphics/Sprite.h"

namespace proton {

	SpriteSheet::SpriteSheet(const Shared<Texture>& texture, uint32_t tileWidth, uint32_t tileHeight)
		: m_SheetSize({ texture->GetWidth(), texture->GetHeight() }),
		m_TileSize(tileWidth, tileHeight), m_Texture(texture)
	{
		m_TileCount = m_SheetSize / m_TileSize;
		m_TileScale = glm::vec2{ 1.0f } / (glm::vec2)m_TileCount;

		uint32_t x = 0, y = 0;
		m_TextureCoords.resize(m_TileCount.x);
		for (auto& column : m_TextureCoords)
		{
			column.resize(m_TileCount.y);
			for (auto& tileTextureCoords : column)
			{
				// Texture coords: [0.0 - 1.0] range
				tileTextureCoords[0] = { x * m_TileScale.x, y * m_TileScale.y };
				tileTextureCoords[1] = { (x + 1) * m_TileScale.x, y * m_TileScale.y };
				tileTextureCoords[2] = { (x + 1) * m_TileScale.x, (y + 1) * m_TileScale.y };
				tileTextureCoords[3] = { x * m_TileScale.x, (y + 1) * m_TileScale.y };
				y++;
			}
			y = 0; x++;
		}
	}

	Shared<Texture> SpriteSheet::GetTexture()
	{
		return m_Texture;
	}

	const TextureCoords& SpriteSheet::GetTextureCoords(uint32_t x, uint32_t y) const
	{
		assert(x < m_TileCount.x && y < m_TileCount.y && "Tile position out of bounds!");
		return m_TextureCoords[x % m_TileCount.x][y % m_TileCount.y];
	}

	Sprite::Sprite(const Shared<Texture>& texture)
		: m_Texture(texture), m_PixelSize({ texture->GetWidth(), texture->GetHeight() })
	{
	}

	Sprite::Sprite(const Shared<SpriteSheet>& spriteSheet)
		: m_SpriteSheet(spriteSheet), m_Texture(spriteSheet->GetTexture()),
		m_PixelSize(spriteSheet->m_TileSize)
	{
	}

	void Sprite::SetTexture(const Shared<Texture>& texture)
	{
		m_Texture = texture;
		m_PixelSize = { texture->GetWidth(), texture->GetHeight() };
	}

	void Sprite::SetSpriteSheet(const Shared<SpriteSheet>& spriteSheet)
	{
		m_SpriteSheet = spriteSheet;
		m_Texture = spriteSheet->GetTexture();
		m_PixelSize = m_TileSize * spriteSheet->m_TileSize;
		SetTile(0, 0);
	}

	void Sprite::SetTile(uint32_t x, uint32_t y, uint32_t sizeX, uint32_t sizeY)
	{
		assert(m_SpriteSheet && "Sprite doesn't have spritesheet attached!");

		glm::uvec2 count = m_SpriteSheet->GetTileCount();
		x %= count.x; y %= count.y;
		m_TilePos = { x, y };
		m_TileSize = { sizeX, sizeY };
	}

	void Sprite::SetTileX(uint32_t x, uint32_t sizeX, uint32_t sizeY)
	{
		SetTile(x, m_TilePos.y);
	}

	void Sprite::SetTileY(uint32_t y, uint32_t sizeX, uint32_t sizeY)
	{
		SetTile(m_TilePos.x, y);
	}

	void Sprite::NextTile(uint32_t posY, uint32_t sizeX, uint32_t sizeY)
	{
		SetTile(m_TilePos.x + 1, posY, sizeX, sizeY);
	}

	void Sprite::PrevTile(uint32_t posY, uint32_t sizeX, uint32_t sizeY)
	{
		SetTile(m_TilePos.x - 1, posY, sizeX, sizeY);
	}

	void Sprite::MirrorFlip(bool mirror_x, bool mirror_y)
	{
		m_MirrorFlipX = mirror_x, m_MirrorFlipY = mirror_y;
	}

	Shared<Texture> Sprite::GetTexture()
	{
		return m_Texture;
	}

	static TextureCoords s_FullTextureCoords
		= { {{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }} };

	const TextureCoords& Sprite::GetTextureCoords()
	{
		if (m_SpriteSheet)
			return m_SpriteSheet->GetTextureCoords(m_TilePos.x, m_TilePos.y);

		// If spritesheet not attached then return full texture coordinates
		return s_FullTextureCoords;
	}

}