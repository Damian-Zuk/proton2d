#pragma once
#include "Proton/Network/Common.h"

namespace proton {

	class Scene;
	class Server;

	class ReplicationManager
	{
	public:
		ReplicationManager(Server* server);

		void StreamWriteReplicationData(BufferStreamWriter& stream, Scene* scene, bool verifyComponentChecksum);

	private:
		Server* m_Server;
	};

}
