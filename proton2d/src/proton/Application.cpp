#include "pch.h"
#include "Application.h"
#include "Logger.h"
#include <iostream>

namespace proton {

	Application::Application()
	{
	}

	Application::~Application()
	{
	}

	void Application::Start()
	{
		Logger::init();
		LOG("Application started");
		if (OnStart()) {
			while (true) {

			}
		}
	}

}