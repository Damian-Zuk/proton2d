#include "ptpch.h"
#include "Proton/Networking/NetworkManager.h"

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
