#pragma once
#include "Core/Core.h"

namespace proton {

	class PROTON_API Application
	{
	public:
		Application();
		~Application();
		void Start();
	protected:
		virtual bool OnStart() = 0;
	};

	Application* CreateApplication();
}
