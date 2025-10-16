#pragma once
#include "Proton/Graphics/Sprite.h"

namespace proton {

	enum ResizableSprite_Edge : uint16_t
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

	struct ResizableSpriteCell
	{
		glm::mat4 LocalTransform;
		TextureCoords TextureCoords;
	};

	// 9-slice scaled sprite
	// TODO: Render tiles in shader instead
	class ResizableSprite
	{
	public:
		ResizableSprite() = default;

		void Generate(const glm::vec2& transformScale);
		void Render(const glm::mat4& transform, const glm::vec4& color);

		void SetTextureAtlas(const Shared<TextureAtlas>& textureAtlas);
		Shared<TextureAtlas> GetTextureAtlas() const { return m_TextureAtlas; }

		void SetCellScale(float tileScale);
		float GetSetScale() const { return m_CellScale; }

		// Set pattern offset position in a source textureAtlas
		// Relative to Bottom Left corner: (0, 0) 
		void SetPositionOffset(const glm::uvec2& position);
		const glm::uvec2& GetPositionOffset() const { return m_PatternOffset; };

		// Each bit defines if cell at sprite border has a center slice texture
		void SetEdgesBitset(uint8_t edgesBitset);
		uint8_t GetEdgesBitset() const { return m_EdgesBitset; }

	private:
		std::vector<glm::uvec2> CalculateTextureAtlasCellIndexPositions();
		void CalculateCellTransforms(const std::vector<glm::uvec2>& textureAtlasIndexes);

	private:
		Shared<TextureAtlas> m_TextureAtlas = nullptr;

		float m_CellScale = 1.0f;
		glm::uvec2 m_PatternSize = { 3, 3 };
		glm::uvec2 m_PatternOffset = { 0, 0 };
		uint8_t m_EdgesBitset = Edge_All; // bitset

		glm::vec2 m_TransformScale = { 1.0f, 1.0f };
		glm::uvec2 m_CellCount = { 0, 0 };
		glm::uvec2 m_PixelSize = { 0, 0 };

		std::vector<ResizableSpriteCell> m_CellTransforms;

		friend class Scene;
		friend class InspectorPanel;
		friend class SceneSerializer;
		friend class Entity;
	};

}
