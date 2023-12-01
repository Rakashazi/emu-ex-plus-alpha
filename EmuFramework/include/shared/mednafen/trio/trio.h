#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#define trio_snprintf snprintf
#define trio_fprintf fprintf
#define trio_sscanf sscanf
#define trio_fscanf fscanf
#define trio_vasprintf vasprintf
#define trio_vprintf vprintf
#define trio_sprintf sprintf

inline char *trio_vaprintf(const char *format, va_list args)
{
	char *msg{};
	vasprintf(&msg, format, args);
	return msg;
}

inline int trio_vcprintf(auto &&, std::string *strPtr, const char *format, va_list args)
{
	char *msg{};
	int bytes = vasprintf(&msg, format, args);
	*strPtr = msg;
	free(msg);
	return bytes;
}
