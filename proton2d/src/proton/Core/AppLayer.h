#pragma once

#include "Proton/Events/Event.h"

namespace proton {

	// Forward declaration
	class SceneManager;
	class GameInstance;

	class AppLayer
	{
	public:
		virtual ~AppLayer() = default;

		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate(float timestep) {}
		virtual void OnEvent(Event& event) {}
		virtual void OnImGuiRender() {}

		friend class Application;
	};

}
