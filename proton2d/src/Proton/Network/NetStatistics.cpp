#include "ptpch.h"
#include "Proton/Network/NetStatistics.h"
#include "Proton/Network/Server.h"
#include "Proton/Network/NetworkManager.h"

#include "Proton/Core/GameInstance.h"
#include "Proton/Core/Timer.h"

namespace proton {

	constexpr static size_t s_DetailedStatusBufferSize = 2048;

	NetStatistics::NetStatistics(Server* server)
		: m_Server(server)
	{
		GenerateStatsLogFilename();
	}

	NetStatistics::~NetStatistics()
	{
		for (auto& it : m_NetworkStats)
			it.second.Detailed.Release();
	}

	void NetStatistics::AllocateNetworkStatsBuffer(ClientID clientID)
	{
		Buffer detailedConnStatusBuffer;
		detailedConnStatusBuffer.Allocate(s_DetailedStatusBufferSize);
		m_NetworkStats[clientID].Detailed = detailedConnStatusBuffer;

		NetworkManager* networkManager = m_Server->m_NetworkManager;

		if (networkManager->m_SaveStatsForClientID == 0)
		{
			networkManager->m_SaveStatsForClientID = clientID;
		}
	}

	void NetStatistics::ReleaseNetworkStatsBuffer(ClientID clientID)
	{
		auto statsIt = m_NetworkStats.find(clientID);
		if (statsIt != m_NetworkStats.end())
		{
			statsIt->second.Detailed.Release();
			m_NetworkStats.erase(statsIt);
		}

		NetworkManager* networkManager = m_Server->m_NetworkManager;
		if (networkManager->m_SaveStatsForClientID == clientID)
		{
			networkManager->m_SaveStatsForClientID =
				m_Server->m_ConnectedClients.size() ? m_Server->m_ConnectedClients.begin()->first : 0;
		}
	}

	void NetStatistics::UpdateNetworkStatistics()
	{
		static Timer timer;
		if (timer.Elapsed() >= m_StatsUpdateInterval)
		{
			for (auto& [hConn, stats] : m_NetworkStats)
			{
				m_Server->m_Interface->GetConnectionRealTimeStatus(hConn, &stats.RealTime, 0, NULL);
				m_Server->m_Interface->GetDetailedConnectionStatus(hConn, (char*)stats.Detailed.Data, s_DetailedStatusBufferSize);

				NetworkManager* manager = m_Server->m_NetworkManager;
				if (manager->m_SaveNetworkStatsToLogFile && (hConn == manager->m_SaveStatsForClientID || manager->m_SaveStatsForAllClients))
				{
					LogStatsToFile(hConn, stats);
				}
			}
			timer.Reset();
		}
	}

	static inline float round(float f)
	{
		return (float)(std::round((double)f * 100000.0) / 100000.0);
	}

	void NetStatistics::LogStatsToFile(ClientID clientID, const NetworkStats& stats)
	{
		const SteamNetConnectionRealTimeStatus_t& status = stats.RealTime;
		int in = (int)status.m_flInBytesPerSec;
		int out = (int)status.m_flOutBytesPerSec;
		int ping = status.m_nPing;
		int replicated = m_ReplicationStats.RepEntitiesCount / m_ReplicationStats.RepPacketCount;
		m_ReplicationStats = { 0 , 0 };

		if (in == 0 && out == 0)
			return;

		static Timer timer;

		std::ofstream logFile("logs/" + m_StatsLogsFilename, std::ios_base::app);
		if (!m_StatsLogsHeaderWritten)
		{
			// Write header to log file
			logFile << "# scene=" << m_Server->m_GameInstance->GetActiveScene()->GetFilepath() << "\r\n";
			logFile << "# tick_rate=" << m_Server->m_NetworkManager->m_Tickrate << "\r\n";
			logFile << "# client_id; time_point; in_bps; out_bps; ping_ms; rep_count\r\n";
			m_StatsLogsHeaderWritten = true;
			timer.Reset();
		}

		logFile << clientID << "; "
			<< round(timer.Elapsed()) << "; "
			<< in << "; "
			<< out << "; "
			<< ping << "; "
			<< replicated << "\r\n";

		logFile.close();
	}

	void NetStatistics::GenerateStatsLogFilename()
	{
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);

		std::tm bt{};
		localtime_s(&bt, &in_time_t);

		std::ostringstream oss;
		oss << "NetworkStats_" << std::put_time(&bt, "%Y%m%d_%H%M%S") << ".log";
		m_StatsLogsFilename = oss.str();
	}

}
