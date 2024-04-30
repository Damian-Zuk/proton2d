#include "ptpch.h"
#include "Proton/Editor/Panels/EditorGizmo2D.h"
#include "Proton/Editor/Panels/SceneViewportPanel.h"
#include "Proton/Editor/EditorCamera.h"
#include "Proton/Graphics/Renderer/Renderer.h"
#include "Proton/Utils/Utils.h"

namespace proton {

	static constexpr glm::vec4 COLOR_WHITE = glm::vec4{ 1.0f };
	static constexpr glm::vec4 COLOR_YELLOW = { 0.8f, 0.8f, 0.2f, 1.0f };
	static constexpr glm::vec4 COLOR_COLLIDER = { 0.9f, 0.6f, 0.3f, 0.2f };
	static constexpr glm::vec4 COLOR_OUTLINE = { 0.95f, 0.25f, 0.18f, 0.75f };
	static constexpr glm::vec4 COLOR_SENSOR_OUTLINE = { 0.2f, 0.85f, 0.15f, 1.0f };

	EditorGizmo2D::EditorGizmo2D()
		: m_SceneViewport(nullptr), m_ActiveScene(nullptr)
	{
	}

	void EditorGizmo2D::Init(SceneViewportPanel* viewport)
	{
		m_SceneViewport = viewport;
	}

	void EditorGizmo2D::Render()
	{
		if (m_SelectedEntity)
		{
			//EditorCamera* editorCamera = m_SceneViewport->GetCamera();
			//Camera& camera = editorCamera->GetBaseCamera();
			//

			//Renderer::BeginScene(camera, editorCamera->GetPosition());

			//auto& transform = m_SelectedEntity.GetTransform();
			//glm::vec3 position = { transform.WorldPosition.x, transform.WorldPosition.y, 0.8f };
			//glm::vec2 scale = glm::vec2(camera.GetZoomLevel(), camera.GetZoomLevel());

			//Renderer::DrawQuad(Math::GetTransform(position, glm::vec2{ 0.2, 0.2 } * scale), COLOR_YELLOW);

			//Renderer::EndScene();
		}
	}

}
