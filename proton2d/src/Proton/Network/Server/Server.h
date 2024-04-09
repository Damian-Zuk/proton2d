#pragma once

#include "Proton/Network/Common/Common.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

#include <thread>
#include <queue>

namespace proton {

	using ClientID = HSteamNetConnection;

	struct ClientInfo
	{
		ClientID ID;
		std::string ConnectionDesc;
	};

	class NetworkManager;
	class GameInstance;

	class Server
	{
	public:
		Server(GameInstance* gameInstance);
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

		void SetOnRecvPlayerActionCallback(uint32_t clientID, OnRecvPlayerActionCallback function);

	private:
		void NetworkThreadFunc(); // Server thread

		static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info);
		void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

		// Server functionality
		void PollIncomingMessages();
		void SetClientNick(HSteamNetConnection hConn, const char* nick);
		void PollConnectionStateChanges();

		void OnClientConnected(const ClientInfo& clientInfo);
		void OnClientDisconnected(const ClientInfo& clientInfo);

		void OnDataReceived(const ClientInfo& clientInfo, const Buffer& buffer);

		void OnFatalError(const std::string& message);

		void OnTick(); // Called from main thread
	private:
		GameInstance* m_GameInstance;
		NetworkManager* m_NetworkManager;

		std::thread m_NetworkThread;
		Buffer m_ScratchBuffer;

		int m_Port = 0;
		bool m_Running = false;
		std::map<HSteamNetConnection, ClientInfo> m_ConnectedClients;

		std::queue<ISteamNetworkingMessage*> m_MessageQueue;
		std::mutex m_QueueMutex;

		std::unordered_map<uint32_t, OnRecvPlayerActionCallback> m_OnRecvPlayerActionCallbacks;

		std::atomic<bool> m_NetworkThreadFinished = false;

		ISteamNetworkingSockets* m_Interface = nullptr;
		HSteamListenSocket m_ListenSocket = 0u;
		HSteamNetPollGroup m_PollGroup = 0u;

		friend class NetworkManager;
	};

}
