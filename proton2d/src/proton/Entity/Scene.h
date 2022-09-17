#pragma once
#include "proton/Entity/Components.h"
#include "proton/Renderer/Camera.h"

#include <entt/entt.hpp>

namespace proton {

	class Entity;

	class Scene
	{
	public:
		friend class Entity;

		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name);

		void OnUpdate(float ts);

	private:
		entt::registry m_Registry;
		Camera m_Camera;
	};

}