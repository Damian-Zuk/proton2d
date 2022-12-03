#pragma once

#include "proton/Core/Core.h"
#include "proton/Graphics/Camera.h"
#include "proton/Events/Event.h"

namespace proton {

	class EditorCameraController {
	public:
		EditorCameraController(const Shared<Camera>& camera = CreateShared<Camera>());
		virtual ~EditorCameraController() = default;

		void OnUpdate(float ts);
		void OnEvent(Event& e);

		Shared<Camera> GetCamera() { return m_Camera; }
		const Shared<Camera> GetCamera() const { return m_Camera; }

	private:
		Shared<Camera> m_Camera;
		float m_AspectRatio = 16.0f / 9.0f;
		float m_CameraSpeed = 3.0f;
		float m_ZoomLevelTarget = 1.0f;
		float m_CameraZoomSpeed = 0.10f;
	};
}