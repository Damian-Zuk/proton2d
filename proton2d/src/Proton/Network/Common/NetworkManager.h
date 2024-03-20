#pragma once
#include "Proton/Network/Common/Network.h"

namespace proton {

	class NetworkManager
	{
	public:
		NetworkManager();
		virtual ~NetworkManager();

		void SetNetMode(NetMode mode);
		NetMode GetNetMode() const { return m_NetMode; }

	private:
		NetMode m_NetMode = NetMode::Standalone;
	};

}
