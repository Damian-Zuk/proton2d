#pragma once
#include "proton/Core/Layer.h"
#include "proton/Entity/Entity.h"
#include "proton/Editor/Inspector.h"

namespace proton {

	class EditorOverlay : public Layer
	{
	public:
		EditorOverlay();
		virtual ~EditorOverlay() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnImGuiRender() override;

		void SetActiveScene(const Shared<Scene>& scene);
	private:
		Inspector m_Inspector;
		Shared<Scene> m_ActiveScene;
		Entity m_SelectedEntity;
	};

}