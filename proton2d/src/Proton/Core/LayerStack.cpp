#include "ptpch.h"
#include "Proton/Core/LayerStack.h"

namespace proton {

	LayerStack::~LayerStack()
	{
		for (const auto& layer : m_Layers)
		{
			layer->OnDestroy();
			delete layer;
		}
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
		if (it != m_Layers.begin() + m_LayerInsertIndex)
		{
			layer->OnDestroy();
			delete layer;
			m_Layers.erase(it);
			m_LayerInsertIndex--;
		}
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
		if (it != m_Layers.end())
		{
			overlay->OnDestroy();
			delete overlay;
			m_Layers.erase(it);
		}
	}

}