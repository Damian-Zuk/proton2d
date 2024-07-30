#pragma once
#ifdef PT_EDITOR
#include "Proton/Scene/Entity.h"

namespace proton {

	class Scene;

	// Interface for all editor panels
	class EditorPanel
	{
	public:
		virtual ~EditorPanel() = default;

		virtual void OnCreate() {};
		virtual void OnImGuiRender() = 0;
		virtual void OnUpdate(float ts) {};
		virtual void OnEvent(Event& event) {};

		virtual void OnSelectEntity(Entity entity) {}
		virtual void OnSetActiveScene(Scene* scene) {}

		virtual Entity GetSelectedEntity(bool targetFocusedViewport = true);
		virtual Scene* GetActiveScene(bool targetFocusedViewport = true);

		friend class EditorLayer;
	};

}
#endif // PT_EDITOR
