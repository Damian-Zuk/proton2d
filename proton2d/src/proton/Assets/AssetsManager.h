#include "proton/Graphics/Sprite.h"

#include <unordered_map>

namespace proton {

	class AssetsManager
	{
	public:
		AssetsManager() = default;
		~AssetsManager() = default;

		static void Init();

		static bool TextureExists(const std::string& filepath);
		static bool SpriteSheetExists(const std::string& filepath);

		static Shared<Texture> LoadTexture(const std::string& filepath);
		static Shared<Texture> GetTexture(const std::string& filepath);
		static bool UnloadTexture(const std::string& filepath);

		static Shared<SpriteSheet> LoadSpriteSheet(const std::string& filepath, uint32_t tileWidth, uint32_t tileHeight);
		static Shared<SpriteSheet> GetSpriteSheet(const std::string& filepath);
		static bool UnloadSpritesheet(const std::string& filepath);

	private:
		static AssetsManager* s_Instance;

		std::unordered_map<std::string, Shared<Texture>> m_Textures;
		std::unordered_map<std::string, Shared<SpriteSheet>> m_Spritesheets;

		friend class Inspector;
	};

}
