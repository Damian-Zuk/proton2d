#pragma once
#ifdef PT_EDITOR
#include "Proton/Editor/EditorCamera.h"
#include "Proton/Editor/EditorMenuBar.h"
#include "Proton/Core/AppLayer.h"
#include "Proton/Core/Config.h"
#include "Proton/Scene/Entity.h"
#include "Proton/Graphics/Renderer/Framebuffer.h"

struct ImFont;

namespace proton {

	class EditorPanel;
	class SceneViewportPanel;

	class EditorLayer : AppLayer
	{
	public:
		EditorLayer() = default;
		virtual ~EditorLayer() = default;

		static EditorLayer* Get();

		virtual void OnCreate() override;
		virtual void OnDestroy() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& event) override;

		static EditorCamera* GetCamera();
		static SceneViewportPanel* GetSceneViewportPanel();
		static ImFont* GetFontAwesome();
		static ImFont* GetSmallFont();

		static void SetActiveScene(Scene* scene);
		static void SelectEntity(Entity entity);

		GameInstance* GetFocusedGameInstance();

	private:
		// ImGui setup
		void SetupFonts();
		void SetupThemeStyle();
		void SetupImGuiViewports();
		void InitializeImGui();

		// OpenGL/GLFW render
		void BeginImGuiRender();
		void EndImGuiRender();

		// Callbacks
		void OnPlayButton();
		void OnStopButton();
		void OnPauseButton();
		void OnBeginSceneSimulation(Scene* scene);
		void OnStopSceneSimulation(Scene* scene);

	private:
		bool m_EnableViewports = true; // true: ImGui windows can be detached from main GLFW window

		Scene* m_ActiveScene = nullptr;
		Entity m_SelectedEntity;

		std::unordered_map<std::string, Shared<Scene>> m_SceneBackup;

		EditorConfig m_Config;
		EditorMenuBar m_MenuBar;

		std::vector<EditorPanel*> m_EditorPanels;
		bool m_BlockEvents = true;

		uint32_t m_NetNumClients = 1;
		std::vector<Unique<GameInstance>> m_ClientGameInstances;
		std::vector<Unique<SceneViewportPanel>> m_ClientViewports;
		GameInstance* m_FocusedGameInstance = nullptr;

		friend class Application;
		friend class Scene;

		friend class SceneViewportPanel;
		friend class InspectorPanel;
		friend class SettingsPanel;
		friend class InfoPanel;
		friend class ToolbarPanel;
		friend class EditorCamera;
		friend class EditorMenuBar;
	};

}
#endif
