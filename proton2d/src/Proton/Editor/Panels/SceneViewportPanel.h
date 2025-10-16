#pragma once
#ifdef PT_EDITOR
#include "Proton/Editor/Panels/EditorPanel.h"

namespace proton {

	class GameInstance;
	class EditorCamera;
	class Framebuffer;
	struct EditorGameInstance;

	class SceneViewportPanel : public EditorPanel
	{
	public:
		SceneViewportPanel() = default;
		virtual ~SceneViewportPanel();

		virtual void OnCreate() override;
		virtual void OnImGuiRender() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnEvent(Event& event) override;

		bool IsMainViewport() const { return m_IsMainViewport; }
		EditorCamera* GetCamera() const { return m_Camera.get(); }
		float GetAspectRatio() const { return m_ViewportAspectRatio; }
		const std::string& GetWindowName() const { return m_ImGuiWindowName; }

		void SetActiveScene(Scene* scene, bool sceneManagerCall = false) const;
		void SetSelectedEntity(Entity entity, bool sceneManagerCall = false);

		virtual Scene* GetActiveScene() const;
		Entity GetSelectedEntity() const;

	private:
		void DrawCollidersAndSelectionOutline(float ts);
		void HandleImGuiDragAndDrop();

	private:
		Shared<EditorGameInstance> m_EditorGameInstance;
		std::string m_ImGuiWindowName = "Viewport";

		bool m_IsMainViewport = true;
		bool m_IsViewportFocused = false;
		bool m_IsViewportHovered = false;

		Shared<GameInstance> m_GameInstance;
		Entity m_SelectedEntity;

		Unique<EditorCamera> m_Camera;
		Unique<Framebuffer> m_Framebuffer;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportBounds[2] = { { 0.0f, 0.0f }, {0.0f, 0.0f} };
		float m_ViewportAspectRatio = 1.0f;
		
		glm::vec2 m_MousePos = { 0.0f, 0.0f };
		glm::vec2 m_SelectionMouseOffset = { 0.0f, 0.0f };
		bool m_MoveSelectedEntity = false;

		// Debug tracing
		bool m_TraceEntitySync = true;
		bool m_ShowCullDistance = false;

		bool m_ShowSelectionOutline = true;
		bool m_ShowSelectionCollider = true;
		bool m_ShowAllColliders = false;
	
		friend class Scene;
		friend class SceneManager;

		friend class EditorLayer;
		friend class EditorCamera;
		friend class SettingsPanel;
		friend class EditorMenuBar;
		friend class ToolbarPanel;
	};

}
#endif // PT_EDITOR
