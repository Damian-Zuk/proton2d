#include "proton/Graphics/Sprite.h"

#include <unordered_map>
#include <nlohmann/json.hpp>

namespace proton {

	using json = nlohmann::ordered_json;

	class AssetManager
	{
	public:
		static void Init();

		// Loads texture and stores using filepath as key.
		static Shared<Texture> LoadTexture(const std::string& filepath);

		// Returns OpenGL Texture object pointer.
		// If texture isn't loaded then tries to load it.
		static Shared<Texture> GetTexture(const std::string& filepath);

		// Deletes OpenGL Texture object and frees up memory.
		static bool UnloadTexture(const std::string& filepath);

		// Checks if OpenGL Texture object is loaded in memory.
		static bool IsTextureLoaded(const std::string& filepath);

		// Loads spritesheet and stores using filepath as key.
		static Shared<Spritesheet> LoadSpritesheet(const std::string& filepath);

		// Returns Spritesheet object pointer.
		// If spritesheet isn't loaded then tries to load it.
		static Shared<Spritesheet> GetSpritesheet(const std::string& filepath);

		// Deletes Spritesheet object and frees up memory.
		static bool UnloadSpritesheet(const std::string& filepath);

		// Checks if Spritesheet object is loaded in memory.
		static bool IsSpritesheetLoaded(const std::string& filepath);

		// Reloads list of assets in "assets" directory.
		// Reloads spritesheet list from "spritesheets.json" file.
		static void ReloadAssetsList();

	private:
		static AssetManager* s_Instance;

		std::unordered_map<std::string, Shared<Texture>> m_Textures;
		std::unordered_map<std::string, Shared<Spritesheet>> m_Spritesheets;

		std::vector<std::string> m_TexturesFilepathList;
		std::unordered_map<std::string, glm::uvec2> m_SpritesheetList;

		friend class InspectorPanel;
	};

}
