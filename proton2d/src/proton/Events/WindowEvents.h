#pragma once

#include "Event.h"

namespace proton {

	class WindowResizedEvent : public Event
	{
	public:
		WindowResizedEvent(unsigned int width, unsigned int height)
			: m_Width(width), m_Height(height) {}

		unsigned int getWidth() const { return m_Width; }
		unsigned int getHeight() const { return m_Height; }

		std::string toString() const override
		{
			std::stringstream ss;
			ss << "WindowResizedEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResized)
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