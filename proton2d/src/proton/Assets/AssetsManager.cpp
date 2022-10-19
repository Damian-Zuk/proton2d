#include "pch.h"
#include "proton/Assets/AssetsManager.h"

namespace proton {

	static struct AssetsManagerData
	{
		std::unordered_map<std::string, Shared<Texture>> Textures;
		std::unordered_map<std::string, Shared<SpriteSheet>> SpriteSheets;
	} data;

	Shared<Texture> AssetsManager::LoadTexture(const std::string& filepath)
	{
		auto texture = CreateShared<Texture>(filepath);
		if (texture->IsLoaded()) 
		{
			LOG_INFO("[AssetsManager] Loaded texture:", filepath);
			data.Textures[filepath] = texture;
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
			data.SpriteSheets[filepath] = spriteSheet;

			return spriteSheet;
		}
		
		return nullptr;
	}

	bool AssetsManager::TextureExists(const std::string& keyPath)
	{
		return data.Textures.find(keyPath) != data.Textures.end();
	}

	bool AssetsManager::SpriteSheetExists(const std::string& keyPath)
	{
		return data.SpriteSheets.find(keyPath) != data.SpriteSheets.end();
	}

	Shared<Texture> AssetsManager::GetTexture(const std::string& keyPath)
	{
		if (!TextureExists(keyPath))
		{
			if (LoadTexture(keyPath))
				return data.Textures[keyPath];

			return nullptr;
		}

		return data.Textures[keyPath];
	}
	Shared<SpriteSheet> AssetsManager::GetSpriteSheet(const std::string& keyPath)
	{
		if (!SpriteSheetExists(keyPath))
			return nullptr;

		return data.SpriteSheets[keyPath];
	}
}
