#pragma once
#include "Proton/Network/Common.h"
#include "Proton/Network/Messages.h"

#include <queue>

namespace proton {

	class Scene;
	class Client;
	class Server;
	class NetworkManager;

	class NetTransformSystem
	{
	public:
		NetTransformSystem(Client* client);
		NetTransformSystem(Server* server);
		NetTransformSystem() = delete;

		void Client_SendSequenceNumberMessage();

		void Server_OnSequenceNumberMessage(ClientID clientID, NetworkStreamReader& stream);

		void OnUpdate(Scene* scene, float ts);

	private:
		Client* m_Client = nullptr;
		Server* m_Server = nullptr;
		NetworkManager* m_NetworkManager;

		std::vector<NetMessageTransformSequenceNumber::PayloadItem> m_SequenceNumbersToSend;
	};

}
