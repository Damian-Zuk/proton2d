#pragma once
#include "proton/Graphics/Sprite.h"

#define TILEMAP_BLANK_TILE glm::uvec2{ -1, -1 }

namespace proton {

	enum BlockBorder : uint16_t
	{
		BlockBorder_Left        = 1 << 0,
		BlockBorder_Right       = 1 << 1,
		BlockBorder_Top         = 1 << 2,
		BlockBorder_Bottom      = 1 << 3,
		BlockBorder_TopLeft     = 1 << 4,
		BlockBorder_TopRight    = 1 << 5,
		BlockBorder_BottomLeft  = 1 << 6,
		BlockBorder_BottomRight = 1 << 7,
		BlockBorder_All         = 0xFF
	};

	using BlockBorders = uint8_t;

	class NineSliceSprite
	{
	public:
		NineSliceSprite();

		void SetSpritesheet(const Shared<SpriteSheet>& spritesheet);
		Shared<SpriteSheet> GetSpritesheet();

		// Set size in tile count
		void SetSize(uint32_t width, uint32_t height);
		std::pair<uint32_t, uint32_t> GetSize() const;

		// Set sliced sprite position inside spritesheet 
		// Bottom left corner is (0, 0)
		void SetSlicedSpritePosition(const glm::uvec2& position);
		const glm::uvec2& GetSlicedSpritePosition() const { return m_SlicedSpritePos; };

		void GenerateSliceScaledSprite();

		void SetBlockBorders(BlockBorders borders);
		BlockBorders GetBlockBorders();

	private:
		Shared<SpriteSheet> m_Spritesheet = nullptr;
		uint32_t m_Width = 1, m_Height = 1;

		std::vector<std::vector<glm::uvec2>> m_Tilemap; // [x][y]

		// Slice scaled sprites
		glm::uvec2 m_SlicedSpritePos = { 0, 0 };
		BlockBorders m_BlockBorders = BlockBorder_All;

		friend class Scene;
		friend class Inspector;
		friend class SceneSerializer;
	};

}