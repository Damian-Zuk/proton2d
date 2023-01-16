#pragma once

#include "proton/Core/Core.h"
#include "proton/Graphics/Camera.h"
#include "proton/Events/Event.h"

namespace proton {

	class EditorCamera {
	public:
		EditorCamera(const Shared<Camera>& camera = CreateShared<Camera>());
		virtual ~EditorCamera() = default;

		void OnUpdate(float ts);
		void OnEvent(Event& e);

		Shared<Camera> GetCamera() { return m_Camera; }
		const Shared<Camera>& GetCamera() const { return m_Camera; }

		const glm::vec3& GetPosition() const { return m_Position; }

	private:
		Shared<Camera> m_Camera;
		
		glm::vec3 m_Position;

		float m_AspectRatio;
		float m_CameraSpeed = 3.0f;
		float m_ZoomLevelTarget = 1.0f;
		float m_CameraZoomSpeed = 0.10f;

		friend class EditorOverlay;
	};
}