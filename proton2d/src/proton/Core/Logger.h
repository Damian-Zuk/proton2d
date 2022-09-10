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
		static void log(int level, TArgs ... args) {
			std::string msg = to_string(std::forward<TArgs>(args)...);
			display_log(level, msg);
		}
		static void display_log(int log_type, std::string msg);
		static void init();
	private:
		static std::string to_string();
		template<typename P1, typename ... Param>
		static std::string to_string(const P1& p1, const Param& ... param) {
			std::stringstream ss; ss << p1;
			return ss.str() + ' ' + to_string(param...);
		}

	private:
		static HANDLE m_ConsoleHandle;
	};

}

#define LOG(...)		::proton::Logger::log(0, __VA_ARGS__);
#define LOG_INFO(...)	::proton::Logger::log(1, __VA_ARGS__);
#define LOG_WARN(...)	::proton::Logger::log(2, __VA_ARGS__);
#define LOG_ERROR(...)	::proton::Logger::log(3, __VA_ARGS__);
#define LOG_OK(...)		::proton::Logger::log(4, __VA_ARGS__);