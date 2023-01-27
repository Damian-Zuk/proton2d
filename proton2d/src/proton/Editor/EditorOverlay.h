#pragma once
#include "proton/Core/AppLayer.h"
#include "proton/Scene/Entity.h"

#include "proton/Editor/Inspector.h"
#include "proton/Editor/DebugInfo.h"
#include "proton/Editor/EditorCamera.h"

namespace proton {

	class EditorOverlay : AppLayer
	{
	public:
		EditorOverlay();
		virtual ~EditorOverlay() = default;

		virtual void OnCreate() override;
		virtual void OnDestroy() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& event) override;

		static EditorOverlay* Get() { return s_Instance; }

		static void SetSceneContext(Scene* context);
		static void SetInspectorContext(Entity entity);
		static Entity GetInspectorContext();
		static EditorCamera& GetCamera() { return s_Instance->m_Camera; }
		static bool UsingCameraEditorInRuntime() { return s_Instance->m_UseEditorCameraInRuntime; }

	private:
		void BeginImGuiRender();
		void EndImGuiRender();

		void DrawEntityTreeNode(Entity entity);

		void DrawScenePanel();
		void DrawPrefabPanel();
		void DrawCollidersAndSelectionOutline();

		void ResetCameraPosition();

	private:
		static EditorOverlay* s_Instance;

		Scene* m_ActiveScene = nullptr;

		Inspector m_Inspector;
		DebugInfo m_DebugInfo;
		EditorCamera m_Camera;

		Entity m_DraggedEntity;

		bool m_UseEditorCameraInRuntime = false;
		bool m_ShowSelectionOutline = true;
		bool m_ShowSelectionCollider = true;
		bool m_ShowAllColliders = false;

		bool m_MovingSelection = false;
		bool m_MovingCamera = false;
		glm::vec2 m_SelectionMouseOffset = { 0.0f, 0.0f };
		glm::vec2 m_CameraDragOffset = { 0.0f, 0.0f };

		float m_SavedSceneTextTimer = 0.0f;

		friend class Application;
		friend class Inspector;
		friend class DebugInfo;
		friend class EditorCamera;
	};

}