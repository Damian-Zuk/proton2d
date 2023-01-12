#pragma once
#include "pch.h"

namespace proton {

	enum class EventType {
		WindowResized, WindowClosed,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	class Event 
	{
	public:
		virtual EventType GetEventType()	const = 0;
		virtual std::string GetEventName()	const = 0;
		virtual std::string ToString()		const { return GetEventName(); }

	public:
		bool m_Handled = false;
	};

#define EVENT_CLASS_TYPE(type) \
	static EventType GetStaticType() { return EventType::type; } \
	EventType GetEventType()	const override	{ return GetStaticType(); } \
	std::string GetEventName()	const override	{ return #type; } 
								
	class EventDispatcher
	{
	public:
		EventDispatcher(Event& event) : m_Event(event) {}

		template<typename T, typename F>
		void Dispatch(const F& function) {
			if (m_Event.GetEventType() == T::GetStaticType())
				m_Event.m_Handled |= function(static_cast<T&>(m_Event));
		}

	private:
		Event& m_Event;
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}
}