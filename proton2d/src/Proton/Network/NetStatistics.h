#pragma once

#include "Proton/Core/Buffer.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

namespace proton {

	// Forward declaration
	class Server;
	class NetworkManager;

	using ClientID = HSteamNetConnection;

	class NetStatistics
	{
	public:
		struct NetworkStats
		{
			SteamNetConnectionRealTimeStatus_t RealTime;
			Buffer Detailed;
		};

		struct ReplicationStats
		{
			uint32_t RepEntitiesCount = 0;
			uint32_t RepMessageCount = 0;
		};

		NetStatistics(Server* server);
		virtual ~NetStatistics();

		void AllocateNetworkStatsBuffer(ClientID clientID);
		void ReleaseNetworkStatsBuffer(ClientID clientID);
		void UpdateNetworkStatistics();
		void LogStatsToFile(ClientID clientID, const NetworkStats& stats);
		void GenerateStatsLogFilename();

		const std::map<HSteamNetConnection, NetworkStats>& GetNetworkStats() const { return m_NetworkStats; }

	private:
		Server* m_Server;

		const float m_StatsUpdateInterval = 0.2f;
		std::map<ClientID, NetworkStats> m_NetworkStats;
		ReplicationStats m_ReplicationStats;
		std::string m_StatsLogsFilename;
		bool m_StatsLogsHeaderWritten = false;

		friend class Server;
		friend class NetReplicator;

		friend class InfoPanel;
	};

}
