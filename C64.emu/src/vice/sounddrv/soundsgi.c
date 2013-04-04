/*
 * soundsgi.c - Implementation of the SGI sound device
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

#if defined(HAVE_DMEDIA_AUDIO_H)
#include <dmedia/audio.h>
#endif

#if defined(HAVE_BSTRING_H)
#include <bstring.h>
#endif

static ALconfig sgi_audioconfig = NULL;
static ALport sgi_audioport = NULL;

static void sgi_errorhandler(long err, const char *msg, ...)
{
    printf("sgierrorhandler: %d, %s\n", (int)err, msg);
}

static int sgi_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    long chpars[] = { AL_OUTPUT_RATE, 0 };
    int st;

    /* No stereo capability. */
    *channels = 1;

    ALseterrorhandler(sgi_errorhandler);
    chpars[1] = *speed;
    st = ALsetparams(AL_DEFAULT_DEVICE, chpars, 2);
    if (st < 0) {
        return 1;
    }
    st = ALgetparams(AL_DEFAULT_DEVICE, chpars, 2);
    if (st < 0) {
        return 1;
    }
    *speed = chpars[1];

    sgi_audioconfig = ALnewconfig();
    if (!sgi_audioconfig) {
        return 1;
    }
    st = ALsetchannels(sgi_audioconfig, AL_MONO);
    if (st < 0) {
        goto fail;
    }
    st = ALsetwidth(sgi_audioconfig, AL_SAMPLE_16);
    if (st < 0) {
        goto fail;
    }
    st = ALsetqueuesize(sgi_audioconfig, *fragsize * *fragnr);
    if (st < 0) {
        goto fail;
    }
    sgi_audioport = ALopenport("outport", "w", sgi_audioconfig);
    if (!sgi_audioport) {
        goto fail;
    }
    return 0;
fail:
    ALfreeconfig(sgi_audioconfig);
    sgi_audioconfig = NULL;
    return 1;
}

static int sgi_write(SWORD *pbuf, size_t nr)
{
    int i;
    i = ALwritesamps(sgi_audioport, pbuf, nr);
    if (i < 0) {
        return 1;
    }
    return 0;
}

static int sgi_bufferspace(void)
{
    int i;
    /* ALgetfillable returns space in samples. */
    i = ALgetfillable(sgi_audioport);
    return i;
}

static void sgi_close(void)
{
    /* XXX: code missing */
    ALfreeconfig(sgi_audioconfig);
    sgi_audioconfig = NULL;
}

static sound_device_t sgi_device =
{
    "sgi",
    sgi_init,
    sgi_write,
    NULL,
    NULL,
    sgi_bufferspace,
    sgi_close,
    NULL,
    NULL,
    1,
    1
};

int sound_init_sgi_device(void)
{
    return sound_register_device(&sgi_device);
}
