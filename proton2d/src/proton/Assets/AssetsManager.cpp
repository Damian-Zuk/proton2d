#include "pch.h"
#include "proton/Assets/AssetsManager.h"
#include "proton/Core/Utils.h"

namespace proton {

	AssetsManager* AssetsManager::s_Instance = nullptr;

	void AssetsManager::Init()
	{
		if (!s_Instance)
		{
			s_Instance = new AssetsManager();
			ReloadAssetsList();
		}
	}

	Shared<Texture> AssetsManager::LoadTexture(const std::string& filepath)
	{
		auto texture = CreateShared<Texture>(filepath);
		if (!texture->IsLoaded()) 
		{
			LOG_ERROR("[AssetsManager] Couldn't load texture:", filepath);
			return nullptr;
		}

		LOG_INFO("[AssetsManager] Loaded texture:", filepath);
		s_Instance->m_Textures[filepath] = texture;
		return texture;
	}

	Shared<SpriteSheet> AssetsManager::LoadSpriteSheet(const std::string& filepath)
	{
		auto texture = GetTexture(filepath);
		if (!texture)
			return nullptr;
		
		auto& spritesheetList = s_Instance->m_SpritesheetList;
		if (spritesheetList.find(filepath) == spritesheetList.end())
		{
			LOG_ERROR("Spritesheet not found in assets/spritesheets.json");
			return nullptr;
		}

		const auto& size = spritesheetList.at(filepath);
		LOG_INFO("[AssetsManager] Loaded spritesheet", filepath, "size:", size.x, "x", size.y);
		return CreateShared<SpriteSheet>(texture, size.x, size.y);
	}

	bool AssetsManager::TextureExists(const std::string& filepath)
	{
		return s_Instance->m_Textures.find(filepath) != s_Instance->m_Textures.end();
	}

	bool AssetsManager::SpriteSheetExists(const std::string& filepath)
	{
		return s_Instance->m_Spritesheets.find(filepath) != s_Instance->m_Spritesheets.end();
	}

	Shared<Texture> AssetsManager::GetTexture(const std::string& filepath)
	{
		if (!TextureExists(filepath))
		{
			if (LoadTexture(filepath))
				return s_Instance->m_Textures[filepath];

			LOG_ERROR("[AssetsManager] Texture not found:", filepath);
			return nullptr;
		}

		return s_Instance->m_Textures.at(filepath);
	}

	Shared<SpriteSheet> AssetsManager::GetSpriteSheet(const std::string& filepath)
	{
		if (!SpriteSheetExists(filepath))
		{
			auto spritesheet = LoadSpriteSheet(filepath);
			if (!spritesheet)
			{
				LOG_ERROR("[AssetsManager] Spritesheet not found:", filepath);
				return nullptr;
			}
			s_Instance->m_Spritesheets[filepath] = spritesheet;
		}

		return s_Instance->m_Spritesheets.at(filepath);
	}

	bool AssetsManager::UnloadTexture(const std::string& filepath)
	{
		if (!TextureExists(filepath))
			return false;

		s_Instance->m_Textures.erase(filepath);
		return true;
	}

	bool AssetsManager::UnloadSpritesheet(const std::string& filepath)
	{
		if (!SpriteSheetExists(filepath))
			return false;

		s_Instance->m_Spritesheets.erase(filepath);
		return true;
	}

	void AssetsManager::ReloadAssetsList()
	{
		auto& textureList = s_Instance->m_TexturesFilepathList;
		auto& spritesheetList = s_Instance->m_SpritesheetList;

		textureList.clear();
		spritesheetList.clear();

		textureList = Utils::GetFilesFromDirectoryRecursive("assets",
			{ ".bmp", ".png", ".jpg", ".jpeg", ".tga", ".hdr", ".pic", ".psd", ".gif" });

		for (auto& s : json::parse(Utils::ReadFile("assets/spritesheets.json")))
		{
			std::string filepath = s["Filepath"];
			uint32_t width = s["TileWidth"], height = s["TileHeight"];
			spritesheetList[filepath] = glm::uvec2{ width, height };
			textureList.erase(
				std::remove(textureList.begin(), textureList.end(), filepath),
				textureList.end()
			);
		}
	}
}
