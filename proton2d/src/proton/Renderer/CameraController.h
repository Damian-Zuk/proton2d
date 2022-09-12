#pragma once
#include "proton/Core/Core.h"
#include "proton/Renderer/Camera.h"
#include "proton/Events/Event.h"

namespace proton {

	class CameraController {
	public:
		CameraController();
		virtual ~CameraController();

		void OnUpdate(float ts);
		void OnEvent(Event& e);

		Camera& GetCamera() { return m_Camera; }
		const Camera& GetCamera() const { return m_Camera; }

	private:
		float m_AspectRatio;
		Camera m_Camera;
		glm::vec2 m_CameraSpeed = glm::vec2(0.7f);
		float m_ZoomLevelTarget = 1.0f;
		float m_CameraZoomSpeed = 0.10f;
	};
}