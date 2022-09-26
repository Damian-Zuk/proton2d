#pragma once

#include "proton/Core/KeyCodes.h"

namespace proton {


	class Input 
	{
	public:
		static bool IsKeyPressed(int keyCode)             { return s_Instance->IsKeyPressed_Implementation(keyCode); }
		static std::pair<float, float> GetMousePosition() { return s_Instance->GetMousePosition_Implementation();    }

	protected:
		// Implementation per platform
		virtual bool IsKeyPressed_Implementation(int keyCode) = 0;
		virtual std::pair<float,float> GetMousePosition_Implementation() = 0;

	private:
		static Input* s_Instance;

		friend class Application;
	};

}