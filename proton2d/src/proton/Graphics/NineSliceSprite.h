#pragma once
#include "proton/Graphics/Sprite.h"

#define TILEMAP_BLANK_TILE glm::uvec2{ -1, -1 }

namespace proton {

	enum Edge : uint16_t
	{
		Edge_Left        = 1 << 0,
		Edge_Right       = 1 << 1,
		Edge_Top         = 1 << 2,
		Edge_Bottom      = 1 << 3,
		Edge_TopLeft     = 1 << 4,
		Edge_TopRight    = 1 << 5,
		Edge_BottomLeft  = 1 << 6,
		Edge_BottomRight = 1 << 7,
		Edge_All         = 0xFF
	};

	using Edges = uint8_t;
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

		void SetEdges(Edges borders);
		Edges GetEdges();

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
		Edges m_Edges = Edge_All;

		friend class Scene;
		friend class InspectorPanel;
		friend class SceneSerializer;
		friend class Entity;
	};

}