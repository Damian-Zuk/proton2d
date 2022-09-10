#include "pch.h"
#include "proton/Renderer/CameraController.h"
#include "proton/Core/Input.h"
#include "proton/Events/MouseEvents.h"

namespace proton {

	CameraController::CameraController(float aspectRatio)
		: m_Camera(aspectRatio), m_AspectRatio(aspectRatio)
	{
	}

	CameraController::~CameraController()
	{
	}

	void CameraController::Update(float ts)
	{
		if (Input::IsKeyPressed(Key::W)) {
			m_Camera.Move({ 0.0f, m_CameraSpeed.y * m_AspectRatio * ts });
		}

		if (Input::IsKeyPressed(Key::A)) {
			m_Camera.Move({ -m_CameraSpeed.x * ts, 0.0f });
		}

		if (Input::IsKeyPressed(Key::S)) {
			m_Camera.Move({ 0.0f, -m_CameraSpeed.y * m_AspectRatio * ts });
		}

		if (Input::IsKeyPressed(Key::D)) {
			m_Camera.Move({ m_CameraSpeed.x * ts, 0.0f });
		}
	}

	void CameraController::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<MouseScrolledEvent>([&](MouseScrolledEvent& event) -> bool {
			m_Camera.Zoom(m_CameraZoomSpeed * -event.GetYOffset());
			return false;
		});
	}

}
