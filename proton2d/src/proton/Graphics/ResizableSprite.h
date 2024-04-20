#pragma once
#include "Proton/Graphics/Sprite.h"
#include "Proton/Graphics/Renderer/Framebuffer.h"

namespace proton {
	
	class Entity; // forward declaration
	class Framebuffer;
	struct TransformComponent;

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

	struct ResizableSpriteTile
	{
		TextureCoords Coords;
		glm::mat4 LocalTransform;
	};

	class ResizableSprite
	{
	public:
		ResizableSprite() = default;

		void SetSpritesheet(const Shared<Spritesheet>& spritesheet, Entity* owningEntity);
		Shared<Spritesheet> GetSpritesheet();

		// Generate sprite tilemap representing for each tile
		// - texture coords from source image (spritesheet)
		// - local transformation matrix (glm::mat4) for Renderer
		void Generate(Entity* owningEntity);

		// Set scale of indivudual tiles
		void SetTileScale(float tileScale, Entity* owningEntity);
		float GetTileScale() const { return m_TileScale; }

		// Set sliced sprite position inside spritesheet 
		// Bottom left corner is (0, 0)
		void SetPositionOffset(const glm::uvec2& position);
		const glm::uvec2& GetPositionOffset() const { return m_PositionOffset; };

		// Toggle sprite edges to be rendered as center pieces
		// Use Edge Enum to toggle specific bits representing edge/corner
		void SetEdges(uint8_t edges);
		uint8_t GetEdges() const;

	private:
		void RenderToFrameBuffer(const std::vector<ResizableSpriteTile>& tilemap);
		void Render(const glm::mat4& transform, const glm::vec4& color);

		// Calculate tile index positions in source Spritesheet
		using TilemapIndexPositions = std::vector<glm::uvec2>;
		void DetermineTileIndexPositions(TilemapIndexPositions& tilemap);

		// Calculate local transformation matrix (glm::mat4) for each tile
		void CalculateTileTransforms();

	private:
		Shared<Framebuffer> m_Framebuffer = nullptr;
		Shared<Texture> m_Texture = nullptr;
		Shared<Spritesheet> m_Spritesheet = nullptr;

		float m_TileScale = 1.0f;
		glm::vec2 m_Scale = { 1.0f, 1.0f };

		// Slice scaled sprites
		glm::uvec2 m_PositionOffset = { 0, 0 };
		uint8_t m_Edges = Edge_All;

		uint32_t m_Width = 0, m_Height = 0;
		glm::uvec2 m_PixelSize = { 0, 0 };

		friend class Scene;
		friend class InspectorPanel;
		friend class SceneSerializer;
		friend class Entity;
	};

}
