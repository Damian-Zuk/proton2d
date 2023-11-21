#pragma once
#include "proton/Editor/Panels/EditorPanel.h"

namespace proton {

	class PrefabPanel : public EditorPanel
	{
	public:
		virtual void OnImGuiRender() override;
	};

}