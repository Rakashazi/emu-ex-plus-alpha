#pragma once

#include <imagine/config/build.h>
#include <imagine/util/builtins.h>
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
