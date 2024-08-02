#pragma once
#ifdef PT_EDITOR

#include "Proton/Core/Base.h"
#include "Proton/Graphics/Camera.h"
#include "Proton/Events/Event.h"

namespace proton {

	class SceneViewportPanel;

	class EditorCamera {
	public:
		EditorCamera(SceneViewportPanel* viewportPanel);
		virtual ~EditorCamera() = default;

		void OnUpdate(float ts);
		void OnEvent(Event& e);

		Camera& GetBaseCamera() { return m_Camera; }
		const Camera& GetBaseCamera() const { return m_Camera; }

		const glm::vec3& GetPosition() const { return m_Position; }
		void SetPosition(const glm::vec3& position);

	private:
		SceneViewportPanel* m_ViewportPanel;

		Camera m_Camera;
		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		
		glm::vec2 m_CameraDragOffset = { 0.0f, 0.0f };
		bool m_MoveEditorCamera = false;

		float m_ZoomLevelTarget = 1.0f;
		float m_CameraZoomSpeed = 0.10f;
		bool m_UseInRuntime = false;

		friend class Scene;
		friend class SceneSerializer;
		friend class SceneViewportPanel;
		friend class SettingsPanel;
	};
}
#endif // PT_EDITOR
