#pragma once
#include "Core.h"

namespace proton {

	class PROTON_API Application
	{
	public:
		Application();
		~Application();
		void Start();
	};

	Application* CreateApplication();
}
