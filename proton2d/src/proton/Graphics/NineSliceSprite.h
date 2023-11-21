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
	struct TransformComponent;

	struct NineSliceTile
	{
		glm::mat4 LocalTransform;
		TextureCoords Coords;
	};

	class NineSliceSprite
	{
	public:
		NineSliceSprite() = default;

		void SetSpritesheet(const Shared<Spritesheet>& spritesheet);
		Shared<Spritesheet> GetSpritesheet();

		// If size, block borders or position offset has changed
		// regenerated sprite data
		void Refresh();

		// Sets scale of indivudual tiles
		void SetTileScale(float tileScale);
		float GetTileScale() const { return m_TileScale; }

		// Set sliced sprite position inside spritesheet 
		// Bottom left corner is (0, 0)
		void SetPositionOffset(const glm::uvec2& position);
		const glm::uvec2& GetPositionOffset() const { return m_PositionOffset; };

		void SetBlockBorders(BlockBorders borders);
		BlockBorders GetBlockBorders();

	private:
		// Calculate tile positions and local transformation matrix for each tile
		void CalculateTilesLocalTransform();

		// Calculate tile index positions from source spirtesheet
		std::vector<std::vector<glm::uvec2>> CalculateTilePositions();

	private:
		std::vector<std::vector<NineSliceTile>> m_Tilemap; // [x][y]

		TransformComponent* m_Transform = nullptr;
		Shared<Spritesheet> m_Spritesheet = nullptr;
		uint32_t m_Width = 0, m_Height = 0;
		float m_TileScale = 1.0f;

		// Slice scaled sprites
		glm::uvec2 m_PositionOffset = { 0, 0 };
		BlockBorders m_BlockBorders = BlockBorder_All;

		friend class Scene;
		friend class InspectorPanel;
		friend class SceneSerializer;
		friend class Entity;
	};

}