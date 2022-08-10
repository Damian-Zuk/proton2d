#pragma once
#include "proton/Core/Input.h"

namespace proton {

	class WindowsInput : public Input 
	{
	protected:
		virtual bool isKeyPressed_Impl(int keyCode) override;
		virtual std::pair<float, float> getMousePosition_Impl() override;
	};

}