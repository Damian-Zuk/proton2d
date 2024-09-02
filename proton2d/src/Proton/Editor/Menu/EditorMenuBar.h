#pragma once
#ifdef PT_EDITOR

namespace proton {

	class SceneManager;

	class EditorMenuBar
	{
	public:
		void OnCreate();
		void OnImGuiRender();

		void NewScene();
		void OpenScene();
		void SaveScene();
		void SaveSceneAs();

	private:
		void HandleProjectProportiesPopup();

	private:
		SceneManager* m_SceneManager;
		
		bool m_OpenProjectProporties = false;

	};

}

#endif // PT_EDITOR
