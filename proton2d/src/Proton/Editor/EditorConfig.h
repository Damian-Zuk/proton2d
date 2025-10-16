#pragma once

#ifdef PT_EDITOR

namespace proton {

	class EditorConfig
	{
	public:
		EditorConfig() = default;
		virtual ~EditorConfig() = default;

		struct Font
		{
			std::string FontFilepath;
			float  FontSize;
		};
		std::map<std::string, Font> EditorFonts;

		void LoadConfig();
		void WriteConfig();

	private:
		std::string m_Filepath = "editor/editor-config.json";
	};
}

#endif
