#pragma once
#include <nlohmann/json.hpp>

namespace proton {

	using json = nlohmann::ordered_json;

	class Scene;
	class Entity;

	class SceneSerializer
	{
	public:
		static void SetContext(Scene* scene);

		static bool Serialize(const std::string& filepath);
		static json SerializeEntity(Entity entity);

		static bool Deserialize(const std::string& filepath);
		static Entity DeserializeEntity(json jsonObj);
	
	private:
		static Scene* m_Scene;
	};

}