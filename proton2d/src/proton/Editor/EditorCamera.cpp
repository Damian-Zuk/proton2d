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
		if (EditorLayer::Get()->m_ActiveScene->m_SceneState != SceneState::Stop
			&& !EditorLayer::s_Instance->m_UseEditorCameraInRuntime)
			return;

		float zoomLevel = m_Camera->GetZoomLevel();
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
		Scene* activeScene = EditorLayer::Get()->m_ActiveScene;
		
		if (!ImGui::GetIO().WantCaptureMouse && activeScene
			&& (activeScene->m_SceneState == SceneState::Stop || EditorLayer::s_Instance->m_UseEditorCameraInRuntime))
		{
			dispatcher.Dispatch<MouseScrolledEvent>([&](MouseScrolledEvent& event) -> bool 
			{
				float zoomOffset = m_CameraZoomSpeed * -event.GetYOffset();
				m_ZoomLevelTarget += round(zoomOffset * round(m_ZoomLevelTarget * 10.0f) * 1000.0f) / 10000.0f;
				m_ZoomLevelTarget = glm::min(glm::max(m_ZoomLevelTarget, 0.2f), 30.0f);
				return false;
			});
		}

		dispatcher.Dispatch<WindowResizedEvent>([&](WindowResizedEvent& event) -> bool 
		{
			m_AspectRatio = Application::Get().GetWindow().GetAspectRatio();
			m_Camera->SetAspectRatio(m_AspectRatio);
			return false;
		});
	}

}
