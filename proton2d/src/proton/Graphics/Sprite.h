#pragma once

#include "proton/Graphics/OpenGL/Texture.h"

#include <glm/glm.hpp>

namespace proton {

	// [0] - bottom left (0,0)
	// [1] - bottom right (1,0)
	// [2] - top right (1,1)
	// [3] - top left (0,1)
	using TextureCoords = std::array<glm::vec2, 4>;

	class Spritesheet
	{
	public:
		Spritesheet(Shared<Texture> texture, uint32_t tileWidth, uint32_t tileHeight);

		// Returns pointer to OpenGL texture object
		Shared<Texture> GetTexture();

		// Get sheet size in pixels
		const glm::uvec2& GetSheetSize() const { return m_SheetSize; }
		// Get tile size in pixels
		const glm::uvec2& GetTileSize() const { return m_TileSize; }
		// Get max tiles count that can fit into spritesheet
		const glm::uvec2& GetTileCount() const { return m_TileCount; }

	private:
		const TextureCoords& GetTextureCoords(uint32_t x, uint32_t y) const;
		
		std::vector<std::vector<TextureCoords>> m_TextureCoords;
		Shared<Texture> m_Texture;

		glm::uvec2 m_SheetSize = { 0, 0 }; // pixels
		glm::uvec2 m_TileSize  = { 0, 0 }; // pixels
		glm::uvec2 m_TileCount = { 0, 0 }; // sheet size / tile size
		glm::vec2 m_TileScale  = { 0, 0 }; // 0.0f - 1.0f

		friend class InspectorPanel;
		friend class Sprite;
		friend class NineSliceSprite;
		friend class Scene;
	};

	class Sprite
	{
	public:
		Sprite() = default;
		Sprite(Shared<Texture> texture);
		Sprite(Shared<Spritesheet> spritesheet);

		// Sets pointer to texture object, use AssetManager to get texture
		void SetTexture(Shared<Texture> texture);
		// Sets pointer to spritesheet object, use AssetManager to get spritesheet
		void SetSpritesheet(Shared<Spritesheet> spritesheet);

		// Sets spritesheet tile position
		void SetTile(uint32_t x, uint32_t y);
		// Sets spritesheet tile X position
		void SetTileX(uint32_t x);
		// Sets spritesheet tile Y position
		void SetTileY(uint32_t y);
		// Returns spritesheet tile position
		const glm::uvec2& GetTilePos() const { return m_TilePos; }

		// Sets size in source texture (spritesheet) in tile count
		void SetTileSize(uint32_t tilesWidth, uint32_t tilesHeight);

		const glm::uvec2& GetTileSize() const { return m_TileSize; }

		// Sets sprite mirror flip
		void MirrorFlip(bool mirror_x, bool mirror_y);

		// Returns pointer to OpenGL texture object
		const Shared<Texture> GetTexture() const { return m_Texture; };

		float GetAspectRatio() const { return (float)m_PixelSize.x / (float)m_PixelSize.y; }

		operator bool() const { return m_Texture != nullptr; }

	private:
		void CalculateTextureCoords();
		const TextureCoords& GetTextureCoords() const;
		
	private:
		Shared<Texture> m_Texture = nullptr;
		Shared<Spritesheet> m_Spritesheet = nullptr;
		TextureCoords m_TextureCoords;
		
		glm::uvec2 m_PixelSize = { 0, 0 };
		glm::uvec2 m_TilePos   = { 0, 0 };
		glm::uvec2 m_TileSize  = { 1, 1 };
		bool m_MirrorFlipX = false, m_MirrorFlipY = false;

		friend class Renderer;
		friend class Scene;
		friend class InspectorPanel;
		friend class EditorLayer;
		friend class SceneSerializer;
		friend class Entity;
	};

}