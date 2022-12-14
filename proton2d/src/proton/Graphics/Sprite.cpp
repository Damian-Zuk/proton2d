#include "pch.h"
#include "proton/Graphics/Sprite.h"

namespace proton {

	SpriteSheet::SpriteSheet(const Shared<Texture>& texture, uint32_t tileWidth, uint32_t tileHeight)
		: m_SheetWidth(texture->GetWidth()), m_SheetHeight(texture->GetHeight()),
		m_TileWidth(tileWidth), m_TileHeight(tileHeight), m_Texture(texture)
	{
		m_MaxTilesX = (uint32_t)((float)m_SheetWidth / (float)m_TileWidth);
		m_MaxTilesY = (uint32_t)((float)m_SheetHeight / (float)m_TileHeight);
		
		// Tile size: [0.0 - 1.0] range
		float tileSizeX = (float)tileWidth / (float)m_SheetWidth;
		float tileSizeY = (float)tileHeight / (float)m_SheetHeight;

		m_TextureCoords.resize(m_MaxTilesX);
		uint32_t x = 0, y = 0;

		for (auto& column : m_TextureCoords)
		{
			column.resize(m_MaxTilesY);
			for (auto& tileTextureCoords : column)
			{
				// Texture coords: [0.0 - 1.0] range
				tileTextureCoords[0] = { x * tileSizeX, y * tileSizeY };
				tileTextureCoords[1] = { (x + 1) * tileSizeX, y * tileSizeY };
				tileTextureCoords[2] = { (x + 1) * tileSizeX, (y + 1) * tileSizeY };
				tileTextureCoords[3] = { x * tileSizeX, (y + 1) * tileSizeY };
				y++;
			}
			y = 0; x++;
		}
	}

	Shared<Texture> SpriteSheet::GetTexture()
	{
		return m_Texture;
	}

	std::pair<uint32_t, uint32_t> SpriteSheet::GetSheetSize() const
	{
		return { m_SheetWidth, m_SheetHeight };
	}

	std::pair<uint32_t, uint32_t> SpriteSheet::GetTileSize() const
	{
		return { m_TileWidth, m_TileHeight };
	}

	std::pair<uint32_t, uint32_t> SpriteSheet::GetMaxTilesCount() const
	{
		return { m_MaxTilesX, m_MaxTilesY };
	}

	const TextureCoords& SpriteSheet::GetTextureCoords(uint32_t x, uint32_t y) const
	{
		assert(x < m_MaxTilesX && y < m_MaxTilesY && "Tile position out of bounds!");
		return m_TextureCoords[x][y];
	}

	Sprite::Sprite(const Shared<Texture>& texture)
		: m_Texture(texture), m_Width_px(texture->GetWidth()), m_Height_px(texture->GetHeight())
	{
	}

	Sprite::Sprite(const Shared<SpriteSheet>& spriteSheet,
		uint32_t x, uint32_t y, uint32_t sizeX, uint32_t sizeY)
		: m_PosX(x), m_PosY(y), m_SizeX(sizeX), m_SizeY(sizeY),
		m_Width_px(sizeX * spriteSheet->m_TileWidth),
		m_Height_px(sizeY * spriteSheet->m_TileHeight),
		m_SpriteSheet(spriteSheet), m_Texture(spriteSheet->GetTexture())
	{
		SetTile(x, y, sizeX, sizeY);
	}

	void Sprite::SetSpriteSheet(const Shared<SpriteSheet>& spriteSheet)
	{
		m_SpriteSheet = spriteSheet;
		m_Texture = spriteSheet->GetTexture();
		m_Width_px = m_SizeX * spriteSheet->m_TileWidth;
		m_Height_px = m_SizeY * spriteSheet->m_TileHeight;
		SetTile(0, 0);
	}

	void Sprite::SetTile(uint32_t x, uint32_t y, uint32_t sizeX, uint32_t sizeY)
	{
		assert(m_SpriteSheet && "Sprite doesn't have spritesheet attached!");

		auto& [maxX, maxY] = m_SpriteSheet->GetMaxTilesCount();
		x %= maxX; y %= maxY;
		m_PosX = x; m_PosY = y;
	}

	void Sprite::SetTileX(uint32_t x, uint32_t sizeX, uint32_t sizeY)
	{
		SetTile(x, m_PosY);
	}

	void Sprite::SetTileY(uint32_t y, uint32_t sizeX, uint32_t sizeY)
	{
		SetTile(m_PosX, y);
	}

	void Sprite::NextTile(uint32_t posY, uint32_t sizeX, uint32_t sizeY)
	{
		SetTile(m_PosX + 1, posY, sizeX, sizeY);
	}

	void Sprite::PrevTile(uint32_t posY, uint32_t sizeX, uint32_t sizeY)
	{
		SetTile(m_PosX - 1, posY, sizeX, sizeY);
	}

	void Sprite::FlipX(bool flip)
	{
		m_FlipX = flip;
	}

	void Sprite::FlipY(bool flip)
	{
		m_FlipY = flip;
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
			return m_SpriteSheet->GetTextureCoords(m_PosX, m_PosY);

		// If spritesheet not attached then return full texture coordinates
		return s_FullTextureCoords;
	}

}