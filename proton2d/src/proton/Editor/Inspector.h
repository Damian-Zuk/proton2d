#pragma once

#include "proton/Core/Core.h"
#include "proton/Entity/Entity.h"

namespace proton {

	class Inspector
	{
	public:
		Inspector() = default;

		void OnImGuiRender();

	private:
		template<typename T>
		void DrawComponentUI(const std::string& name, void(*drawFunc)(T& component));

	private:
		Shared<Scene> m_ActiveScene;
		Entity m_SelectedEntity;

		friend class EditorOverlay;
	};

}