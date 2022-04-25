#pragma once
#include "proton/Core/Layer.h"

namespace proton {
	class PROTON_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void onAttach();
		void onDetach();
		void onUpdate(float timestep);
		void onEvent(Event& event);
	};
}


