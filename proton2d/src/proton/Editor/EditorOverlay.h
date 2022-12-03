#pragma once
#include "proton/Core/AppLayer.h"
#include "proton/Entity/Entity.h"

#include "proton/Editor/Inspector.h"
#include "proton/Editor/DebugInfo.h"
#include "proton/Editor/EditorCameraController.h"

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

		static void SetSceneContext(Scene* context);
		static void SetInspectorContext(Entity entity);
		static Entity GetInspectorContext();

	private:
		void BeginImGuiRender();
		void EndImGuiRender();

		void DrawEntityTreeNode(Entity entity);
		void DrawSceneSerializationPanel();

		void TryMouseSelectEntity();

	private:
		static EditorOverlay* s_Instance;

		bool m_DrawSelectedEntityOutline = true;

		Inspector m_Inspector;
		DebugInfo m_DebugInfo;

		EditorCameraController m_CameraController;

		Scene* m_ActiveScene = nullptr;

		friend class Application;
		friend class Scene;
		friend class DebugInfo;
	};

}