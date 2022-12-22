#include "pch.h"

#include "proton/Editor/EditorCamera.h"
#include "proton/Core/Input.h"
#include "proton/Events/MouseEvents.h"
#include "proton/Core/Application.h"
#include "proton/Events/WindowEvents.h"

#include <imgui.h>

namespace proton {

	EditorCamera::EditorCamera(const Shared<Camera>& camera)
		: m_AspectRatio(Application::Get().GetWindow().GetAspectRatio()),
		m_Camera(camera), m_Position({ 0.0f, 0.0f, 0.0f })
	{
	}

	void EditorCamera::OnUpdate(float ts)
	{
		if (EditorOverlay::Get()->m_ActiveScene->m_SceneState != SceneState::EditMode)
			return;

		float zoomLevel = m_Camera->GetZoomLevel();

		if (!ImGui::GetIO().WantCaptureKeyboard)
		{
			if (Input::IsKeyPressed(Key::W)) 
				m_Position.y += m_CameraSpeed * m_AspectRatio * ts * zoomLevel;

			if (Input::IsKeyPressed(Key::A))
				m_Position.x += -m_CameraSpeed * ts * zoomLevel;

			if (Input::IsKeyPressed(Key::S))
				m_Position.y += -m_CameraSpeed * m_AspectRatio * ts * zoomLevel;

			if (Input::IsKeyPressed(Key::D))
				m_Position.x += m_CameraSpeed * ts * zoomLevel;
		}

		float zoomTargetDiff = glm::abs(m_ZoomLevelTarget - zoomLevel);
		float zoomOffset = glm::max(glm::round(zoomTargetDiff * ts * 10000.0f) / 1000.0f, 0.001f);

		if (m_ZoomLevelTarget > zoomLevel)
			m_Camera->SetZoomLevel(glm::min(zoomLevel + zoomOffset, m_ZoomLevelTarget));

		else if (m_ZoomLevelTarget < zoomLevel)
			m_Camera->SetZoomLevel(glm::max(zoomLevel - zoomOffset, m_ZoomLevelTarget));
	}	

	void EditorCamera::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		if (!ImGui::GetIO().WantCaptureMouse)
		{
			if (EditorOverlay::Get()->m_ActiveScene->m_SceneState == SceneState::EditMode)
			{
				dispatcher.Dispatch<MouseScrolledEvent>([&](MouseScrolledEvent& event) -> bool 
				{
					float zoomOffset = m_CameraZoomSpeed * -event.GetYOffset();
					m_ZoomLevelTarget += round(zoomOffset * round(m_ZoomLevelTarget * 10.0f) * 1000.0f) / 10000.0f;
					m_ZoomLevelTarget = glm::min(glm::max(m_ZoomLevelTarget, 0.2f), 10.0f);
					return false;
				});
			}
		}

		dispatcher.Dispatch<WindowResizedEvent>([&](WindowResizedEvent& event) -> bool 
		{
			m_AspectRatio = Application::Get().GetWindow().GetAspectRatio();
			m_Camera->SetAspectRatio(m_AspectRatio);
			return false;
		});
	}

}
