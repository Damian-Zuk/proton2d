#pragma once

#include "proton/Graphics/OpenGL/Texture.h"

#include <glm/glm.hpp>

namespace proton {

	// [0] - bottom left (0,0)
	// [1] - bottom right (1,0)
	// [2] - top right (1,1)
	// [3] - top left (0,1)
	using TextureCoords = std::array<glm::vec2, 4>;

	class SpriteSheet
	{
	public:
		SpriteSheet(const Shared<Texture>& texture, uint32_t tileWidth, uint32_t tileHeight);

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

		glm::uvec2 m_SheetSize; // pixels
		glm::uvec2 m_TileSize; // pixels
		glm::uvec2 m_TileCount; // sheet size / tile size
		glm::vec2 m_TileScale; // 0.0 - 1.0

		friend class Inspector;
		friend class Sprite;
		friend class NineSliceSprite;
		friend class Scene;
	};

	class Sprite
	{
	public:
		Sprite() = default;
		Sprite(const Shared<Texture>& texture);
		Sprite(const Shared<SpriteSheet>& spriteSheet);
		Sprite(const Sprite& other);

		void SetTexture(const Shared<Texture>& texture);
		void SetSpriteSheet(const Shared<SpriteSheet>& spriteSheet);

		void SetTile(uint32_t x, uint32_t y, uint32_t sizeX = 1, uint32_t sizeY = 1);
		void SetTileX(uint32_t x, uint32_t sizeX = 1, uint32_t sizeY = 1);
		void SetTileY(uint32_t y, uint32_t sizeX = 1, uint32_t sizeY = 1);
		const glm::uvec2& GetTilePos() const { return m_TilePos; }

		void NextTile(uint32_t posY = 0, uint32_t sizeX = 1, uint32_t sizeY = 1);
		void PrevTile(uint32_t posY = 0, uint32_t sizeX = 1, uint32_t sizeY = 1);

		void MirrorFlip(bool mirror_x, bool mirror_y);

		Shared<Texture> GetTexture();

	private:
		const TextureCoords& GetTextureCoords();
		
	private:
		Shared<Texture> m_Texture = nullptr;
		Shared<SpriteSheet> m_SpriteSheet = nullptr;
		
		glm::uvec2 m_PixelSize;
		glm::uvec2 m_TilePos;
		glm::uvec2 m_TileSize;
		bool m_MirrorFlipX = false, m_MirrorFlipY = false;

		friend class Renderer;
		friend class Scene;
		friend class Inspector;
		friend class EditorOverlay;
		friend class SceneSerializer;
		friend class Entity;
	};

}