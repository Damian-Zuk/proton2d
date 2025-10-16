#include "Proton/Graphics/Sprite.h"

#include <unordered_map>

namespace proton {

	class AssetManager
	{
	public:
		static void Init();
		static void ReloadAssetsList();

		static Shared<Texture> LoadTexture(const std::string& filepath);
		static Shared<Texture> GetTexture(const std::string& filepath);
		static bool UnloadTexture(const std::string& filepath);
		static bool IsTextureLoaded(const std::string& filepath);

		static Shared<TextureAtlas> LoadTextureAtlas(const std::string& filepath);
		static Shared<TextureAtlas> GetTextureAtlas(const std::string& filepath);
		static bool UnloadTextureAtlas(const std::string& filepath);
		static bool IsTextureAtlasLoaded(const std::string& filepath);

	private:
		static AssetManager* s_Instance;

		std::unordered_map<std::string, Shared<Texture>> m_Textures;
		std::unordered_map<std::string, Shared<TextureAtlas>> m_TextureAtlases;

		std::vector<std::string> m_TexturesFilepathList;
		std::unordered_map<std::string, glm::uvec2> m_TextureAtlasList;

		friend class InspectorPanel;
	};

}
