/*
 * soundahi.c - Implementation of the ahi sound device
 *
 * Written by
 *  Mathias Roslund <vice.emu@amidog.se>
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
#include "ahi.h"

static int num_channels = 1;

static int _ahi_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    u32 mode;

/*
  printf("speed: %d, fragsize: %d, fragnr: %d, channels: %d\n", *speed, *fragsize, *fragnr, *channels);
*/

    if (*channels == 1) {
        mode = AUDIO_M16S;
    } else {
        mode = AUDIO_S16S;
    }

    num_channels = *channels;

    if (ahi_open(*speed, mode, *fragsize, *fragnr, NULL) != 0) {
        return 1;
    }

    return 0;
}

static int _ahi_write(SWORD *pbuf, size_t nr)
{
    nr /= num_channels; /* we want "samples per channel" */
    ahi_play_samples(pbuf, nr, NOTIME, NOWAIT);
    return 0;
}

static int _ahi_bufferspace(void)
{
    return ahi_samples_free();
}

static void _ahi_close(void)
{
    ahi_close();
}

static int _ahi_suspend(void)
{
    ahi_pause();
    return 0;
}

static int _ahi_resume(void)
{
    return 0;
}

static sound_device_t ahi_device =
{
    "ahi",
    _ahi_init,
    _ahi_write,
    NULL,
    NULL,
    _ahi_bufferspace,
    _ahi_close,
    _ahi_suspend,
    _ahi_resume,
    1,
    2
};

int sound_init_ahi_device(void)
{
    return sound_register_device(&ahi_device);
}
