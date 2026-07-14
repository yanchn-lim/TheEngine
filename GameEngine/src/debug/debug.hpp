#pragma once

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

#include "core/ring_buffer.hpp"

constexpr int MAX_LOG_ENTRIES = 1024;

enum class LogLevel : uint8_t
{
	VERBOSE,
	INFO,
	WARNING,
	ERROR
};

struct LogEntry
{
	LogLevel level;
	std::string message;
};

class DebugConsole
{
public:
	static DebugConsole& Get()
	{
		static DebugConsole instance;
		return instance;
	}

	DebugConsole(const DebugConsole&) = delete;
	DebugConsole& operator=(const DebugConsole&) = delete;

	void PushLog(LogLevel level, std::string message);
	void Draw();
	void Clear();
	bool& IsOpen();

private:
	DebugConsole() = default;
	RingBuffer<LogEntry, MAX_LOG_ENTRIES> _entries{};
	bool _open{ true };
	bool _scrollToBottom{ false };
};

namespace Debug
{
	template<typename... Args>
	void CLog(Args&&... args)
	{
		(std::cout << ... << args);
	}

	template<typename... Args>
	void Log(Args&&... args)
	{
		std::ostringstream oss;
		(oss << ... << args);
		DebugConsole::Get().PushLog(LogLevel::INFO, oss.str());
	}

	template<typename... Args>
	void LogWarning(Args&&... args)
	{
		std::ostringstream oss;
		(oss << ... << args);
		DebugConsole::Get().PushLog(LogLevel::WARNING, oss.str());
	}

	template<typename... Args>
	void LogError(Args&&... args)
	{
		std::ostringstream oss;
		(oss << ... << args);
		DebugConsole::Get().PushLog(LogLevel::ERROR, oss.str());
	}

	template<typename... Args>
	void LogVerbose(Args&&... args)
	{
		std::ostringstream oss;
		(oss << ... << args);
		DebugConsole::Get().PushLog(LogLevel::VERBOSE, oss.str());
	}

}
