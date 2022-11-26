#pragma once

#include "proton/Graphics/OpenGL/Texture.h"

#include <glm/glm.hpp>

namespace proton {

	using TextureCoords = std::array<glm::vec2, 4>;

	class SpriteSheet
	{
	public:
		SpriteSheet(const Shared<Texture>& texture, uint32_t tileWidth, uint32_t tileHeight);

		Shared<Texture> GetTexture();

		std::pair<uint32_t, uint32_t> GetSheetSize() const;
		std::pair<uint32_t, uint32_t> GetTileSize() const;
		std::pair<uint32_t, uint32_t> GetMaxTilesCount() const;

		const TextureCoords& GetTextureCoords(uint32_t x, uint32_t y) const;

	private:
		std::vector<std::vector<TextureCoords>> m_TextureCoords;
		Shared<Texture> m_Texture;

		uint32_t m_SheetWidth = 0, m_SheetHeight = 0;
		uint32_t m_TileWidth = 0, m_TileHeight = 0;
		uint32_t m_MaxTilesX = 0, m_MaxTilesY = 0;

		friend class Inspector;
		friend class Sprite;
		friend class Scene;
	};

	class Sprite
	{
	public:
		Sprite(const Shared<Texture>& texture);
		Sprite(const Shared<SpriteSheet>& spriteSheet, uint32_t x = 0, uint32_t y = 0, uint32_t sizeX = 1, uint32_t sizeY = 1);

		void SetSpriteSheet(const Shared<SpriteSheet>& spriteSheet);
		void SetTile(uint32_t x, uint32_t y, uint32_t sizeX = 1, uint32_t sizeY = 1);
		void NextTile(uint32_t posY = 0, uint32_t sizeX = 1, uint32_t sizeY = 1);
		void PrevTile(uint32_t posY = 0, uint32_t sizeX = 1, uint32_t sizeY = 1);

		void FlipX(bool flip = true);
		void FlipY(bool flip = true);

		Shared<Texture> GetTexture();

	private:
		const TextureCoords& GetTextureCoords();
		
	private:
		Shared<Texture> m_Texture;
		Shared<SpriteSheet> m_SpriteSheet = nullptr;

		uint32_t m_Width_px = 1, m_Height_px = 1;
		uint32_t m_PosX = 0, m_PosY = 0;
		uint32_t m_SizeX = 1, m_SizeY = 1;
		bool m_FlipX = false, m_FlipY = false;

		friend class Renderer;
		friend class Scene;
		friend class Inspector;
		friend class SceneSerializer;
	};

}