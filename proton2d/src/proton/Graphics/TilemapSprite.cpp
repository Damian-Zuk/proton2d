#include "pch.h"
#include "proton/Graphics/TilemapSprite.h"

namespace proton {

	TilemapSprite::TilemapSprite()
	{
		SetSize(1, 1);
	}

	void TilemapSprite::SetSpritesheet(const Shared<SpriteSheet>& spritesheet)
	{
		m_Spritesheet = spritesheet;
	}

	void TilemapSprite::SetSize(uint32_t width, uint32_t height)
	{
		if (width != m_Width || height != m_Height)
		{
			m_Width = width; m_Height = height;
			m_Tilemap.resize(width);
			for (auto& column : m_Tilemap)
				column.resize(height, TILEMAP_BLANK_TILE);

			GenerateSliceScaledSprite();
		}
	}

	void TilemapSprite::GenerateSliceScaledSprite()
	{
		uint16_t s = 2;
		uint16_t sx = m_SlicedSpritePos.x, sy = m_SlicedSpritePos.y;

		// Fill whole tilemap with center slices
		for (uint32_t y = 0; y < m_Height; y++)
			for (uint32_t x = 0; x < m_Width; x++)
				m_Tilemap[x][y] = { sx + 1 + x % (s - 1), sy + 1 + y % (s - 1) };

		// Top left corner
		if (m_BlockBorders & BlockBorder_TopLeft)
		{
			m_Tilemap[0][m_Height - 1] = { sx, sy + s };

			if (!(m_BlockBorders & BlockBorder_Left))
				m_Tilemap[0][m_Height - 1] = { sx + 1, sy + s };

			if (!(m_BlockBorders & BlockBorder_Top))
				m_Tilemap[0][m_Height - 1] = { sx, sy + s - 1 };
		}

		// Top right corner
		if (m_BlockBorders & BlockBorder_TopRight)
		{
			m_Tilemap[m_Width - 1][m_Height - 1] = { sx + s, sy + s };

			if (!(m_BlockBorders & BlockBorder_Right))
				m_Tilemap[m_Width - 1][m_Height - 1] = { sx + s - 1, sy + s };

			if (!(m_BlockBorders & BlockBorder_Top))
				m_Tilemap[m_Width - 1][m_Height - 1] = { sx + s, sy + s - 1 };
		}

		// Bottom left corner
		if (m_BlockBorders & BlockBorder_BottomLeft)
		{
			m_Tilemap[0][0] = { sx, sy };

			if (!(m_BlockBorders & BlockBorder_Left))
				m_Tilemap[0][0] = { sx + 1, sy };

			if (!(m_BlockBorders & BlockBorder_Bottom))
				m_Tilemap[0][0] = { sx, sy + 1 };
		}

		// Bottom right corner
		if (m_BlockBorders & BlockBorder_BottomRight)
		{
			m_Tilemap[m_Width - 1][0] = { sx + s, sy };

			if (!(m_BlockBorders & BlockBorder_Right))
				m_Tilemap[m_Width - 1][0] = { sx + s - 1, sy };

			if (!(m_BlockBorders & BlockBorder_Bottom))
				m_Tilemap[m_Width - 1][0] = { sx + s, sy + s - 1};
		}

		// Left border
		if (m_BlockBorders & BlockBorder_Left)
			for (uint32_t y = 1; y < m_Height - 1; y++)
				m_Tilemap[0][y] = { sx, sy + 1 + y % (s - 1) };

		// Right border
		if (m_BlockBorders & BlockBorder_Right)
			for (uint32_t y = 1; y < m_Height - 1; y++)
				m_Tilemap[m_Width - 1][y] = { sx + s, sy + 1 + y % (s - 1) };

		// Top border
		if (m_BlockBorders & BlockBorder_Top)
			for (uint32_t x = 1; x < m_Width - 1; x++)
				m_Tilemap[x][m_Height - 1] = { sx + 1 + x % (s - 1), sy + s };

		// Bottom border
		if (m_BlockBorders & BlockBorder_Bottom)
			for (uint32_t x = 1; x < m_Width - 1; x++)
				m_Tilemap[x][0] = { sx + 1 + x % (s - 1), sy };
	}

	void TilemapSprite::SetSlicedSpritePosition(const glm::uvec2& position)
	{
		if (position != m_SlicedSpritePos)
		{
			m_SlicedSpritePos = position;
			GenerateSliceScaledSprite();
		}
	}

	void TilemapSprite::SetBlockBorders(BlockBorders borders)
	{
		if (borders != m_BlockBorders)
		{
			m_BlockBorders = borders;
			GenerateSliceScaledSprite();
		}
	}

	Shared<SpriteSheet> TilemapSprite::GetSpritesheet()
	{
		return m_Spritesheet;
	}

	std::pair<uint32_t, uint32_t> TilemapSprite::GetSize() const
	{
		return std::make_pair(m_Width, m_Height);
	}

	BlockBorders TilemapSprite::GetBlockBorders()
	{
		return m_BlockBorders;
	}

}
