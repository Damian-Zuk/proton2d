#include "proton/Graphics/Sprite.h"

#include <unordered_map>

namespace proton {

	class AssetsManager
	{
	public:
		static bool TextureExists(const std::string& keyPath);
		static bool SpriteSheetExists(const std::string& keyPath);

		static Shared<Texture> LoadTexture(const std::string& filepath);
		static Shared<SpriteSheet> LoadSpriteSheet(const std::string& filepath, uint32_t tileWidth, uint32_t tileHeight);

		// Texture auto-loaded
		static Shared<Texture> GetTexture(const std::string& keyPath); 
		// Texture not auto-loaded
		static Shared<SpriteSheet> GetSpriteSheet(const std::string& keyPath); 
	};

}
