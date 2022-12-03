#pragma once
#include "proton/Graphics/Sprite.h"

#define TILEMAP_BLANK_TILE glm::ivec2{ -1, -1 }

namespace proton {

	class TilemapSprite
	{
	public:
		TilemapSprite();

		void SetSpritesheet(const Shared<SpriteSheet>& spritesheet);

		/*
			Set size in tile count
		*/
		void SetSize(uint32_t width, uint32_t height);

		void GenerateTilemapBlock();

		/*
			Each byte defines if tilemap edge tiles texture
			is changed to texture of the center of spritesheet block
			
			Byte 0 - Left edge
			Byte 1 - Right edge
			Byte 2 - Top edge
			Byte 3 - Bottom edge
			Byte 4 - Top left corner
			Byte 5 - Top right corner
			Byte 6 - Bottom left corner
			Byte 7 - bottom right corner
		*/
		void SetBlockEdges(uint8_t edges);

		Shared<SpriteSheet> GetSpritesheet();
		std::pair<uint32_t, uint32_t> GetSize();
		uint8_t GetBlockEdges();

	private:
		Shared<SpriteSheet> m_Spritesheet = nullptr;
		uint32_t m_Width = 1, m_Height = 1;
		std::vector<std::vector<glm::ivec2>> m_Tilemap; // [x][y]
		uint8_t m_BlockEdges = 0xff;

		friend class Scene;
		friend class Inspector;
	};

}