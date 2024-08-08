#pragma once
#ifdef PT_EDITOR
#include "Proton/Editor/Panels/EditorPanel.h"

namespace proton {

	class InspectorPanel : public EditorPanel
	{
	public:
		virtual void OnImGuiRender() override;

	private:
		void DrawSceneProporties();

		template<typename T>
		void DrawComponentUI(const std::string& name, const std::function<void(T&)>& drawContentFunction);

		friend class EditorLayer;
		friend class SceneViewportPanel;
		friend class SettingsPanel;
	};

}

#endif // PT_EDITOR
