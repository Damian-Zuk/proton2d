#include "ptpch.h"
#include "Proton/Core/AssetManager.h"
#include "Proton/Utils/Utils.h"

namespace proton {

	AssetManager* AssetManager::s_Instance = nullptr;

	void AssetManager::Init()
	{
		if (!s_Instance)
		{
			s_Instance = new AssetManager();
			ReloadAssetsList();
		}
	}

	Shared<Texture> AssetManager::LoadTexture(const std::string& filepath)
	{
		auto texture = MakeShared<Texture>("content/textures/" + filepath);
		if (!texture->IsLoaded()) 
		{
			PT_CORE_ERROR("Couldn't load texture '{}'", filepath);
			return nullptr;
		}

		//PT_CORE_INFO("file='{}'", filepath);
		s_Instance->m_Textures[filepath] = texture;
		return texture;
	}

	Shared<TextureAtlas> AssetManager::LoadTextureAtlas(const std::string& filepath)
	{
		auto texture = GetTexture(filepath);
		if (!texture)
			return nullptr;
		
		auto& textureAtlasList = s_Instance->m_TextureAtlasList;
		if (textureAtlasList.find(filepath) == textureAtlasList.end())
		{
			PT_CORE_ERROR("TextureAtlas not found in 'content/textureAtlas.json'");
			return nullptr;
		}

		const auto& size = textureAtlasList.at(filepath);
		//PT_CORE_INFO("file='{}' tile_size=({},{})", filepath, size.x, size.y);
		return MakeShared<TextureAtlas>("content/textures/" + filepath, size.x, size.y);
	}

	bool AssetManager::IsTextureLoaded(const std::string& filepath)
	{
		return s_Instance->m_Textures.find(filepath) != s_Instance->m_Textures.end();
	}

	bool AssetManager::IsTextureAtlasLoaded(const std::string& filepath)
	{
		return s_Instance->m_TextureAtlases.find(filepath) != s_Instance->m_TextureAtlases.end();
	}

	Shared<Texture> AssetManager::GetTexture(const std::string& filepath)
	{
		if (!IsTextureLoaded(filepath))
		{
			if (LoadTexture(filepath))
				return s_Instance->m_Textures[filepath];

			PT_CORE_ERROR("Texture not loaded '{}'", filepath);
			return nullptr;
		}

		return s_Instance->m_Textures.at(filepath);
	}

	Shared<TextureAtlas> AssetManager::GetTextureAtlas(const std::string& filepath)
	{
		if (!IsTextureAtlasLoaded(filepath))
		{
			auto textureAtlas = LoadTextureAtlas(filepath);
			if (!textureAtlas)
			{
				PT_CORE_ERROR("TextureAtlas not found '{}'", filepath);
				return nullptr;
			}
			s_Instance->m_TextureAtlases[filepath] = textureAtlas;
		}

		return s_Instance->m_TextureAtlases.at(filepath);
	}

	bool AssetManager::UnloadTexture(const std::string& filepath)
	{
		if (!IsTextureLoaded(filepath))
			return false;

		s_Instance->m_Textures.erase(filepath);
		return true;
	}

	bool AssetManager::UnloadTextureAtlas(const std::string& filepath)
	{
		if (!IsTextureAtlasLoaded(filepath))
			return false;

		s_Instance->m_TextureAtlases.erase(filepath);
		return true;
	}

	void AssetManager::ReloadAssetsList()
	{
		auto& textureList = s_Instance->m_TexturesFilepathList;
		auto& textureAtlasList = s_Instance->m_TextureAtlasList;

		textureList.clear();
		textureAtlasList.clear();

		static std::vector<std::string> extensions = { ".bmp", ".png", ".jpg", ".jpeg", ".tga", ".hdr", ".pic", ".psd" };

		for (const auto& entry : std::filesystem::recursive_directory_iterator("content/textures")) 
		{
			if (!entry.is_regular_file())
				continue;

			std::string filepath = entry.path().string();
			std::string extension = entry.path().extension().string();

			for (const std::string& ext : extensions)
			{
				if (extension != ext)
					continue;

				std::string textureFilename = filepath.substr(sizeof("content/textures"), filepath.length());
				textureList.push_back(textureFilename);
				break;
			}
		}

		for (auto& s : json::parse(Utils::ReadFile("content/meta.json")))
		{
			std::string filepath = s["file_path"];
			uint32_t width = s["tile_width"];
			uint32_t height = s["tile_height"];
			textureAtlasList[filepath] = glm::uvec2{ width, height };
			textureList.erase(
				std::remove(textureList.begin(), textureList.end(), filepath),
				textureList.end()
			);
		}
	}
}
