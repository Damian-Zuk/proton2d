#include "pch.h"
#include "proton/Assets/AssetsManager.h"
#include "proton/Core/Utils.h"

namespace proton {

	AssetsManager* AssetsManager::s_Instance = nullptr;

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

	Shared<SpriteSheet> AssetsManager::LoadSpriteSheet(const std::string& filepath, uint32_t tileWidth, uint32_t tileHeight)
	{
		auto texture = GetTexture(filepath);
		if (!texture) 
		{
			LOG_ERROR("[AssetsManager] Couldn't load spritesheet:", filepath);
			return nullptr;
		}

		auto spriteSheet = CreateShared<SpriteSheet>(texture, tileWidth, tileHeight);
		s_Instance->m_Spritesheets[filepath] = spriteSheet;
		return spriteSheet;
	}

	void AssetsManager::Init()
	{
		if (!s_Instance)
		{
			s_Instance = new AssetsManager();
			ReloadAssetsList();
		}
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

		return s_Instance->m_Textures[filepath];
	}

	bool AssetsManager::UnloadTexture(const std::string& filepath)
	{
		if (!TextureExists(filepath))
			return false;

		s_Instance->m_Textures.erase(filepath);
		return true;
	}

	Shared<SpriteSheet> AssetsManager::GetSpriteSheet(const std::string& filepath)
	{
		if (!SpriteSheetExists(filepath))
		{
			LOG_ERROR("[AssetsManager] Spritesheet not found:", filepath);
			return nullptr;
		}

		return s_Instance->m_Spritesheets[filepath];
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
		s_Instance->m_TexturesFilepathList.clear();
		s_Instance->m_TexturesFilepathList =
			Utils::GetFilesFromDirectoryRecursive("assets",
				{ ".bmp", ".png", ".jpg", ".jpeg", ".tga", ".hdr", ".pic", ".psd", ".gif" });
	}
}
