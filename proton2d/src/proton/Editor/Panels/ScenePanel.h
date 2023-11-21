#pragma once
#include "proton/Editor/Panels/EditorPanel.h"

namespace proton {

	class ScenePanel : public EditorPanel
	{
	public:
		virtual void OnImGuiRender() override;
		virtual void OnUpdate(float ts) override;

	private:
		float m_SavedSceneTextTimer = 0.0f;
	};

}