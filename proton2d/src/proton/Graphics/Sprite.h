#pragma once

#include "proton/Graphics/Texture.h"

#include <glm/glm.hpp>

namespace proton {

	class SpriteSheet
	{
	public:
		SpriteSheet(const Shared<Texture>& texture, uint32_t tileWidth, uint32_t tileHeight);

		Shared<Texture> GetTexture();

		std::pair<uint32_t, uint32_t> GetSheetSize();
		std::pair<uint32_t, uint32_t> GetTileSize();

	private:
		Shared<Texture> m_Texture;
		uint32_t m_SheetWidth; uint32_t m_SheetHeight;
		uint32_t m_TileWidth; uint32_t m_TileHeight;
	};

	class Sprite
	{
	public:
		Sprite(const Shared<Texture>& texture);
		Sprite(const Shared<SpriteSheet>& spriteSheet, uint32_t x = 0, uint32_t y = 0, uint32_t sizeX = 1, uint32_t sizeY = 1);

		void SetSpriteSheet(const Shared<SpriteSheet>& spriteSheet);
		void SetTile(uint32_t x, uint32_t y, uint32_t sizeX = 1, uint32_t sizeY = 1);

		Shared<Texture> GetTexture();
		const glm::mat4x2& GetTextureCoords();

	private:
		uint32_t m_PosX = 0; uint32_t m_PosY = 0;
		uint32_t m_SizeX = 1; uint32_t m_SizeY = 1;
		Shared<SpriteSheet> m_SpriteSheet = nullptr;
		Shared<Texture> m_Texture;
		glm::mat4x2 m_TextureCoords = glm::mat4x2(0.0f);
	};

}