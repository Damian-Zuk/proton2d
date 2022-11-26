#pragma once

#include "proton/Core/Core.h"
#include "proton/Entity/Entity.h"

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
		Entity m_SelectedEntity;

		friend class EditorOverlay;
	};

}