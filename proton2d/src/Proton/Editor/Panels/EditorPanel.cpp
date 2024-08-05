#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/Panels/EditorPanel.h"
#include "Proton/Editor/EditorLayer.h"

namespace proton {

	Entity EditorPanel::GetSelectedEntity(bool mainInstanceOnly)
	{
		return EditorLayer::GetSelectedEntity(mainInstanceOnly);
	}

	Scene* EditorPanel::GetActiveScene(bool mainInstanceOnly)
	{
		return EditorLayer::GetActiveScene(mainInstanceOnly);
	}

}
#endif // PT_EDITOR
