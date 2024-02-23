/*
 * console.c - SDL specific console access interface.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Andreas Boose <viceteam@t-online.de>
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

#include "vice.h"

#include <stdio.h>

#include "console.h"

int native_console_init(void)
{
    return 0;
}

char *native_console_in(console_t *log, const char *prompt)
{
    return NULL;
}

int native_console_out(console_t *log, const char *format, ...)
{
#ifdef DEBUG_CONSOLE
    fprintf(stderr, "%s - remove this\n", __func__);
#endif
    return 0;
}

int native_console_petscii_out(console_t *log, const char *format, ...)
{
#ifdef DEBUG_CONSOLE
    fprintf(stderr, "%s - remove this\n", __func__);
#endif
    return 0;
}

int native_console_close_all(void)
{
    return 0;
}

console_t *native_console_open(const char *id)
{
    return NULL;
}

int native_console_close(console_t *log)
{
    return 0;
}
