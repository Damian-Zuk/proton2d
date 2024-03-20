#include "ptpch.h"
#include "Proton/Network/Common/NetworkManager.h"

namespace proton {

	NetworkManager::NetworkManager()
	{
	}

	NetworkManager::~NetworkManager()
	{
	}

	void NetworkManager::SetNetMode(NetMode mode)
	{
		m_NetMode = mode;
	}

}
