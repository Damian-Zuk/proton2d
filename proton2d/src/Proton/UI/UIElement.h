#pragma once

#include "Proton/UI/UIElementType.h"

namespace proton {

	struct TransformComponent;

	class UIElement
	{
	public:
		UIElement(UIElementType type);

		virtual bool OnCreate() { return true; };
		virtual void OnDestroy() {};
		virtual void OnUpdate() {};
		virtual void Draw(TransformComponent* transform) = 0;

		void Hide(bool hidden = true);
		bool IsHidden() const { return m_Hidden; }

	private:
		UIElementType m_ElementType;
		bool m_Hidden = false;

		friend class InspectorPanel;
	};

}
