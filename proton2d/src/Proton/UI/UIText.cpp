#include "ptpch.h"
#include "Proton/UI/UIText.h"
#include "Proton/Graphics/Renderer/Renderer.h"
#include "Proton/Utils/Utils.h"

namespace proton {

	UIText::UIText()
		: UIElement(UIElementType::Text)
	{
	}

	void UIText::Draw(TransformComponent* transform)
	{
		Renderer::DrawString(m_TextString, m_FontAsset,
			Math::GetTransform(transform->WorldPosition, transform->Scale, transform->Rotation),
			Renderer::TextParams{m_Color, m_Kerning, m_LineSpacing}
		);
	}

}
