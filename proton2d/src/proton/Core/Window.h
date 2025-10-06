#pragma once

#include "Proton/Core/Base.h"
#include "Proton/Events/Event.h"

namespace proton {
	
	struct WindowSpecification
	{
		std::string Title;
		uint32_t Width = 1280;
		uint32_t Height = 720;
		bool Fullscreen = false;
		bool IsResizeable = true;
		bool VSync = true;
	};

	class Window
	{
	public:
		Window(const WindowSpecification& spec) { m_Data.Spec = spec; }
		virtual ~Window() = default;

		virtual void OnUpdate() = 0;
		virtual void* GetNativeWindow() const = 0;

		uint32_t GetWidth() const { return m_Data.Spec.Width; }
		uint32_t GetHeight() const { return m_Data.Spec.Height; }
		inline float GetAspectRatio() const { return (float)GetWidth() / (float)GetHeight(); }

		virtual void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }
		virtual void SetVSync(bool enabled) { m_Data.Spec.VSync = enabled; }
		virtual bool IsVSync() const { return m_Data.Spec.VSync; }

		virtual void SetFullscreen(bool fullscreen = true) { m_Data.Spec.Fullscreen = fullscreen; }
		virtual bool IsFullscreen() const { return m_Data.Spec.Fullscreen; }

		WindowSpecification& GetWindowSpecification() { return m_Data.Spec; }
		const WindowSpecification& GetWindowSpecification() const { return m_Data.Spec; }
	
	protected:
		struct WindowData
		{
			WindowSpecification Spec;
			EventCallbackFn EventCallback;
		} m_Data;
	};

}
