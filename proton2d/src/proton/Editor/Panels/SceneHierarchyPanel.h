#pragma once
#include "proton/Editor/Panels/EditorPanel.h"

namespace proton {

	class SceneHierarchyPanel : public EditorPanel 
	{
	public:
		virtual void OnImGuiRender() override;
	
	private:
		void DrawEntityTreeNode(Entity entity);

	private:
		Entity m_EntityDragTarget;
	};

}
