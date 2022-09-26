#include "proton/Graphics/Sprite.h"

#include <unordered_map>

namespace proton {

	class AssetsManager
	{
	public:
		AssetsManager();

		static AssetsManager& Get();

		bool TextureExists(const std::string& keyPath);
		bool SpriteSheetExists(const std::string& keyPath);

		Shared<Texture> LoadTexture(const std::string& filepath);
		Shared<SpriteSheet> LoadSpriteSheet(const std::string& filepath, uint32_t tileWidth, uint32_t tileHeight);

		// Texture auto-loaded
		Shared<Texture> GetTexture(const std::string& keyPath); 
		// Texture not auto-loaded
		Shared<SpriteSheet> GetSpriteSheet(const std::string& keyPath); 

	private:
		static AssetsManager* s_Instance;
		std::unordered_map<std::string, Shared<Texture>> m_Textures;
		std::unordered_map<std::string, Shared<SpriteSheet>> m_SpriteSheets;
	};

}
