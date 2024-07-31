#pragma once
#ifdef PT_EDITOR

namespace proton {

	class EditorMenuBar
	{
	public:
		void OnImGuiRender();

		void NewScene();
		void OpenScene();
		void SaveScene();
		void SaveSceneAs();

	private:
		void HandleProjectProportiesPopup();

	private:
		bool m_OpenProjectProporties = false;
	};

}

#endif // PT_EDITOR
