#pragma once
#ifdef PT_EDITOR
#include "Proton/Editor/Panels/EditorPanel.h"

namespace proton {

	// TODO: Refactor
	class PrefabPanel : public EditorPanel
	{
	public:
		virtual void OnImGuiRender() override;
	};

}
#endif
