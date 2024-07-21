/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "LoggerStdio"
#include <imagine/fs/FS.hh>
#include <imagine/util/string/StaticString.hh>
#include <imagine/logger/logger.h>
#include <cstdio>
#include <cstring>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifdef __APPLE__
#include <asl.h>
#include <unistd.h>
#endif

using namespace IG;

static const bool bufferLogLineOutput = Config::envIsAndroid || Config::envIsIOS;
static char logLineBuffer[512]{};
uint8_t loggerVerbosity = loggerMaxVerbosity;
static FILE *logExternalFile{};
static bool logEnabled = Config::DEBUG_BUILD; // default logging off in release builds

static FS::PathString externalLogEnablePath(const char *dirStr)
{
	return FS::pathString(dirStr, "imagine_enable_log_file");
}

static FS::PathString externalLogPath(const char *dirStr)
{
	return FS::pathString(dirStr, "imagine_log.txt");
}

static bool shouldLogToExternalFile(const char *dirStr)
{
	return FS::exists(externalLogEnablePath(dirStr));
}

void logger_setLogDirectoryPrefix(const char *dirStr)
{
	if(!logEnabled)
		return;
	#if defined __APPLE__ && (defined __i386__ || defined __x86_64__)
	asl_add_log_file(nullptr, STDERR_FILENO); // output to stderr
	#endif
	if(shouldLogToExternalFile(dirStr) && !logExternalFile)
	{
		auto path = externalLogPath(dirStr);
		logMsg("external log file: %s", path.data());
		logExternalFile = fopen(path.data(), "wb");
	}
}

void logger_setEnabled(bool enable)
{
	logEnabled = enable;
}

bool logger_isEnabled()
{
	return logEnabled;
}

static void printToLogLineBuffer(const char* msg, va_list args)
{
	vsnprintf(logLineBuffer + strlen(logLineBuffer), sizeof(logLineBuffer) - strlen(logLineBuffer), msg, args);
}

constexpr int severityToLogLevel([[maybe_unused]] LoggerSeverity severity)
{
	#ifdef __ANDROID__
	switch(severity)
	{
		case LOGGER_DEBUG_MESSAGE: return ANDROID_LOG_DEBUG;
		default: [[fallthrough]];
		case LOGGER_MESSAGE: return ANDROID_LOG_INFO;
		case LOGGER_WARNING: return ANDROID_LOG_WARN;
		case LOGGER_ERROR: return ANDROID_LOG_ERROR;
	}
	#elif defined __APPLE__
	switch(severity)
	{
		case LOGGER_DEBUG_MESSAGE: return ASL_LEVEL_DEBUG;
		default: [[fallthrough]];
		case LOGGER_MESSAGE: return ASL_LEVEL_NOTICE;
		case LOGGER_WARNING: return ASL_LEVEL_WARNING;
		case LOGGER_ERROR: return ASL_LEVEL_ERR;
	}
	#else
	return 0;
	#endif
}

void logger_vprintf(LoggerSeverity severity, const char* msg, va_list args)
{
	if(!logEnabled)
		return;
	if(severity > loggerVerbosity) return;

	if(logExternalFile)
	{
		va_list args2;
		va_copy(args2, args);
		vfprintf(logExternalFile, msg, args2);
		va_end(args2);
		fflush(logExternalFile);
	}

	if(bufferLogLineOutput && !strchr(msg, '\n'))
	{
		printToLogLineBuffer(msg, args);
		return;
	}

	#ifdef __ANDROID__
	if(strlen(logLineBuffer))
	{
		printToLogLineBuffer(msg, args);
		__android_log_write(severityToLogLevel(severity), "imagine", logLineBuffer);
		logLineBuffer[0] = 0;
	}
	else
		__android_log_vprint(severityToLogLevel(severity), "imagine", msg, args);
	#elif defined __APPLE__
	if(strlen(logLineBuffer))
	{
		printToLogLineBuffer(msg, args);
		asl_log(nullptr, nullptr, severityToLogLevel(severity), "%s", logLineBuffer);
		logLineBuffer[0] = 0;
	}
	else
		asl_vlog(nullptr, nullptr, severityToLogLevel(severity), msg, args);
	#else
	fprintf(stderr, "%s", IG::Log::severityToColorCode(severity));
	vfprintf(stderr, msg, args);
	#endif
}

void logger_printf(LoggerSeverity severity, const char* msg, ...)
{
	if(!logEnabled)
		return;
	va_list args;
	va_start(args, msg);
	logger_vprintf(severity, msg, args);
	va_end(args);
}

namespace IG::Log
{

void printMsg([[maybe_unused]] LoggerSeverity lv, const char* str, size_t strSize)
{
	const char newLine = '\n';
	if(logExternalFile)
	{
		fwrite(str, 1, strSize, logExternalFile);
		fwrite(&newLine, 1, 1, logExternalFile);
		fflush(logExternalFile);
	}
	#ifdef __ANDROID__
	__android_log_write(severityToLogLevel(lv), "imagine", str);
	#elif defined __APPLE__
	asl_log(nullptr, nullptr, severityToLogLevel(lv), "%s", str);
	#else
	fwrite(str, 1, strSize, stderr);
	fwrite(&newLine, 1, 1, stderr);
	#endif
}

void print(LoggerSeverity lv, std::string_view tag, std::string_view format, std::format_args args)
{
	if(!logEnabled || lv > loggerVerbosity)
		return;
	StaticString<4096> str;
	Log::beginMsg(str, lv, tag, format, args);
	printMsg(lv, str.c_str(), str.size());
}

}
