#pragma once
#include "pch.h"

namespace proton {

	enum class EventType {
		WindowResized, WindowClosed, WindowFocused, WindowLostFocus,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};


	class Event 
	{
	public:
		virtual EventType getEventType()	const = 0;
		virtual std::string getEventName()	const = 0;
		virtual std::string toString()		const { return getEventName(); }

	public:
		bool m_Handled = false;
	};


#define EVENT_CLASS_TYPE(type)	static EventType getStaticType()			{ return EventType::type; } \
								EventType getEventType()	const override	{ return getStaticType(); } \
								std::string getEventName()	const override	{ return #type; }
								


	class EventDispatcher
	{

	public:
		EventDispatcher(Event& event) : m_Event(event) {}

		template<typename T, typename F>
		void dispatch(const F& function) {
			if (m_Event.getEventType() == T::getStaticType())
				m_Event.m_Handled |= function(static_cast<T&>(m_Event));
		}

	private:
		Event& m_Event;
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.toString();
	}
}