#include "ptpch.h"
#include "Proton/Graphics/ResizableSprite.h"
#include "Proton/Scene/Components.h"
#include "Proton/Utils/Utils.h"
#include "Proton/Scene/Entity.h"
#include "Proton/Graphics/Renderer/Renderer.h"

namespace proton {

	void ResizableSprite::SetSpritesheet(const Shared<Spritesheet>& spritesheet, Entity* owningEntity)
	{
		m_Spritesheet = spritesheet;
		Generate(owningEntity);
	}

	void ResizableSprite::Generate(Entity* owningEntity)
	{
		auto& transform = owningEntity->GetTransform();
		m_Scale = transform.Scale;

		if (m_Scale.x < 0.0f || m_Scale.y < 0.0f)
		{
			m_Width = 0; m_Height = 0;
			return;
		}

		m_Width = std::max((uint32_t)ceil(m_Scale.x / m_TileScale), 2u);
		m_Height = std::max((uint32_t)ceil(m_Scale.y / m_TileScale), 2u);
		
		m_PixelSize = {
			m_Scale.x * m_TileScale * m_Spritesheet->m_TileSize.x,
			m_Scale.y * m_TileScale * m_Spritesheet->m_TileSize.y
		};

		CalculateTileTransforms();
	}

	void ResizableSprite::SetTileScale(float tileScale, Entity* owningEntity)
	{
		m_TileScale = tileScale < 0.01f ? 0.01f : tileScale;
		Generate(owningEntity);
	}

	// Generate tile index positions
	void ResizableSprite::DetermineTileIndexPositions(TilemapIndexPositions& tilemap)
	{
		tilemap.resize(m_Width * m_Height);

		uint16_t sx = m_PositionOffset.x, sy = m_PositionOffset.y;

		// Fill whole tilemap with center slices
		for (auto& tile : tilemap)
			tile = { sx + 1, sy + 1 };

		// Top left corner
		if (m_Edges & Edge_TopLeft)
		{
			tilemap[(m_Height - 1) * m_Width] = {sx, sy + 2};

			if (!(m_Edges & Edge_Left))
				tilemap[(m_Height - 1) * m_Width] = { sx + 1, sy + 2 };

			if (!(m_Edges & Edge_Top))
				tilemap[(m_Height - 1) * m_Width] = { sx, sy + 2 - 1 };
		}

		// Top right corner
		if (m_Edges & Edge_TopRight)
		{
			tilemap[m_Height * m_Width - 1] = { sx + 2, sy + 2 };

			if (!(m_Edges & Edge_Right))
				tilemap[m_Height * m_Width - 1] = { sx + 1, sy + 2 };

			if (!(m_Edges & Edge_Top))
				tilemap[m_Height * m_Width - 1] = { sx + 2, sy + 1 };
		}

		// Bottom left corner
		if (m_Edges & Edge_BottomLeft)
		{
			tilemap[0] = { sx, sy };

			if (!(m_Edges & Edge_Left))
				tilemap[0] = { sx + 1, sy };

			if (!(m_Edges & Edge_Bottom))
				tilemap[0] = { sx, sy + 1 };
		}

		// Bottom right corner
		if (m_Edges & Edge_BottomRight)
		{
			tilemap[m_Width - 1] = { sx + 2, sy };

			if (!(m_Edges & Edge_Right))
				tilemap[m_Width - 1] = { sx + 1, sy };

			if (!(m_Edges & Edge_Bottom))
				tilemap[m_Width - 1] = { sx + 2, sy + 1};
		}

		// Left border
		if (m_Edges & Edge_Left)
			for (uint32_t y = 1; y < m_Height - 1; y++)
				tilemap[y * m_Width] = { sx, sy + 1 };

		// Right border
		if (m_Edges & Edge_Right)
			for (uint32_t y = 1; y < m_Height - 1; y++)
				tilemap[m_Width - 1 + y * m_Width] = { sx + 2, sy + 1 };

		// Top border
		if (m_Edges & Edge_Top)
			for (uint32_t x = 1; x < m_Width - 1; x++)
				tilemap[x + (m_Height - 1) * m_Width] = {sx + 1, sy + 2};

		// Bottom border
		if (m_Edges & Edge_Bottom)
			for (uint32_t x = 1; x < m_Width - 1; x++)
				tilemap[x] = { sx + 1, sy };
	}

	void ResizableSprite::CalculateTileTransforms()
	{
		std::vector<ResizableSpriteTile> tilemap;
		tilemap.resize(m_Width * m_Height);

		// width, height tile count (with fraction)
		float width = m_Scale.x / m_TileScale;
		float height = m_Scale.y / m_TileScale;
		glm::uvec2 tileCount = { (uint32_t)ceil(width), (uint32_t)ceil(height) };
		
		// Offset means the size of a cut of the penultimate tile
		// If transform scale.x is for example: 4.85 and tilescale.x is 1.0
		// then offset.x will be equal to 0.15
		glm::vec2 offset = {
			fmod((1.0f - (width - (int)width)), 1.0f),
			fmod((1.0f - (height - (int)height)), 1.0f)
		};

		// Fix offset values for sprites with width or height < 2.0
		if (width < 2.0f)
		{
			offset.x /= 2.0f;
		}
		if (width <= 1.0f)
		{
			tileCount.x = 2;
			offset.x = 0.5f + (1.0f - width) / 2.0f;
		}
		if (height < 2.0f)
		{
			offset.y /= 2.0f;
		}
		if (height <= 1.0f)
		{
			tileCount.y = 2;
			offset.y = 0.5f + (1.0f - height) / 2.0f;
		}

		// Calculate texture coords offset
		glm::vec2 coordOffset = {
			m_Spritesheet->m_TileScale.x * offset.x,
			m_Spritesheet->m_TileScale.x * offset.y
		};

		// Generate spritesheet tile positions
		TilemapIndexPositions spritesheetTilePositions;
		DetermineTileIndexPositions(spritesheetTilePositions);

		for (uint32_t y = 0; y < tileCount.y; y++)
		{
			for (uint32_t x = 0; x < tileCount.x; x++)
			{
				// Tile position (in spritesheet)
				const glm::uvec2& tp = spritesheetTilePositions[x + y * m_Width];
				TextureCoords& coords = tilemap[x + y * m_Width].Coords;
				coords = m_Spritesheet->GetTextureCoords(tp.x, tp.y);

				glm::vec2 scale = { m_TileScale, m_TileScale };
				glm::vec2 pos = {
					(-width / 2.0f + x) * scale.x + 0.5f * m_TileScale,
					(-height / 2.0f + y) * scale.y + 0.5f * m_TileScale
				};
				// ^^^ Above are default position, scale, coords values
				// If offset is != 0.0 then some tiles must be cut, code below VVV

				// ========= X axis =========
				// Width > 2.0
				if (offset.x && width > 2.0f)
				{
					if (x == tileCount.x - 2) // One before last
					{
						coords[1].x -= coordOffset.x;
						coords[2].x -= coordOffset.x;
						scale.x *= (1.0f - offset.x);
						pos.x -= offset.x / 2.0f * m_TileScale;
					}
					else if (x == tileCount.x - 1) // Last
					{
						pos.x -= offset.x * m_TileScale;
					}
				}
				// Width < 2.0
				if (offset.x && width < 2.0f)
				{
					if (x == 0) // First
					{
						coords[1].x -= coordOffset.x;
						coords[2].x -= coordOffset.x;
						scale.x *= (1.0f - offset.x);
						pos.x -= offset.x / 2.0f * m_TileScale;
					}
					else // Second
					{
						coords[0].x += coordOffset.x;
						coords[3].x += coordOffset.x;
						scale.x *= (1.0f - offset.x);
						pos.x -= offset.x * 1.5f * m_TileScale;
					}
				}

				// ========= Y axis =========
				// Height > 2.0
				if (offset.y && height > 2.0f)
				{
					if (y == tileCount.y - 2) // One before last
					{
						coords[0].y += coordOffset.y;
						coords[1].y += coordOffset.y;
						scale.y *= (1.0f - offset.y);
						pos.y -= offset.y / 2.0f * m_TileScale;
					}
					else if (y == tileCount.y - 1) // Last
					{
						pos.y -= offset.y * m_TileScale;
					}
				}
				// Height < 2.0
				else if (offset.y && height < 2.0f)
				{
					if (y == 0) // First
					{
						coords[2].y -= coordOffset.y;
						coords[3].y -= coordOffset.y;
						scale.y *= (1.0f - offset.y);
						pos.y -= offset.y / 2.0f * m_TileScale;
					}
					else // Second
					{
						coords[0].y += coordOffset.y;
						coords[1].y += coordOffset.y;
						scale.y *= (1.0f - offset.y);
						pos.y -= offset.y * 1.5f * m_TileScale;
					}
				}

				// Store glm::mat4 tile transform matrix
				tilemap[x + y * m_Width].LocalTransform = Math::GetTransform(
					{ pos.x, pos.y, 0 }, { scale.x, scale.y }
				);
			}
		}

		RenderToFrameBuffer(tilemap);
	}

	void ResizableSprite::SetPositionOffset(const glm::uvec2& position)
	{
		if (position != m_PositionOffset)
		{
			m_PositionOffset = position;
			CalculateTileTransforms();
		}
	}

	void ResizableSprite::SetEdges(uint8_t edges)
	{
		if (edges != m_Edges)
		{
			m_Edges = edges;
			CalculateTileTransforms();
		}
	}

	Shared<Spritesheet> ResizableSprite::GetSpritesheet()
	{
		return m_Spritesheet;
	}

	uint8_t ResizableSprite::GetEdges() const
	{
		return m_Edges;
	}

	void ResizableSprite::RenderToFrameBuffer(const std::vector<ResizableSpriteTile>& tilemap)
	{
		if (!m_Spritesheet)
			return;

		FramebufferSpecification fbSpec;
		fbSpec.Attachments = {
			{ FramebufferTextureFormat::RGBA8, TextureFilterMode::Nearest },
		};
		fbSpec.Width = m_PixelSize.x;
		fbSpec.Height = m_PixelSize.y;
		m_Framebuffer.reset(new Framebuffer(fbSpec));

		m_Framebuffer->Bind();
		
		Camera camera;
		camera.m_AspectRatio = (float)m_PixelSize.x / (float)m_PixelSize.y;
		camera.m_OrthographicSize = glm::min(m_Scale.x, m_Scale.y);
		
		if (m_Scale.y > m_Scale.x)
			camera.m_OrthographicSize = m_Scale.y;

		camera.RecalculateProjection();
		
		Renderer::BeginScene(camera, glm::vec3{ 0.0f });
		Renderer::SetClearColor({ 1.0f, 1.0f, 1.0f, 0.0f });
		Renderer::Clear();
		for (const auto& tile : tilemap)
		{
			Renderer::DrawQuad(tile.LocalTransform, m_Spritesheet->GetTexture(), tile.Coords, glm::vec4{1.0f});
		}
		Renderer::EndScene();

		m_Texture.reset(new Texture(fbSpec.Width, fbSpec.Height, m_Framebuffer->GetColorAttachmentRendererID()));
		m_Framebuffer->Unbind();
	}

	void ResizableSprite::Render(const glm::mat4& transform, const glm::vec4& color)
	{
		if (!m_Texture)
			return;

		Renderer::DrawQuad(transform, m_Texture, color);
	}

}
