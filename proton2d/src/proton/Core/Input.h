#pragma once

#include "Proton/Core/KeyCodes.h"

namespace proton {

	class EntityScript;

	class Input 
	{
	public:
		static bool IsKeyPressed(int keyCode, EntityScript* script = nullptr);
		static bool IsMouseButtonPressed(MouseCode button, EntityScript* script = nullptr);
		static glm::vec2 GetMousePosition();
	};

}
