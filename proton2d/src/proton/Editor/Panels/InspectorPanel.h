#pragma once

#include "proton/Editor/Panels/EditorPanel.h"
#include "proton/Core/Base.h"
#include "proton/Scene/Entity.h"

namespace proton {

	class InspectorPanel : public EditorPanel
	{
	public:
		virtual void OnImGuiRender() override;

	private:
		void DrawSceneProporties();

		template<typename T>
		void DrawComponentUI(const std::string& name, const std::function<void(T&)>& drawContentFunction);

	private:
		char m_SceneNameBuffer[256] = { 0 };

		friend class EditorLayer;
	};

}
