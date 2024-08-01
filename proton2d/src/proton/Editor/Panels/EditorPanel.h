#pragma once
#ifdef PT_EDITOR
#include "Proton/Scene/Entity.h"

namespace proton {

	class Scene;

	// Interface
	class EditorPanel
	{
	public:
		virtual ~EditorPanel() = default;

		virtual void OnCreate() {};
		virtual void OnImGuiRender() = 0;
		virtual void OnUpdate(float ts) {};
		virtual void OnEvent(Event& event) {};

		// If `targetFocusedViewport` is false: targets MainGameInstance
		virtual Entity GetSelectedEntity(bool targetFocusedViewport = true);
		// If `targetFocusedViewport` is false: targets MainGameInstance
		virtual Scene* GetActiveScene(bool targetFocusedViewport = true);

		friend class EditorLayer;
	};

}
#endif // PT_EDITOR
