#include "pch.h"
#include "proton/Assets/SceneSerializer.h"
#include "proton/Assets/AssetsManager.h"
#include "proton/Assets/ScriptFactory.h"
#include "proton/Entity/Scene.h"
#include "proton/Entity/Entity.h"
#include "proton/Core/Utils.h"

#include <fstream>

#define PROTON_SERIALIZER_INDENT_JSON 0

namespace proton {

	Scene* SceneSerializer::m_Scene = nullptr;

	static inline double round(float f)
	{
		return std::round((double)f * 100000) / 100000;
	}

	void SceneSerializer::SetContext(Scene* scene)
	{
		m_Scene = scene;
	}

	json SceneSerializer::SerializeEntity(Entity entity)
	{
		json jsonObj;

		// Serialize TagComponent
		auto& tag = entity.GetComponent<TagComponent>().Tag;
		jsonObj["Tag"] = tag;

		// Serialize TransformComponent
		auto& transform = entity.GetComponent<TransformComponent>();
		auto& position = transform.Position;
		jsonObj["Transform"]["Position"] = { round(position.x), round(position.y), round(position.z) };
		jsonObj["Transform"]["Rotation"] = round(transform.Rotation);
		jsonObj["Transform"]["Scale"] = { round(transform.Scale.x), round(transform.Scale.y) };

		// Serialize SpriteComponent
		if (entity.HasComponent<SpriteComponent>())
		{
			auto& spriteComponent = entity.GetComponent<SpriteComponent>();
			auto& sprite = spriteComponent.Sprite;
			auto& color = spriteComponent.Color;

			if (sprite)
			{
				jsonObj["Sprite"]["Texture"] = sprite->GetTexture()->GetPath();
				jsonObj["Sprite"]["Flip"] = { sprite->m_FlipX, sprite->m_FlipY };
			}
			if (sprite->m_SpriteSheet)
			{
				jsonObj["Sprite"]["TilePos"] = { sprite->m_PosX, sprite->m_PosY };
				jsonObj["Sprite"]["TileSize"] = { sprite->m_SizeX, sprite->m_SizeY };
			}
			
			jsonObj["Sprite"]["Color"] = { round(color.r), round(color.g), round(color.b), round(color.a) };
		}
		
		// Serialize TilemapSpriteComponent
		if (entity.HasComponent<TilemapSpriteComponent>())
		{
			auto& tilemap = entity.GetComponent<TilemapSpriteComponent>();
			auto& spritesheet = tilemap.Spritesheet;
			
			if (spritesheet)
				jsonObj["TilemapSprite"]["Spritesheet"] = spritesheet->GetTexture()->GetPath();
			else
				jsonObj["TilemapSprite"]["Spritesheet"] = 0;

			jsonObj["TilemapSprite"]["Width"] = tilemap.Width;
			jsonObj["TilemapSprite"]["Height"] = tilemap.Height;
			
			for (const auto& column : tilemap.Tilemap)
			{
				for (const auto& tile : column)
					jsonObj["TilemapSprite"]["Tilemap"].push_back({ tile.x, tile.y });
			}
		}

		// Serialize ScriptComponent
		if (entity.HasComponent<ScriptComponent>())
		{
			auto& script = entity.GetComponent<ScriptComponent>();
			for (auto& kv : script.Scripts)
				jsonObj["Scripts"].push_back(kv.first);
		}

		// Serialize child entities
		auto& relationship = entity.GetComponent<RelationshipComponent>();
		if (relationship.ChildrenCount)
		{
			entt::entity current = relationship.First;
			while (current != entt::null)
			{
				Entity child{ entity.m_Scene, current };
				auto& rel = child.GetComponent<RelationshipComponent>();
				jsonObj["Entities"].push_back(SerializeEntity(child));
				current = rel.Next;
			}
		}

		return jsonObj;
	}

	bool SceneSerializer::Serialize(const std::string& filepath)
	{
		assert(m_Scene && "Scene context not set!");
		json jsonObj;
		jsonObj["Scene name"] = m_Scene->m_SceneName;

		m_Scene->m_Registry.each([&](auto id)
		{
			Entity entity{ m_Scene, id };
			auto& relationship = entity.GetComponent<RelationshipComponent>();
			if (relationship.Parent == entt::null)
				jsonObj["Entities"].push_back(SerializeEntity(entity));
		});

		std::ofstream out(filepath);
		#if PROTON_SERIALIZER_INDENT_JSON
			out << jsonObj.dump(4);
		#else
			out << jsonObj;
		#endif
		out.close();
		return true;
	}
	
	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		std::string jsonData = ReadFileBinary(filepath);
		if (jsonData.size())
		{
			json jsonObj = json::parse(jsonData);
			m_Scene->m_SceneName = jsonObj["Scene name"];
		
			json& entities = jsonObj["Entities"];
			for (auto it = entities.rbegin(); it != entities.rend(); it++)
				DeserializeEntity(*it);

			return true;
		}
		return false;
	}

	Entity SceneSerializer::DeserializeEntity(json jsonObj)
	{
		Entity entity = m_Scene->CreateEntity(jsonObj["Tag"]);

		// Deserialize TransformComponent
		auto& transform = entity.GetComponent<TransformComponent>();
		json& position = jsonObj["Transform"]["Position"];
		json& scale    = jsonObj["Transform"]["Scale"];
		json& rotation = jsonObj["Transform"]["Rotation"];
		transform.Position = { position[0], position[1], position[2] };
		transform.Scale    = { scale[0], scale[1] };
		transform.Rotation = rotation;

		// Deserialize SpriteComponent
		if (jsonObj.contains("Sprite"))
		{
			json& sprite = jsonObj["Sprite"];
			auto& spriteComponent = entity.AddComponent<SpriteComponent>();
			
			if (sprite.contains("Texture"))
			{
				if (sprite.contains("TilePos"))
				{
					spriteComponent.Sprite = CreateShared<Sprite>(
						AssetsManager::GetSpriteSheet(sprite["Texture"]),
						sprite["TilePos"][0], sprite["TilePos"][1],
						sprite["TileSize"][0], sprite["TileSize"][1]);
				} 
				else
				{
					spriteComponent.Sprite 
						= CreateShared<Sprite>(AssetsManager::GetTexture(sprite["Texture"]));
				}

				spriteComponent.Sprite->m_FlipX = sprite["Flip"][0];
				spriteComponent.Sprite->m_FlipX = sprite["Flip"][1];
			}
			
			json& color = jsonObj["Sprite"]["Color"];
			spriteComponent.Color = { color[0], color[1], color[2], color[3] };
		}

		// Deserialize TilemapSpriteComponent
		if (jsonObj.contains("TilemapSprite"))
		{
			uint32_t width = jsonObj["TilemapSprite"]["Width"];
			uint32_t height = jsonObj["TilemapSprite"]["Height"];

			auto& tilemap = entity.AddComponent<TilemapSpriteComponent>(
					AssetsManager::GetSpriteSheet(jsonObj["TilemapSprite"]["Spritesheet"]),
					width, height);

			int i = 0;
			for (auto& jsonCoords : jsonObj["TilemapSprite"]["Tilemap"])
			{
				tilemap.Tilemap[i / height][i % height] 
					= glm::ivec2{ jsonCoords[0], jsonCoords[1] };
				i++;
			}
		}

		// Deserialize scripts
		if (jsonObj.contains("Scripts"))
		{
			for (auto& scriptClass : jsonObj["Scripts"])
			{
				auto& registeredScripts = ScriptFactory::GetScripts();
				
				if (registeredScripts.find(scriptClass) == registeredScripts.end())
					LOG_ERROR("[SceneSerializer] Script not found:", scriptClass);
				
				registeredScripts.at(scriptClass)(entity); // call add script to entity function
			}
		}

		// Deserialize child entities
		if (jsonObj.contains("Entities"))
		{
			json& entities = jsonObj["Entities"];
			for (auto it = entities.rbegin(); it != entities.rend(); it++)
				entity.AddChildEntity(DeserializeEntity(*it));
		}

		return entity;
	}

}
