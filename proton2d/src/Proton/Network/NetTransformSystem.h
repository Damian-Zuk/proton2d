#pragma once
#include "Proton/Network/Common.h"
#include "Proton/Network/Messages.h"

#include "Proton/Scene/Entity.h"

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

		void Client_OnUpdate(Scene* scene, float ts);

		void Client_SendSequenceNumberMessage();

		void Server_OnSequenceNumberMessage(ClientID clientID, NetworkStreamReader& stream);

	private:
		Client* m_Client = nullptr;
		Server* m_Server = nullptr;
		NetworkManager* m_NetworkManager;

		// Prediction fixed update time for non physics entities
		float m_FixedUpdateTimestep = 0.01f;
		float m_FixedUpdateTimer = m_FixedUpdateTimestep;

		std::vector<MessageEntitySequence::PayloadItem> m_SequenceNumbersToSend;
	};

}
