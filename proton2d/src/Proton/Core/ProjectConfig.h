#pragma once

namespace proton {

	class ProjectConfig
	{
	public:
		std::string StartScene;
	
	public:
		ProjectConfig() = default;
		virtual ~ProjectConfig() = default;

		bool LoadConfig();
		void WriteConfig();

	private:
		const std::string m_Filepath = "content/project.json";
	};

}
