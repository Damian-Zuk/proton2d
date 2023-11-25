#pragma once
#include "Proton/Scene/Entity.h"

namespace proton {

	class Scene;

	// Abstract class for all editor panels
	class EditorPanel
	{
	public:
		virtual ~EditorPanel() = default;

		virtual void OnImGuiRender() = 0;
		virtual void OnUpdate(float ts) {};
	
	protected:
		Scene* m_ActiveScene = nullptr;
		Entity m_SelectedEntity;

		friend class EditorLayer;
	};

}
