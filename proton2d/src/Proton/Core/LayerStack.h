#pragma once

#include "Proton/Core/Layer.h"

namespace proton {

	class LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		template<typename TLayer>
		void PushLayer()
		{
			TLayer* layer = new TLayer();
			m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
			m_LayerInsertIndex++;
			layer->OnCreate();
			return layer;
		}

		template<typename TLayer>
		TLayer* PushOverlay()
		{
			TLayer* overlay = new TLayer();
			m_Layers.emplace_back(overlay);
			overlay->OnCreate();
			return overlay;
		}

		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_Layers.end(); }
		std::vector<Layer*>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
		std::vector<Layer*>::reverse_iterator rend() { return m_Layers.rend(); }

		std::vector<Layer*>::const_iterator begin() const { return m_Layers.begin(); }
		std::vector<Layer*>::const_iterator end()	const { return m_Layers.end(); }
		std::vector<Layer*>::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
		std::vector<Layer*>::const_reverse_iterator rend() const { return m_Layers.rend(); }

	private:
		std::vector<Layer*> m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};

}