#include "proton/Graphics/Sprite.h"

#include <unordered_map>
#include <nlohmann/json.hpp>

namespace proton {

	using json = nlohmann::ordered_json;

	class AssetsManager
	{
	public:
		static void Init();

		static bool TextureExists(const std::string& filepath);
		static bool SpriteSheetExists(const std::string& filepath);

		// Loads texture and stores using filepath as key
		static Shared<Texture> LoadTexture(const std::string& filepath);
		// Returns texture pointer. If it isn't loaded then tries to load it first
		static Shared<Texture> GetTexture(const std::string& filepath);
		// Delete texture and freess memory
		static bool UnloadTexture(const std::string& filepath);

		static Shared<SpriteSheet> LoadSpriteSheet(const std::string& filepath);
		static Shared<SpriteSheet> GetSpriteSheet(const std::string& filepath);
		static bool UnloadSpritesheet(const std::string& filepath);

		static void ReloadAssetsList();

	private:
		static AssetsManager* s_Instance;

		std::unordered_map<std::string, Shared<Texture>> m_Textures;
		std::unordered_map<std::string, Shared<SpriteSheet>> m_Spritesheets;

		std::vector<std::string> m_TexturesFilepathList;
		//std::vector<SpritesheetInfo> m_SpritesheetList;
		std::unordered_map<std::string, glm::uvec2> m_SpritesheetList;

		friend class Inspector;
	};

}
