#include "pch.h"
#include "proton/Graphics/Sprite.h"

namespace proton {

	SpriteSheet::SpriteSheet(const Shared<Texture>& texture, uint32_t tileWidth, uint32_t tileHeight)
		: m_SheetWidth(texture->GetWidth()), m_SheetHeight(texture->GetHeight()),
		m_TileWidth(tileWidth), m_TileHeight(tileHeight), m_Texture(texture)
	{
	}

	Shared<Texture> SpriteSheet::GetTexture()
	{
		return m_Texture;
	}

	std::pair<uint32_t, uint32_t> SpriteSheet::GetSheetSize()
	{
		return std::pair<uint32_t, uint32_t>(m_SheetWidth, m_SheetHeight);
	}

	std::pair<uint32_t, uint32_t> SpriteSheet::GetTileSize()
	{
		return std::pair<uint32_t, uint32_t>(m_TileWidth, m_TileHeight);
	}

	Sprite::Sprite(const Shared<Texture>& texture)
		: m_Texture(texture)
	{
		m_TextureCoords = { 
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f } 
		};
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
			auto [sheetWidth, sheetHeight] = m_SpriteSheet->GetSheetSize();
			auto [tileWidth, tileHeight] = m_SpriteSheet->GetTileSize();

			assert((x + sizeX) * tileWidth <= sheetWidth && (y + sizeY) * tileHeight <= sheetHeight
				&& "Sprite texture position out of bounds!");

			float tileSizeX = (float)tileWidth / (float)sheetWidth;
			float tileSizeY = (float)tileHeight / (float)sheetHeight;

			m_TextureCoords = {
				{ x * tileSizeX,           y * tileSizeY           },
				{ (x + sizeX) * tileSizeX, y * tileSizeY           },
				{ (x + sizeX) * tileSizeX, (y + sizeY) * tileSizeY },
				{ x * tileSizeX,           (y + sizeY) * tileSizeY }
			};
		}
		else
			assert("Sprite doesn't have spritesheet attached!");
	}

	Shared<Texture> Sprite::GetTexture()
	{
		return m_Texture;
	}

	const glm::mat4x2& Sprite::GetTextureCoords()
	{
		return m_TextureCoords;
	}

}