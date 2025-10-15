#include "ptpch.h"
#include "Proton/Graphics/Sprite.h"
#include "Proton/Core/AssetManager.h"

namespace proton {

	constexpr TextureCoords DEFAULT_TEXTURE_COORDS = { {{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }} };

	Sprite::Sprite(Shared<Texture> texture)
		: m_Texture(texture), m_PixelSize({ texture->GetWidth(), texture->GetHeight() }),
		m_TextureCoords(DEFAULT_TEXTURE_COORDS)
	{
		CalculateTextureCoords();
	}

	Sprite::Sprite(Shared<TextureAtlas> textureAtlas)
		: m_TextureAtlas(textureAtlas), m_Texture(textureAtlas->GetTexture()),
		m_PixelSize(textureAtlas->m_TileSizePx), m_TextureCoords(DEFAULT_TEXTURE_COORDS)
	{
		CalculateTextureCoords();
	}

	void Sprite::CalculateTextureCoords()
	{
		// No textureAtlas = full texture coords
		if (!m_TextureAtlas)
		{
			m_TextureCoords = { {{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }} };
			return;
		}

		// Single tile (1x1) coords
		if (m_TileSize.x == 1 && m_TileSize.y == 1)
		{
			m_TextureCoords = m_TextureAtlas->GetTextureCoords(m_TileIndex.x, m_TileIndex.y);;
			return;
		}

		// NxN (tiles) size texture coords
		glm::uvec2 s = m_TileIndex + m_TileSize; // top right tile position index
		// Cap index to prevent going out of bounds
		s.x = glm::min(s.x - 1, m_TextureAtlas->m_TileCount.x - 1);
		s.y = glm::min(s.y - 1, m_TextureAtlas->m_TileCount.y - 1);

		m_TextureCoords = m_TextureAtlas->GetTextureCoords(m_TileIndex.x, m_TileIndex.y); // bottom left
		const TextureCoords& topRightCoords = m_TextureAtlas->GetTextureCoords(s.x, s.y); // top right

		// Change bottom right x 
		m_TextureCoords[1].x = topRightCoords[1].x;
		// Change top right x and y 
		m_TextureCoords[2] = topRightCoords[2];
		// Change top left y 
		m_TextureCoords[3].y = topRightCoords[3].y;
	}

	const TextureCoords& Sprite::GetTextureCoords() const
	{
		return m_TextureCoords;
	}

	void Sprite::SetTextureFromPath(const std::string& filepath)
	{
		SetTexture(AssetManager::GetTexture(filepath));
	}

	void Sprite::SetTexture(Shared<Texture> texture)
	{
		m_Texture = texture;
		m_TextureAtlas = nullptr;
		m_PixelSize = { texture->GetWidth(), texture->GetHeight() };
		CalculateTextureCoords();
	}

	void Sprite::SetTextureAtlas(Shared<TextureAtlas> textureAtlas)
	{
		m_TextureAtlas = textureAtlas;
		m_Texture = textureAtlas->GetTexture();
		m_PixelSize = m_TileSize * textureAtlas->m_TileSizePx;
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
		m_TileSize = size;
		CalculateTextureCoords();
	}

	void Sprite::SetTileCount(uint32_t width, uint32_t height)
	{
		SetTileCount({ width, height });
	}

	void Sprite::SetTileCountX(uint32_t width)
	{
		SetTileCount({ width, m_TileSize.y });
	}

	void Sprite::SetTileCountY(uint32_t height)
	{
		SetTileCount({ m_TileSize.x, height });
	}

	void Sprite::SetMirrorFlip(glm::bvec2 flip)
	{
		m_MirrorFlip.x = flip.x;
		m_MirrorFlip.y = flip.y;
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
