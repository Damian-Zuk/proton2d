#pragma once
#include "proton/Core/Layer.h"
#include "proton/Events/MouseEvents.h"

namespace proton {
	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void onAttach();
		void onDetach();
		void onUpdate(float timestep);
		void onEvent(Event& event);
	private:
		bool onMouseMoveEvent(MouseMovedEvent& e);
		bool onMouseButtonPressedEvent(MouseButtonPressedEvent& e);
		bool onMouseButtonReleasedEvent(MouseButtonReleasedEvent& e);
		bool onMouseScrolledEvent(MouseScrolledEvent& e);
	};
}


