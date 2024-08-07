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

		virtual Entity GetSelectedEntity(bool targetMainInstance = false);
		virtual Scene* GetActiveScene(bool targetMainInstance = false);

		friend class EditorLayer;
	};

}
#endif // PT_EDITOR
