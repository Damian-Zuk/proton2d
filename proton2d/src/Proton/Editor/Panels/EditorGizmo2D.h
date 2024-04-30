#pragma once

#include "Proton/Scene/Entity.h"
#include "Proton/Scene/Scene.h"

namespace proton {

	class SceneViewportPanel;
	class Scene;

	class EditorGizmo2D
	{
	public:
		EditorGizmo2D();
		virtual ~EditorGizmo2D() = default;

		void Init(SceneViewportPanel* viewport);
		void Render();

	private:
		SceneViewportPanel* m_SceneViewport;
		Scene* m_ActiveScene;
		Entity m_SelectedEntity;

		friend class SceneViewportPanel;
	};

}