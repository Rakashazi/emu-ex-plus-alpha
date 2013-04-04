/*
 * sounddump.c - Implementation of the dump sound device.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

#include "sound.h"
#include "types.h"

static FILE *dump_fd = NULL;

static int dump_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    /* No stereo capability. */
    *channels = 1;

    dump_fd = fopen(param ? param : "vicesnd.sid", "w");
    return !dump_fd;
}

static int dump_write(SWORD *pbuf, size_t nr)
{
    return 0;
}

static int dump_dump(WORD addr, BYTE byte, CLOCK clks)
{
    return (fprintf(dump_fd, "%d %d %d\n", (int)clks, addr, byte) < 0);
}

static int dump_flush(char *state)
{
    if (fprintf(dump_fd, "%s", state) < 0) {
        return 1;
    }

    return fflush(dump_fd);
}

static void dump_close(void)
{
    fclose(dump_fd);
    dump_fd = NULL;
}

static sound_device_t dump_device =
{
    "dump",
    dump_init,
    dump_write,
    dump_dump,
    dump_flush,
    NULL,
    dump_close,
    NULL,
    NULL,
    0,
    1
};

int sound_init_dump_device(void)
{
    return sound_register_device(&dump_device);
}
