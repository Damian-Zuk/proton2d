#include "pch.h"
#include "proton/Assets/AssetsManager.h"

namespace proton {

	AssetsManager* AssetsManager::s_Instance = nullptr;

	AssetsManager::AssetsManager()
	{
		s_Instance = this;
	}

	AssetsManager& AssetsManager::Get()
	{
		return *s_Instance;
	}

	Shared<Texture> AssetsManager::LoadTexture(const std::string& filepath)
	{
		auto texture = CreateShared<Texture>(filepath);
		if (texture->IsLoaded()) 
		{
			LOG_INFO("[AssetsManager] Loaded texture:", filepath);
			m_Textures[filepath] = texture;
			return texture;
		}
		LOG_ERROR("[AssetsManager] Couldn't load texture:", filepath);

		return nullptr;
	}

	Shared<SpriteSheet> AssetsManager::LoadSpriteSheet(const std::string& filepath, uint32_t tileWidth, uint32_t tileHeight)
	{
		auto texture = GetTexture(filepath);
		if (texture) 
		{
			auto spriteSheet = CreateShared<SpriteSheet>(texture, tileWidth, tileHeight);
			m_SpriteSheets[filepath] = spriteSheet;

			return spriteSheet;
		}
		
		return nullptr;
	}

	bool AssetsManager::TextureExists(const std::string& keyPath)
	{
		return m_Textures.find(keyPath) != m_Textures.end();
	}

	bool AssetsManager::SpriteSheetExists(const std::string& keyPath)
	{
		return m_SpriteSheets.find(keyPath) != m_SpriteSheets.end();
	}

	Shared<Texture> AssetsManager::GetTexture(const std::string& keyPath)
	{
		if (!TextureExists(keyPath))
		{
			if (LoadTexture(keyPath))
				return m_Textures[keyPath];

			return nullptr;
		}

		return m_Textures[keyPath];
	}
	Shared<SpriteSheet> AssetsManager::GetSpriteSheet(const std::string& keyPath)
	{
		if (!SpriteSheetExists(keyPath))
			return nullptr;

		return m_SpriteSheets[keyPath];
	}
}
