// From: https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Renderer/Font.h
#pragma once

namespace proton {

	struct MSDFData;
	class Texture;

	class Font
	{
	public:
		Font(const std::filesystem::path& font);
		~Font();

		const MSDFData* GetMSDFData() const { return m_Data; }
		Shared<Texture> GetAtlasTexture() const { return m_AtlasTexture; }

		static Shared<Font> GetDefault();
	private:
		MSDFData* m_Data;
		Shared<Texture> m_AtlasTexture;
	};

}