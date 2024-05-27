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
	class SceneHierarchyPanel;
	class InspectorPanel;

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

		static ImFont* GetFontAwesome();
		static ImFont* GetSmallFont();
		static EditorCamera* GetCamera();

		static SceneViewportPanel* GetSceneViewportPanel();
		static SceneHierarchyPanel* GetSceneHierarchyPanel();
		static InspectorPanel* GetInspectorPanel();

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

		void OnAddClientButton();
		void OnRemoveClientButton();

		void OpenNewClientGameInstance(uint32_t instanceID);
		void CloseClientGameInstance(uint32_t instanceID);

	private:
		bool m_EnableViewports = false; // true: ImGui windows can be detached from main GLFW window
		uint32_t m_SimulatedScenes = 0;

		GameInstance* m_GameInstance;
		Scene* m_ActiveScene = nullptr;
		Entity m_SelectedEntity;

		std::unordered_map<std::string, Shared<Scene>> m_SceneBackup;

		EditorConfig m_Config;
		EditorMenuBar m_MenuBar;

		std::vector<EditorPanel*> m_EditorPanels;
		bool m_BlockEvents = false;
		GameInstance* m_FocusedGameInstance = nullptr;

		uint32_t m_NetNumClients = 1;

		struct EditorClientInstance
		{
			Unique<GameInstance> Instance;
			Unique<SceneViewportPanel> Viewport;
			uint32_t ID;
		};
		std::vector<EditorClientInstance> m_ClientInstances;
		std::vector<uint32_t> m_ClientInstancesToClose;
		std::vector<uint32_t> m_FreeClientInstanceID;

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
