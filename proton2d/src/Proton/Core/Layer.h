#pragma once

#include "Proton/Events/Event.h"

namespace proton {

	// Forward declaration
	class SceneManager;
	class GameInstance;

	class Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate(float timestep) {}
		virtual void OnEvent(Event& event) {}
		virtual void OnImGuiRender() {}

		const std::string& GetName() const { return m_DebugName; }
		friend class Application;
	protected:
		std::string m_DebugName;
	};

}
