#pragma once

#include "Event.h"

namespace proton {

	class WindowClosedEvent : public Event 
	{
	public:
		EVENT_CLASS_TYPE(WindowClosed)
		WindowClosedEvent() = default;
	};

}