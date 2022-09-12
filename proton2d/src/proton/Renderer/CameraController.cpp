#include "pch.h"
#include "proton/Renderer/CameraController.h"
#include "proton/Core/Input.h"
#include "proton/Events/MouseEvents.h"
#include "proton/Core/Application.h"
#include "proton/Events/WindowEvents.h"

namespace proton {

	CameraController::CameraController()
		: m_AspectRatio(Application::Get().GetWindow().GetAspectRatio()), m_Camera(m_AspectRatio)
	{
	}

	CameraController::~CameraController()
	{
	}

	void CameraController::OnUpdate(float ts)
	{
		if (Input::IsKeyPressed(Key::W)) 
			m_Camera.Move({ 0.0f, m_CameraSpeed.y * m_AspectRatio * ts });

		if (Input::IsKeyPressed(Key::A)) 
			m_Camera.Move({ -m_CameraSpeed.x * ts, 0.0f });

		if (Input::IsKeyPressed(Key::S)) 
			m_Camera.Move({ 0.0f, -m_CameraSpeed.y * m_AspectRatio * ts });

		if (Input::IsKeyPressed(Key::D)) 
			m_Camera.Move({ m_CameraSpeed.x * ts, 0.0f });

		float zoomLevel = m_Camera.GetZoomLevel();
		float zoomTargetDiff = glm::abs(m_ZoomLevelTarget - zoomLevel);
		float zoomOffset = glm::max(glm::round(zoomTargetDiff * ts * 10000.0f) / 1000.0f, 0.001f);

		if (m_ZoomLevelTarget > zoomLevel)
			m_Camera.SetZoomLevel(glm::min(zoomLevel + zoomOffset, m_ZoomLevelTarget));

		else if (m_ZoomLevelTarget < zoomLevel)
			m_Camera.SetZoomLevel(glm::max(zoomLevel - zoomOffset, m_ZoomLevelTarget));
	}	

	void CameraController::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<MouseScrolledEvent>([&](MouseScrolledEvent& event) -> bool {
			
			float zoomOffset = m_CameraZoomSpeed * -event.GetYOffset();
			m_ZoomLevelTarget += round(zoomOffset * round(m_ZoomLevelTarget * 10.0f) * 1000.0f) / 10000.0f;
			m_ZoomLevelTarget = glm::min(glm::max(m_ZoomLevelTarget, 0.2f), 10.0f);
			return false;
		});

		dispatcher.Dispatch<WindowResizedEvent>([&](WindowResizedEvent& event) -> bool {
			
			m_AspectRatio = Application::Get().GetWindow().GetAspectRatio();
			m_Camera.SetAspectRatio(m_AspectRatio);
			return false;
		});
	}

}
