#pragma once
#include "proton/Core/Input.h"

namespace proton {

	class WindowsInput : public Input 
	{
	protected:
		virtual bool IsKeyPressed_Impl(int keyCode) override;
		virtual std::pair<float, float> GetMousePosition_Impl() override;
	};

}