#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/EditorConfig.h"
#include "Proton/Utils/Utils.h"

namespace proton {

	void EditorConfig::LoadConfig()
	{
		EditorFonts.clear();

		if (!std::filesystem::exists(m_Filepath))
		{
			PT_CORE_WARN("'{}' file not found! Creating config with default values.", m_Filepath);
			EditorFonts.insert(std::pair("roboto", Font{ "editor/content/font/Roboto.ttf", 18.0f }));
			WriteConfig();
			return;
		}

		json jsonObj = jsonObj.parse(Utils::ReadFile(m_Filepath));
		for (const auto& font : jsonObj["Fonts"].items())
			EditorFonts.insert(std::pair(font.key(), Font{ font.value()["FontFilepath"], font.value()["FontSize"] }));
	}

	void EditorConfig::WriteConfig()
	{
		json jsonObj;
		for (const auto& [fontName, font] : EditorFonts)
		{
			jsonObj["Fonts"][fontName] = {
				{ "FontFilepath", font.FontFilepath },
				{ "FontSize", font.FontSize }
			};
		}
		std::ofstream configFile(m_Filepath);
		configFile << jsonObj.dump(4);
		configFile.close();
	}
}

#endif
