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

	struct NetworkStats
	{
		SteamNetConnectionRealTimeStatus_t RealTime;
		Buffer Detailed;
	};

	struct ReplicationStats
	{
		uint32_t RepEntitiesCount = 0;
		uint32_t RepPacketCount = 0;
	};

	class NetStatsManager
	{
	public:
		NetStatsManager(Server* server);
		virtual ~NetStatsManager();

		void AllocateNetworkStatsBuffer(ClientID clientID);
		void ReleaseNetworkStatsBuffer(ClientID clientID);
		void UpdateNetworkStatistics();
		void LogStatsToFile(ClientID clientID, SteamNetConnectionRealTimeStatus_t& status);
		void GenerateStatsLogFilename();

		const std::map<HSteamNetConnection, NetworkStats>& GetNetworkStats() const { return m_NetworkStats; }

	private:
		Server* m_Server;

		const float m_StatsUpdateInterval = 0.2f;
		std::map<HSteamNetConnection, NetworkStats> m_NetworkStats;
		ReplicationStats m_ReplicationStats;
		std::string m_StatsLogsFilename;
		bool m_StatsLogsHeaderWritten = false;

		friend class Server;
		friend class ReplicationManager;

		friend class InfoPanel;
	};

}
