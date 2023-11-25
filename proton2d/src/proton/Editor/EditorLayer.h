#pragma once
#include "Proton/Editor/EditorCamera.h"
#include "Proton/Core/AppLayer.h"
#include "Proton/Scene/Entity.h"

namespace proton {

	class EditorPanel;

	class EditorLayer : AppLayer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer() = default;

		virtual void OnCreate() override;
		virtual void OnDestroy() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& event) override;

		static EditorLayer* Get() { return s_Instance; }

		static void SetActiveScene(Scene* scene);
		static void SelectEntity(Entity entity);

		static EditorCamera& GetCamera() { return s_Instance->m_Camera; }

	private:
		void BeginImGuiRender();
		void EndImGuiRender();

		void PushEditorPanel(const std::string& name, EditorPanel* panel);
		void PopEditorPanel(const std::string& name);

		void DrawCollidersAndSelectionOutline();
		void ResetCameraPosition();

	private:
		static EditorLayer* s_Instance;
		Scene* m_ActiveScene = nullptr;
		Entity m_SelectedEntity;

		std::vector<std::pair<std::string, EditorPanel*>> m_EditorPanels;
		EditorCamera m_Camera;

		// Editor options
		bool m_UseEditorCameraInRuntime = false;
		bool m_ShowSelectionOutline = true;
		bool m_ShowSelectionCollider = true;
		bool m_ShowAllColliders = false;

		// Entity selection
		bool m_MoveSelectedEntity = false;
		bool m_MoveEditorCamera = false;
		glm::vec2 m_SelectionMouseOffset = { 0.0f, 0.0f };
		glm::vec2 m_CameraDragOffset = { 0.0f, 0.0f };

		friend class Application;
		friend class Scene;

		friend class InspectorPanel;
		friend class MiscellaneousPanel;
		friend class EditorCamera;
	};

}
