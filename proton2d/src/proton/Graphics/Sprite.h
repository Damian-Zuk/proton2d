#pragma once

#include "proton/Graphics/Texture.h"

#include <glm/glm.hpp>

namespace proton {

	class SpriteSheet
	{
	public:
		SpriteSheet(const Shared<Texture>& texture, uint32_t tileWidth, uint32_t tileHeight);

		Shared<Texture> GetTexture();

		std::pair<uint32_t, uint32_t> GetSheetSize() const;
		std::pair<uint32_t, uint32_t> GetTileSize() const;
		std::pair<uint32_t, uint32_t> GetMaxTilesCount() const;

	private:
		Shared<Texture> m_Texture;
		uint32_t m_SheetWidth = 0, m_SheetHeight = 0;
		uint32_t m_TileWidth = 0, m_TileHeight = 0;
		uint32_t m_MaxTilesX = 0, m_MaxTilesY = 0;

		friend class Inspector;
	};

	class Sprite
	{
	public:
		Sprite(const Shared<Texture>& texture);
		Sprite(const Shared<SpriteSheet>& spriteSheet, uint32_t x = 0, uint32_t y = 0, uint32_t sizeX = 1, uint32_t sizeY = 1);

		void SetSpriteSheet(const Shared<SpriteSheet>& spriteSheet);
		void SetTile(uint32_t x, uint32_t y, uint32_t sizeX = 1, uint32_t sizeY = 1);

		Shared<Texture> GetTexture();
		const std::array<glm::vec2, 4>& GetTextureCoords() const;

	private:
		uint32_t m_PosX = 0, m_PosY = 0;
		uint32_t m_SizeX = 1, m_SizeY = 1;
		Shared<SpriteSheet> m_SpriteSheet = nullptr;
		Shared<Texture> m_Texture;
		std::array<glm::vec2, 4> m_TextureCoords;

		friend class Inspector;
	};

}