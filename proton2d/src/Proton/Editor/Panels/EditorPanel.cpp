#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/Panels/EditorPanel.h"
#include "Proton/Editor/EditorLayer.h"

namespace proton {

	Entity EditorPanel::GetSelectedEntity(bool targetMainInstance)
	{
		return EditorLayer::GetSelectedEntity(targetMainInstance);
	}

	Scene* EditorPanel::GetActiveScene(bool targetMainInstance)
	{
		return EditorLayer::GetActiveScene(targetMainInstance);
	}

}
#endif // PT_EDITOR
