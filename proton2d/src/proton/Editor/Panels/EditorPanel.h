#pragma once
#include "proton/Scene/Entity.h"

namespace proton {

	class Scene;

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
