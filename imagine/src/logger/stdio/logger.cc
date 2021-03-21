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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/fs/FS.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/string.h>
#include <cstdio>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifdef __APPLE__
#include <asl.h>
#include <unistd.h>
#endif

static const bool bufferLogLineOutput = Config::envIsAndroid || Config::envIsIOS;
static char logLineBuffer[512]{};
uint8_t loggerVerbosity = loggerMaxVerbosity;
static FILE *logExternalFile{};
static bool logEnabled = Config::DEBUG_BUILD; // default logging off in release builds

static FS::PathString externalLogDir(Base::ApplicationContext app)
{
	FS::PathString prefix{"."};
	if(Config::envIsIOS)
		prefix = FS::makePathString("/var/mobile");
	else if(Config::envIsAndroid)
		prefix = app.sharedStoragePath();
	return prefix;
}

static FS::PathString externalLogEnablePath(Base::ApplicationContext app)
{
	return FS::makePathStringPrintf("%s/imagine_enable_log_file", externalLogDir(app).data());
}

static FS::PathString externalLogPath(Base::ApplicationContext app)
{
	return FS::makePathStringPrintf("%s/imagine_log.txt", externalLogDir(app).data());
}

static bool shouldLogToExternalFile(Base::ApplicationContext app)
{
	return FS::exists(externalLogEnablePath(app));
}

void logger_init(Base::ApplicationContext app)
{
	if(!logEnabled)
		return;
	#if defined __APPLE__ && (defined __i386__ || defined __x86_64__)
	asl_add_log_file(nullptr, STDERR_FILENO); // output to stderr
	#endif
	if(shouldLogToExternalFile(app) && !logExternalFile)
	{
		auto path = externalLogPath(app);
		logMsg("external log file: %s", path.data());
		logExternalFile = fopen(path.data(), "wb");
	}
	//logMsg("init logger");
}

void logger_setEnabled(Base::ApplicationContext app, bool enable)
{
	logEnabled = enable;
	if(enable)
	{
		logger_init(app);
	}
}

bool logger_isEnabled()
{
	return logEnabled;
}

static void printToLogLineBuffer(const char* msg, va_list args)
{
	vsnprintf(logLineBuffer + strlen(logLineBuffer), sizeof(logLineBuffer) - strlen(logLineBuffer), msg, args);
}

static int severityToLogLevel(LoggerSeverity severity)
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

static const char *severityToColorCode(LoggerSeverity severity)
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
	fprintf(stderr, "%s", severityToColorCode(severity));
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
