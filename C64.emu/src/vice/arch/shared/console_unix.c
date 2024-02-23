/** \file   console_unix.c
 * \brief   Unix specific console access interface for SDL.
 *
 * \author  Hannu Nuotio <hannu.nuotio@tut.fi>
 * \author  Andreas Boose <viceteam@t-online.de>
 *
 * TODO:    Properly document this code.
 */

/*
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include "console.h"
#include "lib.h"
#include "log.h"
#include "charset.h"

#include "lib/linenoise-ng/linenoise.h"


typedef struct console_private_s {
    FILE *input;
    FILE *output;
} console_private_t;

console_t *native_console_open(const char *id)
{
#ifdef HAVE_SYS_IOCTL_H
    struct winsize w;
#endif

    console_t *console;
    console_private_t *conpriv;

    console = lib_malloc(sizeof(console_t));
    conpriv = lib_malloc(sizeof(console_private_t));
    console->private = conpriv;
    conpriv->input = NULL;
    conpriv->output = NULL;

    if (!isatty(fileno(stdin))) {
        log_error(LOG_DEFAULT, "native_console_open: stdin is not a tty.");
        goto exitnull;
    }
    if (!isatty(fileno(stdout))) {
        log_error(LOG_DEFAULT, "native_console_open: stdout is not a tty.");
        goto exitnull;
    }

    /* change window title for console identification purposes */
    if (getenv("WINDOWID") == NULL) {
        printf("\033]2;VICE monitor console (%d)\007", (int)getpid());
    }

    linenoiseHistorySetMaxLen(100);

#ifdef HAVE_SYS_IOCTL_H
    if (ioctl(fileno(stdin), TIOCGWINSZ, &w)) {
        console->console_xres = 80;
        console->console_yres = 25;
    } else {
        console->console_xres = w.ws_col >= 40 ? w.ws_col : 40;
        console->console_yres = w.ws_row >= 22 ? w.ws_row : 22;
    }
#else
    console->console_xres = 80;
    console->console_yres = 25;
#endif
    console->console_can_stay_open = 1;
    console->console_cannot_output = 0;

    return console;

exitnull:
    lib_free(console->private);
    lib_free(console);
    return NULL;
}

int native_console_close(console_t *log)
{
    lib_free(log->private);
    lib_free(log);
    linenoiseHistoryFree();
    return 0;
}

int native_console_out(console_t *log, const char *format, ...)
{
    va_list ap;
    char *buf;

    va_start(ap, format);
    buf = lib_mvsprintf(format, ap);
    va_end(ap);

    if (buf) {
        if (log && (log->private->output)) {
            fprintf(log->private->output, "%s", buf);
        } else {
            fprintf(stdout, "%s", buf);
        }
        lib_free(buf);
    }
    return 0;
}

int native_console_petscii_out(console_t *log, const char *format, ...)
{
    va_list ap;
    char *buf;
    unsigned char c;
    int i;

    va_start(ap, format);
    buf = lib_mvsprintf(format, ap);
    va_end(ap);

    if (buf) {
        for (i = 0; (c = buf[i]) != 0; i++) {
            if (c == '\t') {
                buf[i] = ' ';
            } else if (((c < 32) || (c > 126)) && (c != '\n')) {
                buf[i] = charset_p_toascii(c, CONVERT_WITH_CTRLCODES);
            }
        }

        if (log && (log->private->output)) {
            fprintf(log->private->output, "%s", buf);
        } else {
            fprintf(stdout, "%s", buf);
        }
        lib_free(buf);
    }
    return 0;
}

char *native_console_in(console_t *log, const char *prompt)
{
    char *p;
    char *ret_string;

    p = linenoise(prompt);
    if (p != NULL && *p != '\0') {
        linenoiseHistoryAdd(p);
    }
    ret_string = lib_strdup(p);
    free(p);

    return ret_string;
}

int native_console_init(void)
{
    return 0;
}

int native_console_close_all(void)
{
    return 0;
}
