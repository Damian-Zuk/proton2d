#include "pch.h"
#include "proton/Graphics/Sprite.h"

namespace proton {

	SpriteSheet::SpriteSheet(const Shared<Texture>& texture, uint32_t tileWidth, uint32_t tileHeight)
		: m_SheetWidth(texture->GetWidth()), m_SheetHeight(texture->GetHeight()),
		m_TileWidth(tileWidth), m_TileHeight(tileHeight), m_Texture(texture)
	{
		m_MaxTilesX = (uint32_t)((float)m_SheetWidth / (float)m_TileWidth);
		m_MaxTilesY = (uint32_t)((float)m_SheetHeight / (float)m_TileHeight);
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

	Sprite::Sprite(const Shared<Texture>& texture)
		: m_Texture(texture)
	{
		m_TextureCoords = { {{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }} };
	}

	Sprite::Sprite(const Shared<SpriteSheet>& spriteSheet,
		uint32_t x, uint32_t y, uint32_t sizeX, uint32_t sizeY)
		: m_PosX(x), m_PosY(y), m_SizeX(sizeX), m_SizeY(sizeY),
		m_SpriteSheet(spriteSheet), m_Texture(spriteSheet->GetTexture())
	{
		SetTile(x, y, sizeX, sizeY);
	}

	void Sprite::SetSpriteSheet(const Shared<SpriteSheet>& spriteSheet)
	{
		m_SpriteSheet = spriteSheet;
		m_Texture = spriteSheet->GetTexture();
		SetTile(0, 0);
	}

	void Sprite::SetTile(uint32_t x, uint32_t y, uint32_t sizeX, uint32_t sizeY)
	{
		if (m_SpriteSheet)
		{
			auto& [maxX, maxY] = m_SpriteSheet->GetMaxTilesCount();
			auto& [sheetWidth, sheetHeight] = m_SpriteSheet->GetSheetSize();
			auto& [tileWidth, tileHeight] = m_SpriteSheet->GetTileSize();
			
			x %= maxX; y %= maxY;
			m_PosX = x; m_PosY = y;

			// Size: [0.0 - 1.0] range
			float tileSizeX = (float)tileWidth / (float)sheetWidth;
			float tileSizeY = (float)tileHeight / (float)sheetHeight;

			// Texture coords: [0.0 - 1.0] range
			m_TextureCoords[0] = {  x * tileSizeX,            y * tileSizeY          };
			m_TextureCoords[1] = { (x + sizeX) * tileSizeX,   y * tileSizeY          };
			m_TextureCoords[2] = { (x + sizeX) * tileSizeX,  (y + sizeY) * tileSizeY };
			m_TextureCoords[3] = {  x * tileSizeX,           (y + sizeY) * tileSizeY };
		}
		else
			assert(false && "Sprite doesn't have spritesheet attached!");
	}

	void Sprite::NextTile(uint32_t sizeX, uint32_t sizeY)
	{
		SetTile(m_PosX + 1, m_PosY, sizeX, sizeY);
	}

	void Sprite::PrevTile(uint32_t sizeX, uint32_t sizeY)
	{
		SetTile(m_PosX - 1, m_PosY, sizeX, sizeY);
	}

	Shared<Texture> Sprite::GetTexture()
	{
		return m_Texture;
	}

	const std::array<glm::vec2, 4>& Sprite::GetTextureCoords() const
	{
		return m_TextureCoords;
	}

}