#pragma once

#include "proton/Core/Core.h"
#include "proton/Graphics/Camera.h"
#include "proton/Events/Event.h"

namespace proton {

	class CameraController {
	public:
		CameraController(Shared<Camera> camera = CreateShared<Camera>());
		virtual ~CameraController();

		void OnUpdate(float ts);
		void OnEvent(Event& e);

		Shared<Camera> GetCamera() { return m_Camera; }
		const Shared<Camera> GetCamera() const { return m_Camera; }

	private:
		float m_AspectRatio;
		Shared<Camera> m_Camera;
		glm::vec2 m_CameraSpeed = glm::vec2(0.7f);
		float m_ZoomLevelTarget = 1.0f;
		float m_CameraZoomSpeed = 0.10f;
	};
}