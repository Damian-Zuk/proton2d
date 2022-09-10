#pragma once
#include "proton/Core/KeyCodes.h"

namespace proton {

	class Input 
	{
	public:
		static bool IsKeyPressed(int keyCode)				{ return s_Instance->IsKeyPressed_Impl(keyCode);	}
		static std::pair<float, float> GetMousePosition()	{ return s_Instance->GetMousePosition_Impl();		}
	protected:
		virtual bool IsKeyPressed_Impl(int keyCode) = 0;
		virtual std::pair<float,float> GetMousePosition_Impl() = 0;
	private:
		static Input* s_Instance;
	};

}