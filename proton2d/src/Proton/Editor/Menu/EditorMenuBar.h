#pragma once
#ifdef PT_EDITOR
#include "Proton/Editor/Panels/EditorPanel.h"

namespace proton {

	class SceneManager;

	class EditorMenuBar : public EditorPanel
	{
	public:
		virtual void OnCreate() override;
		virtual void OnImGuiRender() override;

	private:
		void NewScene();
		void OpenScene();
		void SaveScene();
		void SaveSceneAs();

		void HandleProjectProportiesPopup();

	private:
		SceneManager* m_SceneManager;
		
		bool m_OpenProjectProporties = false;

	};

}

#endif // PT_EDITOR
