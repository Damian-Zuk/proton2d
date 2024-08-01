#pragma once
#ifdef PT_EDITOR
#include "Proton/Editor/Panels/EditorPanel.h"

namespace proton {

	class ToolbarPanel : public EditorPanel
	{
	public:
		virtual void OnImGuiRender() override;

	private:
		void DrawSceneTabBar();
		void HandleLaunchInstancePopup();
		void UpdateInstancePropsWindowTitle();

	private:

		struct LaunchInstanceProps
		{
			bool OpenPopup = false;

			NetMode NetMode = NetMode::Client;
			std::string ClientNickname;
			
			std::string ServerIp = "127.0.0.1";
			int Port = 8192;
			int ServerTickrate = 32;

			bool LoadStartupScene = false;
			std::string WindowName;

		} m_LaunchInstanceProps;
	};

}
#endif // PT_EDITOR
