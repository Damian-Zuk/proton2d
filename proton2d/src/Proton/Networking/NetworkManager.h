#pragma once

namespace proton {

	enum class NetMode : uint8_t
	{
		Standalone = 0,
		ListenServer = 1,
		Client = 3,
	};

	class NetworkManager
	{
	public:
		NetworkManager();
		virtual ~NetworkManager();

		void SetNetMode(NetMode mode);
		NetMode GetNetMode() const { return m_NetMode; }

	private:
		NetMode m_NetMode = NetMode::Standalone;
	};

}
