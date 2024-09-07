// Adapted from: https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Renderer/Font.h
#pragma once

namespace proton {

	struct MSDFData;
	class Texture;
	struct TextComponent;

	class Font
	{
	public:
		Font(const std::filesystem::path& font);
		~Font();

		const MSDFData* GetMSDFData() const { return m_Data; }
		Shared<Texture> GetAtlasTexture() const { return m_AtlasTexture; }

		glm::vec2 CalculateTextSize(const std::string& string, const glm::vec2& scale, float kerning, float lineSpacing);
		glm::vec2 CalculateTextSize(TextComponent* textComponent, const glm::vec2& scale);

		static Shared<Font> GetDefault();
	private:
		MSDFData* m_Data;
		Shared<Texture> m_AtlasTexture;
	};

}