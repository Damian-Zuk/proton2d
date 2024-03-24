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

	class NetworkManager;

	class Client
	{
	public:
		enum class ConnectionStatus
		{
			Disconnected = 0, Connected, Connecting, FailedToConnect
		};
	public:
		Client(NetworkManager* manager);
		~Client();

		void ConnectToServer(const std::string& serverAddress);
		void Disconnect();

		void SendBuffer(Buffer buffer, bool reliable = true);
		void SendString(const std::string& string, bool reliable = true);

		template<typename T>
		void SendData(const T& data, bool reliable = true)
		{
			SendBuffer(Buffer(&data, sizeof(T)), reliable);
		}

		bool IsRunning() const { return m_Running; }
		ConnectionStatus GetConnectionStatus() const { return m_ConnectionStatus; }
		const std::string& GetConnectionDebugMessage() const { return m_ConnectionDebugMessage; }
	private:
		void NetworkThreadFunc();
		void Shutdown();
	private:
		static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info);
		void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

		void PollIncomingMessages();
		void PollConnectionStateChanges();

		void OnFatalError(const std::string& message);
	private:
		NetworkManager* m_NetworkManager;

		std::thread m_NetworkThread;

		ConnectionStatus m_ConnectionStatus = ConnectionStatus::Disconnected;
		std::string m_ConnectionDebugMessage;

		std::string m_ServerAddress;
		bool m_Running = false;

		std::atomic<bool> m_NetworkThreadFinished = false;

		ISteamNetworkingSockets* m_Interface = nullptr;
		HSteamNetConnection m_Connection = 0;

		friend class NetworkManager;
	};

}
