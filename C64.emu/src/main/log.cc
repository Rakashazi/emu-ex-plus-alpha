/*  This file is part of C64.emu.

	C64.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	C64.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with C64.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/logger/logger.h>

extern "C"
{
	#include "log.h"
	#include "uiapi.h"
	#include "uimon.h"
	#include "archapi.h"
}

int log_message(log_t log, const char *format, ...)
{
	if(!logger_isEnabled())
		return 0;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(LOG_M, format, ap);
	logger_printf(LOG_M, "\n");
	va_end(ap);
	return 0;
}

int log_warning(log_t log, const char *format, ...)
{
	if(!logger_isEnabled())
		return 0;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(LOG_W, format, ap);
	logger_printf(LOG_M, "\n");
	va_end(ap);
	return 0;
}

int log_error(log_t log, const char *format, ...)
{
	if(!logger_isEnabled())
		return 0;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(LOG_E, format, ap);
	logger_printf(LOG_M, "\n");
	va_end(ap);
	return 0;
}

int log_debug(const char *format, ...)
{
	if(!logger_isEnabled())
		return 0;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(LOG_D, format, ap);
	logger_printf(LOG_M, "\n");
	va_end(ap);
	return 0;
}

int log_verbose(const char *format, ...)
{
	if(!logger_isEnabled())
		return 0;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(LOG_D, format, ap);
	logger_printf(LOG_M, "\n");
	va_end(ap);
	return 0;
}

CLINK void archdep_startup_log_error(const char *format, ...)
{
	if(!logger_isEnabled())
		return;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(0, format, ap);
	va_end(ap);
}

CLINK void ui_error(const char *format,...)
{
	if(!logger_isEnabled())
		return;
  va_list ap;
  va_start(ap, format);
  logger_vprintf(0, format, ap);
  va_end(ap);
}

CLINK int uimon_out(const char *buffer)
{
	logger_printf(0, "uimon_out: %s", buffer);
	return 0;
}
