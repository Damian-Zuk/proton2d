// Adapted from: https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Renderer/Font.cpp
#include "ptpch.h"
#include "Proton/Graphics/Renderer/Font.h"
#include "Proton/Graphics/Renderer/Texture.h"
#include "Proton/Graphics/Renderer/MSDFData.h"

#include "Proton/Scene/Components.h"

#undef INFINITE
#include "msdf-atlas-gen.h"
#include "FontGeometry.h"
#include "GlyphGeometry.h"

#include <filesystem>

namespace proton
{
	template<typename T, typename S, int N, msdf_atlas::GeneratorFunction<S, N> GenFunc>
	static Shared<Texture> CreateAndCacheAtlas(const std::string& fontName, float fontSize, const std::vector<msdf_atlas::GlyphGeometry>& glyphs,
		const msdf_atlas::FontGeometry& fontGeometry, uint32_t width, uint32_t height)
	{
		msdf_atlas::GeneratorAttributes attributes;
		attributes.config.overlapSupport = true;
		attributes.scanlinePass = true;

		msdf_atlas::ImmediateAtlasGenerator<S, N, GenFunc, msdf_atlas::BitmapAtlasStorage<T, N>> generator(width, height);
		generator.setAttributes(attributes);
		generator.setThreadCount(8);
		generator.generate(glyphs.data(), (int)glyphs.size());

		msdfgen::BitmapConstRef<T, N> bitmap = (msdfgen::BitmapConstRef<T, N>)generator.atlasStorage();

		TextureSpecification spec;
		spec.Width = bitmap.width;
		spec.Height = bitmap.height;
		spec.Format = ImageFormat::RGB8;
		spec.GenerateMips = false;

		Shared<Texture> texture = MakeShared<Texture>(spec);
		texture->SetData((void*)bitmap.pixels, bitmap.width * bitmap.height * 3);
		return texture;
	}

	Font::Font(const std::filesystem::path& filepath)
		: m_Data(new MSDFData())
	{
		msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
		PT_CORE_ASSERT(ft);

		std::string fileString = filepath.string();

		// TODO(Yan): msdfgen::loadFontData loads from memory buffer which we'll need 
		msdfgen::FontHandle* font = msdfgen::loadFont(ft, fileString.c_str());
		if (!font)
		{
			PT_CORE_ERROR("Failed to load font: {}", fileString);
			return;
		}

		struct CharsetRange
		{
			uint32_t Begin, End;
		};

		// From imgui_draw.cpp
		static const CharsetRange charsetRanges[] =
		{
			{ 0x0020, 0x00FF }
		};

		msdf_atlas::Charset charset;
		for (CharsetRange range : charsetRanges)
		{
			for (uint32_t c = range.Begin; c <= range.End; c++)
				charset.add(c);
		}

		double fontScale = 1.0;
		m_Data->FontGeometry = msdf_atlas::FontGeometry(&m_Data->Glyphs);
		int glyphsLoaded = m_Data->FontGeometry.loadCharset(font, fontScale, charset);
		PT_CORE_INFO("Loaded {} glyphs from font (out of {})", glyphsLoaded, charset.size());

		double emSize = 40.0;

		msdf_atlas::TightAtlasPacker atlasPacker;
		// atlasPacker.setDimensionsConstraint()
		atlasPacker.setPixelRange(2.0);
		atlasPacker.setMiterLimit(1.0);
		atlasPacker.setPadding(0);
		atlasPacker.setScale(emSize);
		int remaining = atlasPacker.pack(m_Data->Glyphs.data(), (int)m_Data->Glyphs.size());
		PT_CORE_ASSERT(remaining == 0);

		int width, height;
		atlasPacker.getDimensions(width, height);
		emSize = atlasPacker.getScale();

#define DEFAULT_ANGLE_THRESHOLD 3.0
#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull
#define THREAD_COUNT 8
		// if MSDF || MTSDF

		uint64_t coloringSeed = 0;
		bool expensiveColoring = false;
		if (expensiveColoring)
		{
			msdf_atlas::Workload([&glyphs = m_Data->Glyphs, &coloringSeed](int i, int threadNo) -> bool {
				unsigned long long glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
				glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
				return true;
				}, (int)m_Data->Glyphs.size()).finish(THREAD_COUNT);
		}
		else {
			unsigned long long glyphSeed = coloringSeed;
			for (msdf_atlas::GlyphGeometry& glyph : m_Data->Glyphs)
			{
				glyphSeed *= LCG_MULTIPLIER;
				glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
			}
		}

		m_AtlasTexture = CreateAndCacheAtlas<uint8_t, float, 3, msdf_atlas::msdfGenerator>("Test", (float)emSize, m_Data->Glyphs, m_Data->FontGeometry, width, height);

#if 0
		msdfgen::Shape shape;
		if (msdfgen::loadGlyph(shape, font, 'C'))
		{
			shape.normalize();
			//                      max. angle
			msdfgen::edgeColoringSimple(shape, 3.0);
			//           image width, height
			msdfgen::Bitmap<float, 3> msdf(32, 32);
			//                     range, scale, translation
			msdfgen::generateMSDF(msdf, shape, 4.0, 1.0, msdfgen::Vector2(4.0, 4.0));
			msdfgen::savePng(msdf, "output.png");
		}
#endif

		msdfgen::destroyFont(font);
		msdfgen::deinitializeFreetype(ft);
	}

	glm::vec2 Font::CalculateTextSize(const std::string& string, const glm::vec2& scale, float kerning, float lineSpacing)
	{
		// Total width and height of the text
		double totalWidth = 0.0f;
		double totalHeight = 0.0f;

		double lineWidth = 0.0f; // Width of the current line of text
		int lineCount = 1;      // Number of lines of text (starts with 1)

		// Get font metrics
		const auto& fontGeometry = m_Data->FontGeometry;
		const auto& metrics = fontGeometry.getMetrics();

		// Scaling factors for width and height
		double scaleX = glm::abs(scale.x) * 0.85;
		double scaleY = glm::abs(scale.y) * 0.55;

		double maxAscender = metrics.ascenderY * scaleY;
		double maxDescender = metrics.descenderY * scaleY;

		for (size_t i = 0; i < string.size(); i++)
		{
			const char& character = string[i];

			// Handle new line character
			if (character == '\n')
			{
				// Start a new line
				totalWidth = glm::max(totalWidth, lineWidth); // Update total width if the current line is wider
				lineWidth = 0.0f;  // Reset line width for the new line
				lineCount++;       // Increment the line count
				continue;
			}

			// Get the glyph geometry for the character
			const msdf_atlas::GlyphGeometry* glyph = fontGeometry.getGlyph(character);
			if (!glyph)
			{
				// Handle missing glyphs (e.g., space, special characters) by adding a space width
				const msdf_atlas::GlyphGeometry* spaceGlyph = fontGeometry.getGlyph(' ');
				double spaceAdvance = spaceGlyph ? spaceGlyph->getAdvance() : 0.3; // Rough estimate for space if missing
				lineWidth += scaleX * spaceAdvance;
				continue;
			}

			// Get the advance width of the glyph and apply kerning
			double advance = glyph->getAdvance() * scaleX + kerning;  // Apply kerning here carefully
			lineWidth += advance;

			// Update max ascender and descender based on the glyph's actual bounds
			double pl, pb, pr, pt;
			glyph->getQuadPlaneBounds(pl, pb, pr, pt);

			maxAscender = glm::max(maxAscender, pt * scaleY);
			maxDescender = glm::min(maxDescender, pb * scaleY);
		}

		// Finalize total width for the last line
		totalWidth = glm::max(totalWidth, lineWidth);

		// Calculate total height based on number of lines and max ascender/descender values
		totalHeight = (maxAscender - maxDescender) * lineCount + (lineCount - 1) * lineSpacing;

		// Adjust height to consider baseline shift (ascender height minus descender height) for each line
		return glm::vec2(totalWidth, totalHeight);
	}

	glm::vec2 Font::CalculateTextSize(TextComponent* textComponent, const glm::vec2& scale)
	{
		return CalculateTextSize(textComponent->TextString, scale, textComponent->Kerning, textComponent->LineSpacing);
	}

	Font::~Font()
	{
		delete m_Data;
	}

	Shared<Font> Font::GetDefault()
	{
		static Shared<Font> DefaultFont;
		if (!DefaultFont)
			DefaultFont = MakeShared<Font>("content/fonts/Roboto.ttf");

		return DefaultFont;
	}

}