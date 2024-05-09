#pragma once

#include "Proton/UI/UIElement.h"
#include "Proton/Graphics/Renderer/Font.h"

namespace proton {

	class Font;

	class UIText : public UIElement
	{
	public:
		UIText();

		virtual void Draw(TransformComponent* transform) override;

	private:
		Shared<Font> m_FontAsset = Font::GetDefault();

		std::string m_TextString;
		glm::vec4 m_Color { 1.0f };
		float m_Kerning = 0.0f;
		float m_LineSpacing = 0.0f;

		friend class InspectorPanel;
	};

}
