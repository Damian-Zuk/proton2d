#pragma once

#include "proton/Events/Event.h"

namespace proton {

	class WindowResizedEvent : public Event
	{
	public:
		EVENT_CLASS_TYPE(WindowResized)

		WindowResizedEvent(unsigned int width, unsigned int height)
			: m_Width(width), m_Height(height) {}

		unsigned int GetWidth() const { return m_Width; }
		unsigned int GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizedEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

	private:
		unsigned int m_Width, m_Height;
	};

	class WindowClosedEvent : public Event 
	{
	public:
		EVENT_CLASS_TYPE(WindowClosed)
		WindowClosedEvent() = default;
	};

}
