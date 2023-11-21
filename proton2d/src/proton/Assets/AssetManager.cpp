#include "pch.h"
#include "proton/Assets/AssetManager.h"
#include "proton/Utils/Utils.h"

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
		auto texture = CreateShared<Texture>(filepath);
		if (!texture->IsLoaded()) 
		{
			LOG_ERROR("[AssetManager] Couldn't load texture:", filepath);
			return nullptr;
		}

		LOG_INFO("[AssetManager] Loaded texture:", filepath);
		s_Instance->m_Textures[filepath] = texture;
		return texture;
	}

	Shared<Spritesheet> AssetManager::LoadSpritesheet(const std::string& filepath)
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
		LOG_INFO("[AssetManager] Loaded spritesheet", filepath, "size:", size.x, "x", size.y);
		return CreateShared<Spritesheet>(texture, size.x, size.y);
	}

	bool AssetManager::IsTextureLoaded(const std::string& filepath)
	{
		return s_Instance->m_Textures.find(filepath) != s_Instance->m_Textures.end();
	}

	bool AssetManager::IsSpritesheetLoaded(const std::string& filepath)
	{
		return s_Instance->m_Spritesheets.find(filepath) != s_Instance->m_Spritesheets.end();
	}

	Shared<Texture> AssetManager::GetTexture(const std::string& filepath)
	{
		if (!IsTextureLoaded(filepath))
		{
			if (LoadTexture(filepath))
				return s_Instance->m_Textures[filepath];

			LOG_ERROR("[AssetManager] Texture not found:", filepath);
			return nullptr;
		}

		return s_Instance->m_Textures.at(filepath);
	}

	Shared<Spritesheet> AssetManager::GetSpritesheet(const std::string& filepath)
	{
		if (!IsSpritesheetLoaded(filepath))
		{
			auto spritesheet = LoadSpritesheet(filepath);
			if (!spritesheet)
			{
				LOG_ERROR("[AssetManager] Spritesheet not found:", filepath);
				return nullptr;
			}
			s_Instance->m_Spritesheets[filepath] = spritesheet;
		}

		return s_Instance->m_Spritesheets.at(filepath);
	}

	bool AssetManager::UnloadTexture(const std::string& filepath)
	{
		if (!IsTextureLoaded(filepath))
			return false;

		s_Instance->m_Textures.erase(filepath);
		return true;
	}

	bool AssetManager::UnloadSpritesheet(const std::string& filepath)
	{
		if (!IsSpritesheetLoaded(filepath))
			return false;

		s_Instance->m_Spritesheets.erase(filepath);
		return true;
	}

	void AssetManager::ReloadAssetsList()
	{
		auto& textureList = s_Instance->m_TexturesFilepathList;
		auto& spritesheetList = s_Instance->m_SpritesheetList;

		textureList.clear();
		spritesheetList.clear();

		textureList = Utils::ScanDirectoryRecursive("assets",
			{ ".bmp", ".png", ".jpg", ".jpeg", ".tga", ".hdr", ".pic", ".psd" });

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
