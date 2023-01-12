#pragma once
#include "pch.h"
#include "proton/Core/Core.h"

#define _LOG_COLOR 7
#define _INFO_COLOR 9
#define _WARN_COLOR 14
#define _ERROR_COLOR 12
#define _OK_COLOR 10

namespace proton {

	class Logger
	{
	public:
		template<typename ... TArgs>
		static void Log(int logLevel, TArgs ... args) {
			std::string msg = ToString(std::forward<TArgs>(args)...);
			DisplayLog(logLevel, msg);
		}
		static void DisplayLog(int logLevel, const std::string& msg);
		static void Init();
	private:
		static std::string ToString();
		template<typename P1, typename ... Param>
		static std::string ToString(const P1& p1, const Param& ... param) {
			std::stringstream ss; ss << p1;
			return ss.str() + ' ' + ToString(param...);
		}

	private:
		static HANDLE m_ConsoleHandle;
	};

}

#define LOG(...)		::proton::Logger::Log(0, __VA_ARGS__);
#define LOG_INFO(...)	::proton::Logger::Log(1, __VA_ARGS__);
#define LOG_WARN(...)	::proton::Logger::Log(2, __VA_ARGS__);
#define LOG_ERROR(...)	::proton::Logger::Log(3, __VA_ARGS__);
#define LOG_OK(...)		::proton::Logger::Log(4, __VA_ARGS__);