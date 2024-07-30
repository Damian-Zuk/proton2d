#pragma once
#ifdef PT_EDITOR
#include "Proton/Editor/Panels/EditorPanel.h"
#include "Proton/Graphics/Renderer/Framebuffer.h"

namespace proton {

	class GameInstance;
	class EditorCamera;

	class SceneViewportPanel : public EditorPanel
	{
	public:
		virtual ~SceneViewportPanel();

		virtual void OnCreate() override;
		virtual void OnImGuiRender() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnEvent(Event& event) override;

		virtual void OnSelectEntity(Entity entity) override;
		virtual void OnSetActiveScene(Scene* scene) override;

		EditorCamera* GetCamera() const { return m_Camera.get(); }
		float GetAspectRatio() const { return m_ViewportAspectRatio; }

		const std::string& GetWindowName() const { return m_ImGuiWindowName; }

		void SetActiveScene(Scene* scene) const;
		void SetSelectedEntity(Entity entity);

		Scene* GetActiveScene() const;
		Entity GetSelectedEntity() const;

	private:
		void DrawCollidersAndSelectionOutline(float ts);
		void HandleImGuiDragAndDrop();

	private:
		std::string m_ImGuiWindowName = "Viewport";

		bool m_IsMainViewport = true;
		bool m_IsViewportFocused = false;

		GameInstance* m_GameInstance = nullptr;
		Entity m_SelectedEntity;

		Unique<EditorCamera> m_Camera;
		Shared<Framebuffer> m_Framebuffer;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportBounds[2] = { { 0.0f, 0.0f }, {0.0f, 0.0f} };
		float m_ViewportAspectRatio = 1.0f;
		
		glm::vec2 m_MousePos = { 0.0f, 0.0f };
		glm::vec2 m_SelectionMouseOffset = { 0.0f, 0.0f };
		glm::vec2 m_CameraDragOffset = { 0.0f, 0.0f };

		bool m_ViewportFocused = false;
		bool m_ViewportHovered = false;

		bool m_MoveSelectedEntity = false;
		bool m_MoveEditorCamera = false;

		bool m_ShowSelectionOutline = true;
		bool m_ShowSelectionCollider = true;
		bool m_ShowAllColliders = false;
		bool m_ShowNetPosition = false;
	
		friend class Scene;
		friend class SceneManager;

		friend class EditorLayer;
		friend class SettingsPanel;
		friend class EditorMenuBar;
	};

}
#endif
