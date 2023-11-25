#pragma once
#include "Proton/Editor/Panels/EditorPanel.h"

namespace proton {

	// TODO: Refactor
	class PrefabPanel : public EditorPanel
	{
	public:
		virtual void OnImGuiRender() override;
	};

}
