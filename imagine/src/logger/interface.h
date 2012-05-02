#pragma once

#ifdef IMAGINE_SRC
	#include <config.h>
#endif
#include <config/imagineTypes.h>
#include <stdarg.h>
#include <util/ansiTypes.h>
#include <util/builtins.h>

enum { LOGGER_ERROR, LOGGER_WARNING, LOGGER_MESSAGE, LOGGER_DEBUG_MESSAGE };

static const uint loggerMaxVerbosity = LOGGER_DEBUG_MESSAGE;
extern uint loggerVerbosity;

typedef uint LoggerSeverity;

#ifdef USE_LOGGER

CallResult logger_init() ATTRS(cold);

#ifdef CONFIG_LOGGER_STDIO
	static void logger_update() { }
#else
	void logger_update();
#endif

CLINK void logger_printfn(LoggerSeverity severity, const char* msg, ...) __attribute__ ((format (printf, 2, 3)));
CLINK void logger_vprintfn(LoggerSeverity severity, const char* msg, va_list arg);

#else

static CallResult logger_init() { return(OK); }
static void logger_update() { }
//static void logger_printfn(LoggerSeverity severity, const char* msg, ...) { }
//static void logger_vprintfn(LoggerSeverity severity, const char* msg, va_list arg) { }
static void logger_dummy() { }
#define logger_printfn(severity, msg, ...) logger_dummy()
#define logger_vprintfn(severity, msg, arg) logger_dummy()

#endif // USE_LOGGER

#define logger_printf(severity, msg, ...) logger_printfn(severity, msg "\n", ## __VA_ARGS__)

// some shortcuts
static const uint LOG_M = LOGGER_MESSAGE;
static const uint LOG_D = LOGGER_DEBUG_MESSAGE;
static const uint LOG_W = LOGGER_WARNING;
static const uint LOG_E = LOGGER_ERROR;

#ifndef thisModuleName
	#define thisModuleName
#endif

#define logger_modulePrintf(severity, msg, ...) logger_printf(severity, thisModuleName ": " msg, ## __VA_ARGS__)
#define logger_modulePrintfn(severity, msg, ...) logger_printfn(severity, thisModuleName ": " msg, ## __VA_ARGS__)

#define logMsg(msg, ...) logger_modulePrintf(LOG_M, msg, ## __VA_ARGS__)
#define logDMsg(msg, ...) logger_modulePrintf(LOG_D, msg, ## __VA_ARGS__)
#define logWarn(msg, ...) logger_modulePrintf(LOG_W, msg, ## __VA_ARGS__)
#define logErr(msg, ...) logger_modulePrintf(LOG_E, msg, ## __VA_ARGS__)

#define logMsgn(msg, ...) logger_modulePrintfn(LOG_M, msg, ## __VA_ARGS__)
#define logDMsgn(msg, ...) logger_modulePrintfn(LOG_D, msg, ## __VA_ARGS__)
#define logWarnn(msg, ...) logger_modulePrintfn(LOG_W, msg, ## __VA_ARGS__)
#define logErrn(msg, ...) logger_modulePrintfn(LOG_E, msg, ## __VA_ARGS__)
