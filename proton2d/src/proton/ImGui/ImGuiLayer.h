#pragma once
#include "proton/Core/Layer.h"
#include "proton/Events/MouseEvents.h"

namespace proton {
	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnAttach() override;
		void OnDetach() override;
		void OnImGuiRender() override;

		void Begin();
		void End();
	};
}


