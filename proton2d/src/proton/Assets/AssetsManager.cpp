#include "pch.h"
#include "proton/Assets/AssetsManager.h"

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
		}
	}

	bool AssetsManager::TextureExists(const std::string& keyPath)
	{
		return s_Instance->m_Textures.find(keyPath) != s_Instance->m_Textures.end();
	}

	bool AssetsManager::SpriteSheetExists(const std::string& keyPath)
	{
		return s_Instance->m_Spritesheets.find(keyPath) != s_Instance->m_Spritesheets.end();
	}

	Shared<Texture> AssetsManager::GetTexture(const std::string& keyPath)
	{
		if (!TextureExists(keyPath))
		{
			if (LoadTexture(keyPath))
				return s_Instance->m_Textures[keyPath];

			LOG_ERROR("[AssetsManager] Texture not found:", keyPath);
			return nullptr;
		}

		return s_Instance->m_Textures[keyPath];
	}

	bool AssetsManager::UnloadTexture(const std::string& filepath)
	{
		if (!TextureExists(filepath))
			return false;

		s_Instance->m_Textures.erase(filepath);
		return true;
	}

	Shared<SpriteSheet> AssetsManager::GetSpriteSheet(const std::string& keyPath)
	{
		if (!SpriteSheetExists(keyPath)) 
		{
			LOG_ERROR("[AssetsManager] Spritesheet not found:", keyPath);
			return nullptr;
		}

		return s_Instance->m_Spritesheets[keyPath];
	}

	bool AssetsManager::UnloadSpritesheet(const std::string& filepath)
	{
		if (!SpriteSheetExists(filepath))
			return false;

		s_Instance->m_Spritesheets.erase(filepath);
		return true;
	}
}
