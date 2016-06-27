/*
 * soundpulse.c - Pulseaudio support
 *
 * Written by
 *  Antti S. Lankila <alankila@bel.fi>
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

#include "log.h"
#include "sound.h"

#include <pulse/simple.h>
#include <pulse/error.h>

static pa_simple *s = NULL;

static pa_sample_spec ss = {
    .format = PA_SAMPLE_S16LE,
    .rate = (uint32_t) -1,
    .channels = 0,
};

static pa_buffer_attr attr = {
    .maxlength = (uint32_t) -1,
    .tlength = (uint32_t) -1,
    .prebuf = (uint32_t) -1,
    .minreq = (uint32_t) -1,
    .fragsize = (uint32_t) -1,
};


/* This driver does not use the bufferspace function because it should be
 * unnecessary. Pulse is already going to do its own thing regarding latency
 * and hopefully just does the right thing for us without forcing us to
 * bother with our own timing code. */
static int pulsedrv_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    int error = 0;

    ss.rate = *speed;
    ss.channels = *channels;

    attr.fragsize = *fragsize * 2;
    attr.tlength = *fragsize * *fragnr * 2;

    s = pa_simple_new(NULL, "VICE", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, &attr, &error);
    if (s == NULL) {
        log_error(LOG_DEFAULT, "pa_simple_new(): %s", pa_strerror(error));
        return 1;
    }

    return 0;
}

static int pulsedrv_write(SWORD *pbuf, size_t nr)
{
    int error = 0;
    if (pa_simple_write(s, pbuf, nr * 2, &error)) {
        log_error(LOG_DEFAULT, "pa_simple_write(,%d): %s", (int)nr, pa_strerror(error));
        return 1;
    }

    return 0;
}

static int pulsedrv_suspend(void)
{
    int error = 0;
    if (pa_simple_flush(s, &error)) {
        log_error(LOG_DEFAULT, "pa_simple_flush(): %s", pa_strerror(error));
        return 1;
    }
    return 0;
}

static void pulsedrv_close(void)
{
    int error = 0;
    if (pa_simple_flush(s, &error)) {
        log_error(LOG_DEFAULT, "pa_simple_flush(): %s", pa_strerror(error));
        /* don't stop */
    }
    pa_simple_free(s);
    s = NULL;
}

static sound_device_t pulsedrv_device =
{
    "pulse",
    pulsedrv_init,
    pulsedrv_write,
    NULL,
    NULL,
    NULL,
    pulsedrv_close,
    pulsedrv_suspend,
    NULL,
    1,
    2
};

int sound_init_pulse_device(void)
{
    return sound_register_device(&pulsedrv_device);
}
