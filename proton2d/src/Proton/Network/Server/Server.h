#pragma once

#include "Proton/Core/Buffer.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

#include <string>
#include <map>
#include <thread>
#include <functional>

namespace proton {

	using ClientID = HSteamNetConnection;

	struct ClientInfo
	{
		ClientID ID;
		std::string ConnectionDesc;
	};

	class NetworkManager;

	class Server
	{
	public:
	public:
		Server(NetworkManager* manager);
		~Server();

		void OnUpdate(float ts);

		void Start(int port);
		void Stop();

		void SendBufferToClient(ClientID clientID, Buffer buffer, bool reliable = true);
		void SendBufferToAllClients(Buffer buffer, ClientID excludeClientID = 0, bool reliable = true);

		void SendStringToClient(ClientID clientID, const std::string& string, bool reliable = true);
		void SendStringToAllClients(const std::string& string, ClientID excludeClientID = 0, bool reliable = true);

		template<typename T>
		void SendDataToClient(ClientID clientID, const T& data, bool reliable = true)
		{
			SendBufferToClient(clientID, Buffer(&data, sizeof(T)), reliable);
		}

		template<typename T>
		void SendDataToAllClients(const T& data, ClientID excludeClientID = 0, bool reliable = true)
		{
			SendBufferToAllClients(Buffer(&data, sizeof(T)), excludeClientID, reliable);
		}

		void KickClient(ClientID clientID);

		bool IsRunning() const { return m_Running; }
		const std::map<HSteamNetConnection, ClientInfo>& GetConnectedClients() const { return m_ConnectedClients; }
	private:
		void NetworkThreadFunc(); // Server thread

		static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info);
		void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

		// Server functionality
		void PollIncomingMessages();
		void SetClientNick(HSteamNetConnection hConn, const char* nick);
		void PollConnectionStateChanges();

		void OnFatalError(const std::string& message);
	private:
		NetworkManager* m_NetworkManager;

		std::thread m_NetworkThread;

		int m_Port = 0;
		bool m_Running = false;
		std::map<HSteamNetConnection, ClientInfo> m_ConnectedClients;

		std::atomic<bool> m_NetworkThreadFinished = false;

		ISteamNetworkingSockets* m_Interface = nullptr;
		HSteamListenSocket m_ListenSocket = 0u;
		HSteamNetPollGroup m_PollGroup = 0u;

		friend class NetworkManager;
	};

}
