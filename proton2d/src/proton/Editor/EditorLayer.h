#pragma once
#ifdef PT_EDITOR
#include "Proton/Core/Layer.h"
#include "Proton/Editor/EditorConfig.h"
#include "Proton/Scene/Entity.h"

struct ImFont; // forward declaration

namespace proton {

	class EditorMenuBar;
	class EditorPanel;
	class SceneViewportPanel;
	class SceneHierarchyPanel;
	class InspectorPanel;

	struct EditorGameInstance
	{
		Shared<GameInstance> Instance;
		Shared<SceneViewportPanel> Viewport;
		uint32_t ID;
	};

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer() = default;

		static EditorLayer* Get();

		virtual void OnCreate() override;
		virtual void OnDestroy() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& event) override;

		static void SetActiveScene(Scene* scene);
		static void SelectEntity(Entity entity);

		static Entity GetSelectedEntity(bool targetMainInstance = false);
		static Scene* GetActiveScene(bool targetMainInstance = false);

		static Shared<GameInstance> GetMainGameInstance();
		static Shared<GameInstance> GetFocusedGameInstance();

		static Shared<SceneViewportPanel> GetMainViewportPanel();
		static Shared<SceneViewportPanel> GetFocusedViewportPanel();
		static Shared<SceneViewportPanel> GetSceneViewportPanel(GameInstance* gameInstance);
		static Shared<SceneViewportPanel> GetSceneViewportPanel(const Shared<GameInstance>& gameInstance);
		static Shared<SceneViewportPanel> GetSceneViewportPanel(Scene* scene);
		
		static SceneHierarchyPanel* GetSceneHierarchyPanel();
		static InspectorPanel* GetInspectorPanel();

		static ImFont* GetFontAwesome();
		static ImFont* GetSmallFont();

	private:
		// ImGui setup
		void SetupFonts();
		void SetupThemeStyle();
		void SetupImGuiViewports();
		void InitImGuiForGLFW();

		// OpenGL/GLFW render
		void BeginImGuiRender();
		void EndImGuiRender();

		// Running additional game instances
		Shared<GameInstance> LaunchNewGameInstance(NetMode netMode, bool loadStartupScene, const std::string& windowName);
		void CloseGameInstance(uint32_t id);
		void HandleGameInstanceCloseEvent();

		// Callbacks
		void OnStartSimulationButton();
		void OnStopSimulationButton();
		void OnPauseSimulationButton();
		
		void OnBeginSceneSimulation(Scene* scene);
		void OnStopSceneSimulation(Scene* scene);

	private:
		static EditorLayer* s_Instance;

		EditorConfig m_Config;
		Unique<EditorMenuBar> m_MenuBar;

		std::vector<EditorPanel*> m_EditorPanels; // pointers to all panels except scene viewport

		Shared<EditorGameInstance> m_MainGameInstance; // main viewport (cannot be closed)
		Shared<EditorGameInstance> m_GameInstanceContext; // pointer to focused game instance viewport
		std::vector<Shared<EditorGameInstance>> m_GameInstances;
		std::vector<uint32_t> m_GameInstancesToClose;

		uint32_t m_CurrentInstanceID = 0;

		// Automatically launch client instance when server starts
		bool m_AutostartClient = true;

		std::unordered_map<std::string, Shared<Scene>> m_SceneSimulationBackup;

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
#endif // PT_EDITOR
