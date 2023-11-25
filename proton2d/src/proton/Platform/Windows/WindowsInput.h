#pragma once
#include "proton/Core/Input.h"

namespace proton {

	class WindowsInput : public Input 
	{
	protected:
		virtual bool Impl_IsKeyPressed(int keyCode) override;
		virtual std::pair<float, float> Impl_GetMousePosition() override;
	};

}
