#pragma once

namespace proton {

	class Input 
	{
	public:
		static bool isKeyPressed(int keyCode)				{ return s_Instance->isKeyPressed_Impl(keyCode);	}
		static std::pair<float, float> getMousePosition()	{ return s_Instance->getMousePosition_Impl();		}
	protected:
		virtual bool isKeyPressed_Impl(int keyCode) = 0;
		virtual std::pair<float,float> getMousePosition_Impl() = 0;
	private:
		static Input* s_Instance;
	};

}