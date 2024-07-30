#include "ptpch.h"
#include "Proton/Editor/Panels/EditorPanel.h"
#include "Proton/Editor/EditorLayer.h"

namespace proton {

	Entity EditorPanel::GetSelectedEntity(bool targetFocusedViewport)
	{
		return EditorLayer::GetSelectedEntity(targetFocusedViewport);
	}

	Scene* EditorPanel::GetActiveScene(bool targetFocusedViewport)
	{
		return EditorLayer::GetActiveScene(targetFocusedViewport);
	}

}
