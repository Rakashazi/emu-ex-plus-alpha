#pragma once

#include <imagine/util/utility.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

enum { LOGGER_ERROR, LOGGER_WARNING, LOGGER_MESSAGE, LOGGER_DEBUG_MESSAGE };

static const uint8_t loggerMaxVerbosity = LOGGER_DEBUG_MESSAGE;
extern uint8_t loggerVerbosity;

typedef uint8_t LoggerSeverity;

CLINK void logger_setLogDirectoryPrefix(const char *dirStr) __attribute__((cold));
CLINK void logger_setEnabled(bool enable);
CLINK bool logger_isEnabled();
CLINK void logger_printf(LoggerSeverity severity, const char* msg, ...) __attribute__((format (printf, 2, 3)));
CLINK void logger_vprintf(LoggerSeverity severity, const char* msg, va_list arg);


#define logger_printfn(severity, msg, ...) logger_printf(severity, msg "\n", ## __VA_ARGS__)

// some shortcuts
static const uint8_t LOG_M = LOGGER_MESSAGE;
static const uint8_t LOG_D = LOGGER_DEBUG_MESSAGE;
static const uint8_t LOG_W = LOGGER_WARNING;
static const uint8_t LOG_E = LOGGER_ERROR;

#ifndef LOGTAG
#define LOGTAG
#endif

#define logger_modulePrintf(severity, msg, ...) logger_printf(severity, LOGTAG ": " msg, ## __VA_ARGS__)
#define logger_modulePrintfn(severity, msg, ...) logger_printfn(severity, LOGTAG ": " msg, ## __VA_ARGS__)

#define logMsg(msg, ...) logger_modulePrintfn(LOG_M, msg, ## __VA_ARGS__)
#define logDMsg(msg, ...) logger_modulePrintfn(LOG_D, msg, ## __VA_ARGS__)
#define logWarn(msg, ...) logger_modulePrintfn(LOG_W, msg, ## __VA_ARGS__)
#define logErr(msg, ...) logger_modulePrintfn(LOG_E, msg, ## __VA_ARGS__)

#define logMsgNoBreak(msg, ...) logger_modulePrintf(LOG_M, msg, ## __VA_ARGS__)
#define logDMsgNoBreak(msg, ...) logger_modulePrintf(LOG_D, msg, ## __VA_ARGS__)
#define logWarnNoBreak(msg, ...) logger_modulePrintf(LOG_W, msg, ## __VA_ARGS__)
#define logErrNoBreak(msg, ...) logger_modulePrintf(LOG_E, msg, ## __VA_ARGS__)

#ifdef __cplusplus

#include <imagine/config/defs.hh>
#include <format>

namespace IG::Log
{

void print(LoggerSeverity, std::string_view tag, std::string_view format, std::format_args);
void printMsg(LoggerSeverity, const char* str, size_t strSize);

constexpr auto severityToColorCode(LoggerSeverity severity)
{
	switch(severity)
	{
		case LOGGER_DEBUG_MESSAGE: return "\033[1;36m";
		default: [[fallthrough]];
		case LOGGER_MESSAGE: return "\033[0m";
		case LOGGER_WARNING: return "\033[1;33m";
		case LOGGER_ERROR: return "\033[1;31m";
	}
}

inline void beginMsg(auto& str, LoggerSeverity lv, std::string_view tag, std::string_view format, std::format_args args)
{
	if(Config::envIsLinux)
	{
		str += severityToColorCode(lv);
	}
	if(tag.size())
	{
		str += tag;
		str += ": ";
	}
	std::vformat_to(std::back_inserter(str), format, args);
}

}

namespace IG
{

class SystemLogger
{
public:
	std::string_view tag;

	template <class... T>
	void print(LoggerSeverity lv, std::format_string<T...> format, T&&... args) const
	{
		Log::print(lv, tag, format.get(), std::make_format_args(args...));
	}

	template <class... T>
	void info(std::format_string<T...> format, T&&... args) const
	{
		Log::print(LOG_M, tag, format.get(), std::make_format_args(args...));
	}

	template <class... T>
	void debug(std::format_string<T...> format, T&&... args) const
	{
		Log::print(LOG_D, tag, format.get(), std::make_format_args(args...));
	}

	template <class... T>
	void warn(std::format_string<T...> format, T&&... args) const
	{
		Log::print(LOG_W, tag, format.get(), std::make_format_args(args...));
	}

	template <class... T>
	void error(std::format_string<T...> format, T&&... args) const
	{
		Log::print(LOG_E, tag, format.get(), std::make_format_args(args...));
	}

	template <class... T>
	void beginMsg(auto& str, LoggerSeverity lv, std::format_string<T...> format, T&&... args) const
	{
		Log::beginMsg(str, lv, tag, format.get(), std::make_format_args(args...));
	}

	template <class... T>
	void beginInfo(auto& str, std::format_string<T...> format, T&&... args) const
	{
		Log::beginMsg(str, LOG_M, tag, format.get(), std::make_format_args(args...));
	}

	template <class... T>
	void beginDebug(auto& str, std::format_string<T...> format, T&&... args) const
	{
		Log::beginMsg(str, LOG_D, tag, format.get(), std::make_format_args(args...));
	}

	template <class... T>
	void beginWarn(auto& str, std::format_string<T...> format, T&&... args) const
	{
		Log::beginMsg(str, LOG_W, tag, format.get(), std::make_format_args(args...));
	}

	template <class... T>
	void beginError(auto& str, std::format_string<T...> format, T&&... args) const
	{
		Log::beginMsg(str, LOG_E, tag, format.get(), std::make_format_args(args...));
	}

	void printMsg(LoggerSeverity lv, auto& str) const
	{
		Log::printMsg(lv, str.c_str(), str.size());
	}

	void printInfo(auto& str) const { printMsg(LOG_M, str); }
	void printDebug(auto& str) const { printMsg(LOG_D, str); }
	void printWarn(auto& str) const { printMsg(LOG_W, str); }
	void printError(auto& str) const { printMsg(LOG_E, str); }
};

}

#endif
