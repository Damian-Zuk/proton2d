#pragma once

#include "Proton/Core/KeyCodes.h"

namespace proton {

	class EntityScript;

	class Input 
	{
	public:
		// TODO: Find workaround to not pass pointer to EntityScript
		// It is used for multiple game instances in ImGui editor
		// to check if ImGui viewport window is focused.
		// Possible solution: generate C++ output script.
		static bool IsKeyPressed(int keyCode, EntityScript* script = nullptr);
		static bool IsMouseButtonPressed(MouseCode button, EntityScript* script = nullptr);
		static glm::vec2 GetMousePosition();
	};

}
