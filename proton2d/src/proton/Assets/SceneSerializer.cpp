#include "pch.h"
#include "proton/Assets/SceneSerializer.h"
#include "proton/Entity/Scene.h"
#include "proton/Entity/Entity.h"

#include <fstream>

namespace proton {

	 void SceneSerializer::SerializeEntity(json& j, Entity entity, const Shared<Scene>& scene)
	 {
		auto json_obj = json::object();
		// Serialize TagComponent
		auto& tag = entity.GetComponent<TagComponent>().Tag;
		json_obj["Tag"] = tag;
		
		// Serialize TransformComponent
		auto& transform = entity.GetComponent<TransformComponent>();
		auto& position = transform.Position;
		json_obj["Transform"]["Position"] = json::array({ position.x, position.y, position.z });
		json_obj["Transform"]["Rotation"] = transform.Rotation;
		json_obj["Transform"]["Scale"]    = json::array({ transform.Scale.x, transform.Scale.y });
		
		// Serialize SpriteComponent
		if (entity.HasComponent<SpriteComponent>())
		{
			auto& spriteComponent = entity.GetComponent<SpriteComponent>();
			auto& sprite = spriteComponent.Sprite;
			auto& color = spriteComponent.Color;
			json_obj["Sprite"]["Texture"] = sprite->GetTexture()->GetPath();
			json_obj["Sprite"]["Size"]    = json::array({ sprite->m_SizeX, sprite->m_SizeY });
			json_obj["Sprite"]["Pos"]     = json::array({ sprite->m_PosX, sprite->m_PosY });
			json_obj["Sprite"]["Flip"]    = json::array({ sprite->m_FlipX, sprite->m_FlipY });
			json_obj["Sprite"]["Color"]   = json::array({ color.r, color.g, color.b, color.a });
		}

		// Serialize ScriptComponent
		if (entity.HasComponent<ScriptComponent>())
		{
			auto& script = entity.GetComponent<ScriptComponent>();
			for (auto& kv : script.Scripts)
				json_obj["Scripts"].push_back(kv.first);
		}
		
		// Serialize children
		auto& relationship = entity.GetComponent<RelationshipComponent>();
		if (relationship.ChildrenCount)
		{
			entt::entity current = relationship.First;
			while (current != entt::null)
			{
				Entity child{ scene.get(), current };
				auto& rel = child.GetComponent<RelationshipComponent>();
				SerializeEntity(json_obj["Children"], child, scene);
				current = rel.Next;
			}
		}
		j.push_back(json_obj);
	}

	bool SceneSerializer::Serialize(const Shared<Scene>& scene)
	{
		std::ofstream out("out.json");
		json j;
		scene->m_Registry.each([&](auto id)
		{
			Entity entity{ scene.get(), id};
			auto& relationship = entity.GetComponent<RelationshipComponent>();
			if (relationship.Parent == entt::null)
			{
				SerializeEntity(j, entity, scene);
			}
		});
		out << j.dump(4);
		out.close();

		return true;
	}
}
