#pragma once
#include "proton/Events/Event.h"
#include <string>

namespace proton {

	class Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void onAttach() {}
		virtual void onDetach() {}
		virtual void onUpdate(float timestep) {}
		virtual void onEvent(Event& event) {}

		const std::string& getName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};

}