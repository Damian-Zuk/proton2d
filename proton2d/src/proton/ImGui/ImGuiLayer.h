#pragma once
#include "proton/Core/Layer.h"
#include "proton/Events/MouseEvents.h"

namespace proton {
	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void onAttach() override;
		void onDetach() override;
		void onImGuiRender() override;

		void Begin();
		void End();
	};
}


