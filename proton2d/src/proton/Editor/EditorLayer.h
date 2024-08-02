#pragma once
#ifdef PT_EDITOR
#include "Proton/Editor/Tools/EditorCamera.h"
#include "Proton/Editor/Menu/EditorMenuBar.h"
#include "Proton/Core/AppLayer.h"
#include "Proton/Core/Config.h"
#include "Proton/Scene/Entity.h"

struct ImFont; // forward declaration

namespace proton {

	class EditorPanel;
	class SceneViewportPanel;
	class SceneHierarchyPanel;
	class InspectorPanel;

	class EditorLayer : AppLayer
	{
	public:
		EditorLayer(GameInstance* instance);
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
		static SceneViewportPanel* GetSceneViewportPanel(GameInstance* gameInstance = nullptr);
		static SceneViewportPanel* GetSceneViewportPanel(Scene* scene);
		static SceneHierarchyPanel* GetSceneHierarchyPanel();
		static InspectorPanel* GetInspectorPanel();

		static void SetActiveScene(Scene* scene);
		static void SelectEntity(Entity entity);

		// If `targetFocusedViewport` is false: targets MainGameInstance
		static Entity GetSelectedEntity(bool targetFocusedViewport = true);
		// If `targetFocusedViewport` is false: targets MainGameInstance
		static Scene* GetActiveScene(bool targetFocusedViewport = true);

		static GameInstance* GetFocusedGameInstance();
		static SceneViewportPanel* GetFocusedViewportPanel();

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
		void OnStartSimulationButton();
		void OnStopSimulationButton();
		void OnPauseSimulationButton();
		
		void OnBeginSceneSimulation(Scene* scene);
		void OnStopSceneSimulation(Scene* scene);

		GameInstance* OpenNewClientGameInstance(NetMode netMode, bool loadStartupScene, const std::string& windowName);
		void CloseClientGameInstance(uint32_t id);
		void HandleClientGameInstanceCloseEvent();

	private:
		static EditorLayer* s_Instance;

		bool m_EnableViewports = false; // true: ImGui windows can be detached from main GLFW window
		bool m_BlockEvents = false;

		std::vector<EditorPanel*> m_EditorPanels;
		GameInstance* m_MainGameInstance; // owned by Application
		GameInstance* m_FocusedGameInstance;

		EditorConfig m_Config;
		EditorMenuBar m_MenuBar;

		uint32_t m_SimulatedScenes = 0;
		std::unordered_map<std::string, Shared<Scene>> m_SimulatedScenesBackup;

		struct EditorClientInstance
		{
			Unique<GameInstance> Instance;
			Unique<SceneViewportPanel> Viewport;
			uint32_t ID;
		};
		std::vector<EditorClientInstance> m_ClientInstances;
		std::unordered_map<GameInstance*, SceneViewportPanel*> m_ClientViewportMap;	
		std::vector<uint32_t> m_ClientInstancesToClose;
		uint32_t m_CurrentInstanceID = 0;

		friend class Application;
		friend class Scene;

		friend class SceneViewportPanel;
		friend class InspectorPanel;
		friend class SettingsPanel;
		friend class InfoPanel;
		friend class ToolbarPanel;
		friend class EditorCamera;
		friend class EditorMenuBar;
		friend class NewInstanceModal;
	};

}
#endif
