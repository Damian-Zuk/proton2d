#pragma once

namespace proton {

	class Timer
	{
	public:
		void Tick(float timeStep)
		{
			m_ElapsedTime += timeStep;
		}

		bool OnInterval(float seconds)
		{
			if (m_ElapsedTime >= seconds || m_ElapsedTime < 0.0f)
			{
				m_ElapsedTime = 0.0f;
				return true;
			}

			return false;
		}

	private:
		float m_ElapsedTime = -1.0f;
	};

}