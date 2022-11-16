#pragma once
#include <nlohmann/json.hpp>

namespace proton {

	using json = nlohmann::ordered_json;

	class Scene;
	class Entity;

	class SceneSerializer
	{
	public:
		static bool Serialize(const Shared<Scene>& scene);
		static void SerializeEntity(json& j, Entity entity, const Shared<Scene>& scene);
	};

}