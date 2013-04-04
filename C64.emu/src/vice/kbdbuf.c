/*
 * kbdbuf.c - Kernal keyboard buffer handling for VICE.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  André Fachat <a.fachat@physik.tu-chemnitz.de>
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "kbdbuf.h"
#include "lib.h"
#include "maincpu.h"
#include "mem.h"
#include "translate.h"
#include "types.h"


/* Maximum number of characters we can queue.  */
#define QUEUE_SIZE      16384

/* First location of the buffer.  */
static int buffer_location;

/* Location that stores the number of characters pending in the
   buffer.  */
static int num_pending_location;

/* Maximum number of characters that fit in the buffer.  */
static int buffer_size;

/* Number of cycles needed to initialize the Kernal.  */
static CLOCK kernal_init_cycles;

/* Characters in the queue.  */
static BYTE queue[QUEUE_SIZE];

/* Next element in `queue' we must push into the kernal's queue.  */
static int head_idx;

/* Number of pending characters.  */
static int num_pending;

/* Flag if we are initialized already.  */
static int kbd_buf_enabled = 0;

/* String to feed into the keyboard buffer.  */
static char *kbd_buf_string = NULL;

/* ------------------------------------------------------------------------- */

static void kbd_buf_parse_string(const char *string)
{
    unsigned int i, j;
    size_t len;

    len = strlen(string);

    if (len > QUEUE_SIZE) {
        len = QUEUE_SIZE;
    }

    kbd_buf_string = lib_realloc(kbd_buf_string, len + 1);
    memset(kbd_buf_string, 0, len + 1);

    for (i = 0, j = 0; i < len; i++) {
        if (string[i] == '\\' && i < (len - 2) && isxdigit((int)string[i + 1])
            && isxdigit((int)string[i + 2])) {
            char hexvalue[3];

            hexvalue[0] = string[i + 1];
            hexvalue[1] = string[i + 2];
            hexvalue[2] = '\0';
            kbd_buf_string[j] = (char)strtol(hexvalue, NULL, 16);
            j++;
            i += 2;
        } else {
            kbd_buf_string[j] = string[i];
            j++;
        }
    }
}

int kbdbuf_feed_string(const char *string)
{
    kbd_buf_parse_string(string);

    return kbdbuf_feed(kbd_buf_string);
}

static int kdb_buf_feed_cmdline(const char *param, void *extra_param)
{
    kbd_buf_parse_string(param);

    return 0;
}

static const cmdline_option_t cmdline_options[] =
{
    { "-keybuf", CALL_FUNCTION, 1,
      kdb_buf_feed_cmdline, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_STRING, IDCLS_PUT_STRING_INTO_KEYBUF,
      NULL, NULL },
    { NULL }
};

int kbdbuf_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

void kbdbuf_reset(int location, int plocation, int size, CLOCK mincycles)
{
    buffer_location = location;
    num_pending_location = plocation;
    buffer_size = size;
    kernal_init_cycles = mincycles;

    if (mincycles) {
        kbd_buf_enabled = 1;
    } else {
        kbd_buf_enabled = 0;
    }
}

/* Initialization.  */
void kbdbuf_init(int location, int plocation, int size, CLOCK mincycles)
{
    kbdbuf_reset(location, plocation, size, mincycles);

    if (kbd_buf_string != NULL) {
        kbdbuf_feed(kbd_buf_string);
    }
}

void kbdbuf_shutdown(void)
{
    lib_free(kbd_buf_string);
}

/* Return nonzero if the keyboard buffer is empty.  */
int kbdbuf_is_empty(void)
{
    return (int)(mem_read((WORD)(num_pending_location)) == 0);
}

/* Feed `s' into the queue.  */
int kbdbuf_feed(const char *string)
{
    const int num = (int)strlen(string);
    int i, p;

    if (num_pending + num > QUEUE_SIZE || !kbd_buf_enabled) {
        return -1;
    }

    for (p = (head_idx + num_pending) % QUEUE_SIZE, i = 0;
         i < num; p = (p + 1) % QUEUE_SIZE, i++) {
        queue[p] = string[i];
    }

    num_pending += num;

    /* XXX: We waste time this way, as we copy into the queue and then into
       memory.  */
    kbdbuf_flush();

    return 0;
}

/* Flush pending characters into the kernal's queue if possible.  */
void kbdbuf_flush(void)
{
    unsigned int i, n;

    if ((!kbd_buf_enabled)
        || num_pending == 0
        || maincpu_clk < kernal_init_cycles
        || !kbdbuf_is_empty()) {
        return;
    }

    n = num_pending > buffer_size ? buffer_size : num_pending;
    for (i = 0; i < n; head_idx = (head_idx + 1) % QUEUE_SIZE, i++) {
        mem_store((WORD)(buffer_location + i), queue[head_idx]);
    }

    mem_store((WORD)(num_pending_location), (BYTE)(n));
    num_pending -= n;
}
