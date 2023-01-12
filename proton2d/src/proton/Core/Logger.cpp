#include "pch.h"
#include "Logger.h"

namespace proton {

	HANDLE Logger::m_ConsoleHandle;

	void Logger::DisplayLog(int logLevel, const std::string& msg)
	{
		switch (logLevel) {
		case 1:
			SetConsoleTextAttribute(Logger::m_ConsoleHandle, _INFO_COLOR);
			std::cout << "[INFO] ";
			break;
		case 2:
			SetConsoleTextAttribute(Logger::m_ConsoleHandle, _WARN_COLOR);
			std::cout << "[WARN] ";
			break;
		case 3:
			SetConsoleTextAttribute(Logger::m_ConsoleHandle, _ERROR_COLOR);
			std::cout << "[ERROR] ";
			break;
		case 4:
			SetConsoleTextAttribute(Logger::m_ConsoleHandle, _OK_COLOR);
			std::cout << "[OK] ";
			break;
		default:
			SetConsoleTextAttribute(Logger::m_ConsoleHandle, _LOG_COLOR);
			std::cout << "[LOG] ";
			break;
		}

		SetConsoleTextAttribute(Logger::m_ConsoleHandle, 7);
		std::cout << msg << std::endl;
	}

	void Logger::Init()
	{
		Logger::m_ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	std::string Logger::ToString()
	{
		return std::string();
	}

}
