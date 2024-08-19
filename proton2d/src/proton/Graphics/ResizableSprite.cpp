#include "ptpch.h"
#include "Proton/Graphics/ResizableSprite.h"
#include "Proton/Scene/Components.h"
#include "Proton/Utils/Utils.h"
#include "Proton/Scene/Entity.h"
#include "Proton/Graphics/Renderer/Renderer.h"

namespace proton {

	void ResizableSprite::Generate(const glm::vec2& transformScale)
	{
		if (m_TransformScale.x < 0.0f || m_TransformScale.y < 0.0f)
			return;

		m_TransformScale = transformScale;

		m_CellCount = {
			m_PatternSize.x > 2 ? std::max((uint32_t)ceil(m_TransformScale.x / m_CellScale), 2u) : m_PatternSize.x,
			m_PatternSize.y > 2 ? std::max((uint32_t)ceil(m_TransformScale.y / m_CellScale), 2u) : m_PatternSize.y
		};

		m_PixelSize = {
			m_TransformScale.x * m_CellScale * m_Spritesheet->m_TileSize.x,
			m_TransformScale.y * m_CellScale * m_Spritesheet->m_TileSize.y
		};

		auto spritesheetIndexes = CalculateSpritesheetCellIndexPositions();
		CalculateCellTransforms(spritesheetIndexes);
	}

	void ResizableSprite::Render(const glm::mat4& transform, const glm::vec4& color)
	{
		if (!m_Spritesheet || m_PixelSize.x == 0 || m_PixelSize.y == 0)
			return;

		for (const auto& tile : m_CellTransforms)
		{
			Renderer::DrawQuad(transform * tile.LocalTransform, m_Spritesheet->GetTexture(), tile.TextureCoords, glm::vec4{ 1.0f });
		}
	}

	void ResizableSprite::SetSpritesheet(const Shared<Spritesheet>& spritesheet)
	{
		m_Spritesheet = spritesheet;
		Generate(m_TransformScale);
	}

	void ResizableSprite::SetCellScale(float cellScale)
	{
		m_CellScale = cellScale < 0.01f ? 0.01f : cellScale;
		Generate(m_TransformScale);
	}

	void ResizableSprite::SetPositionOffset(const glm::uvec2& position)
	{
		if (position != m_PatternOffset)
		{
			m_PatternOffset = position;
			Generate(m_TransformScale);
		}
	}

	void ResizableSprite::SetEdgesBitset(uint8_t edgesBitset)
	{
		if (edgesBitset != m_EdgesBitset)
		{
			m_EdgesBitset = edgesBitset;
			Generate(m_TransformScale);
		}
	}

	std::vector<glm::uvec2> ResizableSprite::CalculateSpritesheetCellIndexPositions()
	{
		std::vector<glm::uvec2> cells;

		uint16_t sx = m_PatternOffset.x, sy = m_PatternOffset.y;
		cells.resize(m_CellCount.x * m_CellCount.y);
		
		if (m_PatternSize.x == 1)
		{
			// Fill whole cells with center slices
			for (auto& cell : cells)
				cell = { sx, sy + 1 };

			if (m_EdgesBitset & Edge_TopLeft) // (0, 1)
				cells[0] = { sx, sy + 2 };

			if (m_EdgesBitset & Edge_BottomLeft) // (0, 0)
				cells[m_CellCount.x - 1] = { sx, sy };

			return cells;
		}

		if (m_PatternSize.y == 1)
		{
			// Fill whole cells with center slices
			for (auto& cell : cells)
				cell = { sx + 1 , sy };

			if (m_EdgesBitset & Edge_TopRight) // (0, 0)
				cells[0] = { sx, sy };

			if (m_EdgesBitset & Edge_TopLeft) // (1, 0)
				cells[m_CellCount.x - 1] = { sx + 2, sy };

			return cells;
		}

		// Fill whole cells with center slices
		for (auto& cell : cells)
			cell = { sx + 1, sy + 1 };

		const uint32_t bottomLeftIndex = 0; // (0, 0)
		const uint32_t bottomRightIndex = m_CellCount.x - 1; // (0, 1)
		const uint32_t topRightIndex = m_CellCount.y * m_CellCount.x - 1; // (1, 0)
		const uint32_t topLeftIndex = (m_CellCount.y - 1) * m_CellCount.x; // (1, 1)

		// Bottom left corner (0, 0)
		if (m_EdgesBitset & Edge_BottomLeft)
		{
			cells[bottomLeftIndex] = { sx, sy };

			if (!(m_EdgesBitset & Edge_Left))
				cells[bottomLeftIndex] = { sx + 1, sy };

			if (!(m_EdgesBitset & Edge_Bottom))
				cells[bottomLeftIndex] = { sx, sy + 1 };
		}

		// Bottom right corner (0, 1)
		if (m_EdgesBitset & Edge_BottomRight)
		{
			cells[bottomRightIndex] = { sx + 2, sy };

			if (!(m_EdgesBitset & Edge_Right))
				cells[bottomRightIndex] = { sx + 1, sy };

			if (!(m_EdgesBitset & Edge_Bottom))
				cells[bottomRightIndex] = { sx + 2, sy + 1};
		}

		// Top right corner (1, 0)
		if (m_EdgesBitset & Edge_TopRight)
		{
			cells[topRightIndex] = { sx + 2, sy + 2 };

			if (!(m_EdgesBitset & Edge_Right))
				cells[topRightIndex] = { sx + 1, sy + 2 };

			if (!(m_EdgesBitset & Edge_Top))
				cells[topRightIndex] = { sx + 2, sy + 1 };
		}

		// Top left corner (1, 1)
		if (m_EdgesBitset & Edge_TopLeft)
		{
			cells[topLeftIndex] = { sx, sy + 2 };

			if (!(m_EdgesBitset & Edge_Left))
				cells[topLeftIndex] = { sx + 1, sy + 2 };

			if (!(m_EdgesBitset & Edge_Top))
				cells[topLeftIndex] = { sx, sy + 1 };
		}

		// Left border
		if (m_EdgesBitset & Edge_Left)
			for (uint32_t y = 1; y < m_CellCount.y - 1; y++)
				cells[y * m_CellCount.x] = { sx, sy + 1 };

		// Right border
		if (m_EdgesBitset & Edge_Right)
			for (uint32_t y = 1; y < m_CellCount.y - 1; y++)
				cells[m_CellCount.x - 1 + y * m_CellCount.x] = { sx + 2, sy + 1 };

		// Top border
		if (m_EdgesBitset & Edge_Top)
			for (uint32_t x = 1; x < m_CellCount.x - 1; x++)
				cells[x + (m_CellCount.y - 1) * m_CellCount.x] = {sx + 1, sy + 2};

		// Bottom border
		if (m_EdgesBitset & Edge_Bottom)
			for (uint32_t x = 1; x < m_CellCount.x - 1; x++)
				cells[x] = { sx + 1, sy };

		return cells;
	}

	void ResizableSprite::CalculateCellTransforms(const std::vector<glm::uvec2>& spritesheetIndexes)
	{
		auto& cells = m_CellTransforms;
		cells.resize(m_CellCount.x * m_CellCount.y);

		float width = m_TransformScale.x / m_CellScale;
		float height = m_TransformScale.y / m_CellScale;

		if (m_PatternSize.x < 3)
			width = (float)m_PatternSize.x;

		if (m_PatternSize.y < 3)
			height = (float)m_PatternSize.y;
		
		// Offset - clipping size
		// e.g. For transform.scale.x = 4.85 and tilescale.x = 1.0
		// offset.x (clipping size) will be equal to 0.15
		glm::vec2 offset = {
			fmod((1.0f - (width - (int)width)), 1.0f),
			fmod((1.0f - (height - (int)height)), 1.0f)
		};

		// Offset for center clipping (when width or height < 2.0)
		if (m_PatternSize.x > 2)
		{
			if (width <= 1.0f)
			{
				m_CellCount.x = 2;
				offset.x = 0.5f + (1.0f - width) / 2.0f;
			} 
			else if (width < 2.0f)
				offset.x /= 2.0f;
		}
		if (m_PatternSize.y > 2)
		{
			if (height <= 1.0f)
			{
				m_CellCount.y = 2;
				offset.y = 0.5f + (1.0f - height) / 2.0f;
			}
			else if (height < 2.0f)
				offset.y /= 2.0f;
		}

		// Calculate offset for texture coords
		glm::vec2 coordOffset = {
			m_Spritesheet->m_TileScale.x * offset.x,
			m_Spritesheet->m_TileScale.x * offset.y
		};

		for (uint32_t y = 0; y < m_CellCount.y; y++)
		{
			for (uint32_t x = 0; x < m_CellCount.x; x++)
			{
				const glm::uvec2& tp = spritesheetIndexes.at(x + y * m_CellCount.x);
				TextureCoords& textureCoords = cells.at(x + y * m_CellCount.x).TextureCoords;
				textureCoords = m_Spritesheet->GetTextureCoords(tp.x, tp.y);

				// Default position, scale, coords values
				glm::vec2 scale = { m_CellScale, m_CellScale };
				glm::vec2 pos = {
					(-width / 2.0f + x) * scale.x + 0.5f * m_CellScale,
					(-height / 2.0f + y) * scale.y + 0.5f * m_CellScale
				};

				// Clip penultimate cells if offset is not 0
				if (offset.x && width > 2.0f)
				{
					// Penultimate clipping
					if (x == m_CellCount.x - 2) // Penultimate
					{
						textureCoords[1].x -= coordOffset.x;
						textureCoords[2].x -= coordOffset.x;
						scale.x *= (1.0f - offset.x);
						pos.x -= offset.x / 2.0f * m_CellScale;
					}
					else if (x == m_CellCount.x - 1) // Last
					{
						pos.x -= offset.x * m_CellScale;
					}
				}
				if (offset.x && width < 2.0f)
				{
					// Center clipping
					if (x == 0) // First
					{
						textureCoords[1].x -= coordOffset.x;
						textureCoords[2].x -= coordOffset.x;
						scale.x *= (1.0f - offset.x);
						pos.x -= offset.x / 2.0f * m_CellScale;
					}
					else // Second
					{
						textureCoords[0].x += coordOffset.x;
						textureCoords[3].x += coordOffset.x;
						scale.x *= (1.0f - offset.x);
						pos.x -= offset.x * 1.5f * m_CellScale;
					}
				}

				if (offset.y && height > 2.0f)
				{
					// Penultimate clipping
					if (y == m_CellCount.y - 2) // Penultimate
					{
						textureCoords[0].y += coordOffset.y;
						textureCoords[1].y += coordOffset.y;
						scale.y *= (1.0f - offset.y);
						pos.y -= offset.y / 2.0f * m_CellScale;
					}
					else if (y == m_CellCount.y - 1) // Last
					{
						pos.y -= offset.y * m_CellScale;
					}
				}
				else if (offset.y && height < 2.0f)
				{
					// Center clipping
					if (y == 0) // First
					{
						textureCoords[2].y -= coordOffset.y;
						textureCoords[3].y -= coordOffset.y;
						scale.y *= (1.0f - offset.y);
						pos.y -= offset.y / 2.0f * m_CellScale;
					}
					else // Second
					{
						textureCoords[0].y += coordOffset.y;
						textureCoords[1].y += coordOffset.y;
						scale.y *= (1.0f - offset.y);
						pos.y -= offset.y * 1.5f * m_CellScale;
					}
				}

				// Store glm::mat4 cell transform matrix
				cells.at(x + y * m_CellCount.x).LocalTransform = Math::GetTransform({ pos.x, pos.y, 0 }, { scale.x, scale.y });
			}
		}
	}

}
