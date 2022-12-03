#pragma once

namespace proton {

	class DebugInfo
	{
	public:
		void OnImGuiRender();

	private:
		float m_FrameTime = 0.0f;
		float m_FrameTimeDisplay = 0.0f;
		float m_AvgFrameTime = 0.0f;
		
		uint32_t m_FrameCount = 0;
		float m_FPS = 0.0f;

		size_t m_EntitiesCount = 0;
		size_t m_ScriptedEntitiesCount = 0;

		static constexpr uint32_t s_FrameTimePlotValuesCount = 100;
		float m_FrameTimeHistory[s_FrameTimePlotValuesCount] = {};
		uint32_t m_FrameTimeValuesOffset = 0;

		float m_RefreshStatsTimer = 0.0f;

		friend class Application;
		friend class EditorOverlay;
	};

}