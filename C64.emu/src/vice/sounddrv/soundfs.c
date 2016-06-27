/*
 * soundfs.c - Implementation of the filesystem dump sound device
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
#include "archdep.h"

static FILE *fs_fd = NULL;

static int fs_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    /* No stereo capability. */
    *channels = 1;

    fs_fd = fopen(param ? param : "vicesnd.raw", MODE_WRITE);
    return !fs_fd;
}

static int fs_write(SWORD *pbuf, size_t nr)
{
    return fwrite(pbuf, sizeof(SWORD), nr, fs_fd) != nr;
}

static void fs_close(void)
{
    fclose(fs_fd);
    fs_fd = NULL;
}

static sound_device_t fs_device =
{
    "fs",
    fs_init,
    fs_write,
    NULL,
    NULL,
    NULL,
    fs_close,
    NULL,
    NULL,
    0,
    1
};

int sound_init_fs_device(void)
{
    return sound_register_device(&fs_device);
}
