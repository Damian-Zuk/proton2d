#pragma once

#include "proton/Core/Core.h"
#include "proton/Scene/Entity.h"

namespace proton {

	class Inspector
	{
	public:
		void OnImGuiRender();

	private:
		template<typename T>
		void DrawComponentUI(const std::string& name, const std::function<void(T&)>& drawContentFunction);

		void SetSelectionContext(Entity entity);

	private:
		Scene* m_ActiveScene;
		Entity m_SelectedEntity; // Inspector Context

		char m_SceneNameBuffer[256] = { 0 };

		friend class EditorOverlay;
	};

}