#include "ptpch.h"
#include "Proton/Graphics/Sprite.h"
#include "Proton/Core/AssetManager.h"

namespace proton {

	Sprite::Sprite(const Shared<Texture>& texture)
		: m_Type(SpriteType::FullTexture), m_Texture(texture),
		m_PixelSize({ texture->GetWidth(), texture->GetHeight() })
	{
		CalculateTextureCoords();
	}

	Sprite::Sprite(const Shared<TextureAtlas>& textureAtlas)
		: m_Type(SpriteType::TextureAtlas), m_TextureAtlas(textureAtlas),
		m_Texture(textureAtlas), m_PixelSize({ textureAtlas->GetWidth(), textureAtlas->GetHeight() })
	{
		CalculateTextureCoords();
	}

	void Sprite::CalculateTextureCoords()
	{
		if (!m_TextureAtlas)
		{
			m_TextureCoords = FULL_TEXTURE_COORDS;
			return;
		}

		// 1x1 Tile
		if (m_TileCount.x == 1 && m_TileCount.y == 1)
		{
			m_TextureCoords = m_TextureAtlas->GetTextureCoords(m_TileIndex.x, m_TileIndex.y);;
			return;
		}

		// NxN Tiles
		glm::uvec2 s = m_TileIndex + m_TileCount;
		s.x = glm::min(s.x - 1, m_TextureAtlas->m_TileCount.x - 1);
		s.y = glm::min(s.y - 1, m_TextureAtlas->m_TileCount.y - 1);

		m_TextureCoords = m_TextureAtlas->GetTextureCoords(m_TileIndex.x, m_TileIndex.y); // bottom left
		const TextureCoords& topRightCoords = m_TextureAtlas->GetTextureCoords(s.x, s.y); // top right

		// Extend coords
		m_TextureCoords[1].x = topRightCoords[1].x;
		m_TextureCoords[2] = topRightCoords[2];
		m_TextureCoords[3].y = topRightCoords[3].y;
	}

	void Sprite::SetTextureFromPath(const std::string& filepath)
	{
		SetTexture(AssetManager::GetTexture(filepath));
	}

	void Sprite::SetTexture(const Shared<Texture>& texture)
	{
		m_Type = SpriteType::FullTexture;
		m_Texture = texture;
		m_TextureAtlas = nullptr;
		m_PixelSize = { texture->GetWidth(), texture->GetHeight() };
		CalculateTextureCoords();
	}

	void Sprite::SetTextureAtlas(const Shared<TextureAtlas>& textureAtlas)
	{
		m_Type = SpriteType::TextureAtlas;
		m_TextureAtlas = textureAtlas;
		m_Texture = textureAtlas;
		m_PixelSize = m_TileCount * textureAtlas->m_TileSizePx;
		CalculateTextureCoords();
	}

	void Sprite::SetTileIndex(glm::uvec2 index)
	{
		glm::uvec2 count = m_TextureAtlas->GetTileCount();
		index.x %= count.x; index.y %= count.y;
		m_TileIndex = index;
		CalculateTextureCoords();
	}

	void Sprite::SetTileIndex(uint32_t x, uint32_t y)
	{
		SetTileIndex({ x, y });
	}

	void Sprite::SetTileIndexX(uint32_t x)
	{
		SetTileIndex({ x, m_TileIndex.y });
	}

	void Sprite::SetTileIndexY(uint32_t y)
	{
		SetTileIndex({ m_TileIndex.x, y });
	}

	void Sprite::SetTileCount(glm::uvec2 size)
	{
		m_TileCount = size;
		CalculateTextureCoords();
	}

	void Sprite::SetTileCount(uint32_t width, uint32_t height)
	{
		SetTileCount({ width, height });
	}

	void Sprite::SetTileCountX(uint32_t width)
	{
		SetTileCount({ width, m_TileCount.y });
	}

	void Sprite::SetTileCountY(uint32_t height)
	{
		SetTileCount({ m_TileCount.x, height });
	}

	void Sprite::SetMirrorFlip(glm::bvec2 flip)
	{
		m_MirrorFlip = flip;
	}

	void Sprite::SetMirrorFlip(bool flip_x, bool flip_y)
	{
		m_MirrorFlip.x = flip_x;
		m_MirrorFlip.y = flip_y;
	}

	void Sprite::SetMirrorFlipX(bool flip_x)
	{
		m_MirrorFlip.x = flip_x;
	}

	void Sprite::SetMirrorFlipY(bool flip_y)
	{
		m_MirrorFlip.y = flip_y;
	}

}
