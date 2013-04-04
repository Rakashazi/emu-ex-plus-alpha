/*
 * soundmovie.c - Implementation of the audio stream for movie encoding
 *
 * Written by
 *  Andreas Matthies <andreas.matthies@gmx.net>
 *  Christian Vogelgsang <chris@vogelgsang.org>
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
#include <string.h>

#include "log.h"
#include "sound.h"
#include "resources.h"
#include "types.h"
#include "archdep.h"
#include "soundmovie.h"

static soundmovie_buffer_t *buffer = NULL;
static soundmovie_funcs_t  *funcs = NULL;

int soundmovie_start(soundmovie_funcs_t *f)
{
    funcs = f;

    resources_set_string("SoundRecordDeviceName", "soundmovie");
    resources_set_int("Sound", 1);
    return 0;
}

int soundmovie_stop(void)
{
    resources_set_string("SoundRecordDeviceName", "");
    return 0;
}

static int soundmovie_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    if ((funcs != NULL) && (funcs->init != NULL)) {
        return funcs->init(*speed, *channels, &buffer);
    }

    return -1;
}

static int soundmovie_write(SWORD *pbuf, size_t nr)
{
    size_t copied = 0;
    int samples_to_copy;
    int buffer_size;

    if ((funcs == NULL) || (funcs->encode == NULL)) {
        return 0;
    }
    if ((buffer == NULL) || (buffer->size == 0)) {
        return 0;
    }

    buffer_size = buffer->size;
    while (copied < nr) {
        samples_to_copy = buffer_size - buffer->used;
        if (samples_to_copy > (int)(nr - copied)) {
            samples_to_copy = (int)(nr - copied);
        }
        memcpy(buffer->buffer + buffer->used, pbuf + copied, samples_to_copy * sizeof(SWORD));
        buffer->used += samples_to_copy;
        copied += samples_to_copy;
        if (buffer->used == buffer_size) {
            funcs->encode(buffer);
            buffer->used = 0;
        }
    }

    return 0;
}


static void soundmovie_close(void)
{
    if ((funcs != NULL) && (funcs->close != NULL)) {
        funcs->close();
    }
}

static sound_device_t soundmovie_device =
{
    "soundmovie",
    soundmovie_init,
    soundmovie_write,
    NULL,
    NULL,
    NULL,
    soundmovie_close,
    NULL,
    NULL,
    0,
    2
};

int sound_init_movie_device(void)
{
    return sound_register_device(&soundmovie_device);
}
