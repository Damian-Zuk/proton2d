#pragma once


namespace proton {

	class ApplicationConfig
	{
	public:
		ApplicationConfig();
		
		std::string WindowTitle = std::string();
		int WindowWidth = 1280;
		int WindowHeight = 720;
		bool Fullscreen = false;
		bool VSync = true;

		void LoadConfig();
		
		void WriteConfig(
			const std::string& windowTitle,
			int windowWidth,
			int windowHeight,
			bool fullscreen,
			bool vsync);

	private:
		std::string m_Filepath = "app-config.json";
	};
}

