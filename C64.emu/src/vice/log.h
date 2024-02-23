/*
 * log.h - Logging facility.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef VICE_LOG_H
#define VICE_LOG_H

#include "vice.h"

#include <stdio.h>


#define LOG_LEVEL_NONE  0x00

typedef signed int log_t;
#define LOG_ERR     ((log_t)-1)
#define LOG_DEFAULT ((log_t)-2)

int log_resources_init(void);
void log_resources_shutdown(void);
int log_cmdline_options_init(void);
int log_init(void);
int log_init_with_fd(FILE *f);
log_t log_open(const char *id);
int log_close(log_t log);
void log_close_all(void);
void log_enable(int on);
int log_set_silent(int n);
int log_set_verbose(int n);
int log_early_init(int argc, char **argv);

int log_message(log_t log, const char *format, ...) VICE_ATTR_PRINTF2;
int log_warning(log_t log, const char *format, ...) VICE_ATTR_PRINTF2;
int log_error(log_t log, const char *format, ...) VICE_ATTR_PRINTF2;
int log_debug(const char *format, ...) VICE_ATTR_PRINTF;
int log_verbose(const char *format, ...) VICE_ATTR_PRINTF;

#endif
