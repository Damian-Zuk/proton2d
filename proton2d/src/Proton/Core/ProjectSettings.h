#pragma once

namespace proton {

	class ProjectSettings
	{
	public:
		bool LoadProjectSettings();
		void WriteProjectSettings();

	private:
		const std::string m_Filepath = "content/project.json";
		
		std::string m_StartScene;
		std::string m_IpAddress = "127.0.0.1";
		int m_Port = 8192;

		friend class GameInstance;
		friend class SettingsPanel;
		friend class EditorMenuBar;
	};

}
