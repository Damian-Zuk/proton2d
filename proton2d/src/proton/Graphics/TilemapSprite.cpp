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
		m_Width = width; m_Height = height;
		m_Tilemap.resize(width);
		for (auto& column : m_Tilemap)
			column.resize(height, TILEMAP_BLANK_TILE);
	}

	void TilemapSprite::GenerateTilemapBlock()
	{
		bool left        = m_BlockBorders & BlockBorder_Left;
		bool right       = m_BlockBorders & BlockBorder_Right;
		bool top         = m_BlockBorders & BlockBorder_Top;
		bool bottom      = m_BlockBorders & BlockBorder_Bottom;
		bool topLeft     = m_BlockBorders & BlockBorder_TopLeft;
		bool topRight    = m_BlockBorders & BlockBorder_TopRight;
		bool bottomLeft  = m_BlockBorders & BlockBorder_BottomLeft;
		bool bottomRight = m_BlockBorders & BlockBorder_BottomRight;

		if (m_Width == 1 && m_Height == 1) // 1x1 size
		{
			m_Tilemap[0][0] = { 5, 5 };
		}
		else if (m_Width == 1) // Nx1 size
		{
			for (uint32_t y = 0; y < m_Height; y++)
			{
				if (y == 0)
					m_Tilemap[0][y] = { 5, 0 };
				else if (y == m_Height - 1)
					m_Tilemap[0][y] = { 5, 3 };
				else
					m_Tilemap[0][y] = { 5, 1 + y % 2 };
			}
		}
		else if (m_Height == 1) // 1xN size
		{
			for (uint32_t x = 0; x < m_Width; x++)
			{
				if (x == 0)
					m_Tilemap[x][0] = { 0, 5 };
				else if (x == m_Width - 1)
					m_Tilemap[x][0] = { 3, 5 };
				else
					m_Tilemap[x][0] = { 1 + x % 2, 5 };
			}
		}
		else // NxN size
		{
			// Center
			for (uint32_t y = 1; y < m_Height - 1; y++)
				for (uint32_t x = 1; x < m_Width - 1; x++)
					m_Tilemap[x][y] = { (1 + x % 2), (1 + y % 2) };

			glm::uvec2 tilePos;

			// Left edge
			for (uint32_t y = 0; y < m_Height; y++)
			{
				if (y == 0) // Bottom left corner
				{
					if (!bottomLeft)
						tilePos = { 1, 1 + y % 2 };
					else if (!left)
						tilePos = { 2, 0 };
					else
						tilePos = { 0, 0 };
				}
				else if (y == m_Height - 1) // Top left corner
				{
					if (!topLeft)
						tilePos = { 1, 1 + y % 2 };
					else if (!left)
						tilePos = { 2, 3 };
					else
						tilePos = { 0, 3 };
				}
				else
				{
					if (!left)
						tilePos = { 1, 1 + y % 2 };
					else
						tilePos = { 0, 1 + y % 2 };
				}
				m_Tilemap[0][y] = tilePos;
			}

			// Right edge
			for (uint32_t y = 0; y < m_Height; y++)
			{
				if (y == 0) // Bottom right corner
				{
					if (!bottomRight)
						tilePos = { 2, 1 + y % 2 };
					else if (!right)
						tilePos = { 1 + (m_Width - 1) % 2, 0 };
					else
						tilePos = { 3, 0 };
				}
				else if (y == m_Height - 1) // Top right corner
				{
					if (!topRight)
						tilePos = { 2, 1 + y % 2 };
					else if (!right)
						tilePos = { 1 + (m_Width - 1) % 2, 3 };
					else
						tilePos = { 3, 3 };
				}
				else
				{
					if (!right)
						tilePos = { 1 + (m_Width - 1) % 2, 1 + y % 2 };
					else
						tilePos = { 3, 1 + y % 2 };
				}
				m_Tilemap[m_Width - 1][y] = tilePos;
			}

			// Top edge
			for (uint32_t x = 0; x < m_Width; x++)
			{
				if (x == 0)
				{
					if (!top && topLeft)
						m_Tilemap[x][m_Height - 1] = { 0, 1 + (m_Height - 1) % 2 };
				}
				else if (x == m_Width - 1)
				{
					if (!top && topRight)
						m_Tilemap[x][m_Height - 1] = { 3, 1 + (m_Height - 1) % 2 };
				}
				else
				{
					if (!top)
						tilePos = { 1 + x % 2, 1 + (m_Height - 1) % 2 };
					else
						tilePos = { 1 + x % 2, 3 };

					m_Tilemap[x][m_Height - 1] = tilePos;
				}
			}

			// Bottom edge
			for (uint32_t x = 0; x < m_Width; x++)
			{
				if (x == 0)
				{
					if (!bottom && bottomLeft)
						m_Tilemap[x][0] = { 0, 1 + (m_Height - 1) % 2 };
				}
				else if (x == m_Width - 1)
				{
					if (!bottom && bottomRight)
						m_Tilemap[x][0] = { 3, 1 + (m_Height - 1) % 2 };
				}
				else
				{
					if (!bottom)
						tilePos = { 1 + x % 2, 1 };
					else
						tilePos = { 1 + x % 2, 0 };
					m_Tilemap[x][0] = tilePos;
				}
			}
		}
	}

	void TilemapSprite::SetBlockBorders(BlockBorders borders)
	{
		m_BlockBorders = borders;
	}

	Shared<SpriteSheet> TilemapSprite::GetSpritesheet()
	{
		return m_Spritesheet;
	}

	std::pair<uint32_t, uint32_t> TilemapSprite::GetSize() const
	{
		return std::pair<uint32_t, uint32_t>(m_Width, m_Height);
	}

	uint8_t TilemapSprite::GetBlockBorders()
	{
		return m_BlockBorders;
	}

}
