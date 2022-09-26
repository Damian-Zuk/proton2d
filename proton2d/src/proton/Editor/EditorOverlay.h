#pragma once
#include "proton/Core/Layer.h"
#include "proton/Entity/Entity.h"
#include "proton/Editor/Inspector.h"

namespace proton {

	class EditorOverlay : Layer
	{
	public:
		virtual ~EditorOverlay() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;

		void SetActiveScene(const Shared<Scene>& scene);

	private:
		void BeginImGuiRender();
		void EndImGuiRender();

	private:
		Inspector m_Inspector;
		Shared<Scene> m_ActiveScene;
		Entity m_SelectedEntity;

		friend class Application;
	};

}