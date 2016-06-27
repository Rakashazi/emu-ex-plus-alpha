/*
 * mon_util.c - Utilities for the VICE built-in monitor.
 *
 * Written by
 *  Spiro Trikaliotis <spiro.trikaliotis@gmx.de>
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

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "archdep.h"
#include "console.h"
#include "lib.h"
#include "mem.h"
#include "mon_disassemble.h"
#include "mon_util.h"
#include "monitor.h"
#include "monitor_network.h"
#include "types.h"
#include "uimon.h"


static char *bigbuffer = NULL;
static const unsigned int bigbuffersize = 10000;
static unsigned int bigbufferwrite = 0;

static FILE *mon_log_file = NULL;

/******************************************************************************/

int mon_log_file_open(const char *name)
{
    FILE *fp;

    if (name) {
        /* try to open new file */
        fp = fopen(name, MODE_WRITE_TEXT);
        if (fp) {
            /* close old logfile */
            mon_log_file_close();
            mon_log_file = fp;
            return 0;
        }
    }
    return -1;
}

void mon_log_file_close(void)
{
    if (mon_log_file) {
        fclose(mon_log_file);
    }
    mon_log_file = NULL;
}

static int mon_log_file_out(const char *buffer)
{
    int len;

    if ((mon_log_file) && (buffer)) {
        len = strlen(buffer);
        if (fwrite(buffer, 1, len, mon_log_file) == (size_t)len) {
            return 0;
        }
    }
    return -1;
}

/******************************************************************************/

static void mon_buffer_alloc(void)
{
    if (!bigbuffer) {
        bigbuffer = lib_malloc(bigbuffersize + 1);
        bigbuffer[bigbuffersize] = 0;
    }
}

static int mon_buffer_flush(void)
{
    int rv = 0;

    if (bigbuffer && bigbufferwrite) {
        bigbufferwrite = 0;
        rv = uimon_out(bigbuffer);
    }

    return rv;
}

/* like strncpy, but:
 * 1. always add a null character
 * 2. do not fill the rest of the buffer
 */
static void stringcopy_n(char *dest, const char *src, unsigned int len)
{
    while (*src && len--) {
        *dest++ = *src++;
    }

    *dest = 0;
}

static void mon_buffer_add(const char *buffer, unsigned int bufferlen)
{
    if (bigbufferwrite + bufferlen > bigbuffersize) {
        /* the buffer does not fit into bigbuffer, thus,
           flush the buffer! */
        mon_buffer_flush();
    }

    if (bigbufferwrite + bufferlen <= bigbuffersize) {
        stringcopy_n(&bigbuffer[bigbufferwrite], buffer, bufferlen);
        bigbufferwrite += bufferlen;

        assert(bigbufferwrite <= bigbuffersize);
    }
}

static int mon_out_buffered(const char *buffer)
{
    int rv = 0;

    if (!console_log || console_log->console_cannot_output) {
        mon_buffer_alloc();
        mon_buffer_add(buffer, (unsigned int)strlen(buffer));
    } else {
        rv = mon_buffer_flush();
        rv = uimon_out(buffer) || rv;
    }

    return rv;
}

int mon_out(const char *format, ...)
{
    va_list ap;
    char *buffer;
    int rc = 0;

    va_start(ap, format);
    buffer = lib_mvsprintf(format, ap);
    va_end(ap);

#ifdef HAVE_NETWORK
    if (monitor_is_remote()) {
        rc = monitor_network_transmit(buffer, strlen(buffer));
    } else {
#endif
        rc = mon_out_buffered(buffer);
#ifdef HAVE_NETWORK
    }
#endif
    mon_log_file_out(buffer);

    lib_free(buffer);

    if (rc < 0) {
        monitor_abort();
    }

    return rc;
}

char *mon_disassemble_with_label(MEMSPACE memspace, WORD loc, int hex,
                                 unsigned *opc_size_p, unsigned *label_p)
{
    const char *p;

    if (*label_p == 0) {
        /* process a label, if available */
        p = mon_symbol_table_lookup_name(memspace, loc);
        if (p) {
            *label_p = 1;
            *opc_size_p = 0;
            return lib_msprintf("%s:", p);
        }
    } else {
        *label_p = 0;
    }

    /* process the disassembly itself */
    p = mon_disassemble_to_string_ex(memspace, loc,
                                     mon_get_mem_val(memspace, loc),
                                     mon_get_mem_val(memspace,
                                                     (WORD)(loc + 1)),
                                     mon_get_mem_val(memspace,
                                                     (WORD)(loc + 2)),
                                     mon_get_mem_val(memspace,
                                                     (WORD)(loc + 3)),
                                     hex,
                                     opc_size_p);

    return lib_msprintf((hex ? "%04X: %s%10s" : "%05u: %s%10s"), loc, p, "");
}

char *mon_dump_with_label(MEMSPACE memspace, WORD loc, int hex, unsigned *label_p)
{
    const char *p;
    BYTE val;

    if (*label_p == 0) {
        /* process a label, if available */
        p = mon_symbol_table_lookup_name(memspace, loc);
        if (p) {
            *label_p = 1;
            return lib_msprintf("%s:", p);
        }
    } else {
        *label_p = 0;
    }

    val = mon_get_mem_val(memspace, loc);
    return lib_msprintf((hex ? "%04X: $%02X   %03u   '%c'" : "%05u: $%02X   %03u   '%c'"), loc, val, val, isprint(val) ? val : ' ');
}

#ifndef __OS2__
static char *pchCommandLine = NULL;

void mon_set_command(console_t *console_log, char *command,
                     void (*pAfter)(void))
{
    pchCommandLine = command;

    mon_out("%s\n", command);

    if (pAfter) {
        (*pAfter)();
    }
}

char *uimon_in(const char *prompt)
{
    char *p = NULL;

    while (!p && !pchCommandLine) {
        /* as long as we don't have any return value... */

#ifdef HAVE_NETWORK
        if (monitor_is_remote()) {
            if (monitor_network_transmit(prompt, strlen(prompt)) < 0) {
              return NULL;
            }

            p = monitor_network_get_command_line();
            if (p == NULL) {
                mon_set_command(NULL, "x", NULL);
                return NULL;
            }
        } else {
#endif
            /* make sure to flush the output buffer */
            mon_buffer_flush();

            /* get input from the user */
            p = uimon_get_in(&pchCommandLine, prompt);
#ifdef HAVE_NETWORK
        }
#endif
    }

    if (pchCommandLine) {
        /* we have an "artificially" generated command line */
        lib_free(p);
        p = lib_stralloc(pchCommandLine);
        pchCommandLine = NULL;
    }

    mon_log_file_out(prompt);
    mon_log_file_out(p);
    mon_log_file_out("\n");

    /* return the command (the one or other way...) */
    return p;
}
#endif
