/*
 * soundarts.c - Implementation of the ARTS soundserver sound device
 *
 * Written by
 *  Ricardo Ferreira <storm@atdot.org>
 * Based on sounduss.c by
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
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "log.h"
#include "sound.h"

/*
** Include the ARTS C API
*/
#include <artsc.h>

static arts_stream_t arts_st = (arts_stream_t)-1;
static int artsdrv_speed;
static int artsdrv_bits;
static int artsdrv_fragsize;
static int artsdrv_fragnr;
static double artsdrv_bufsize;
static int artsdrv_suspended = 0;
/*
 * static int artsdrv_bs=0;
*/

static int artsdrv_bufferspace(void);

static int artsdrv_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    /* No stereo capability. */
    *channels = 1;

    /*
    ** Connect to ARTS
    */
    if (arts_init()) {
        log_message(LOG_DEFAULT, "Cannot connect to ARTS soundserver.");
        return 1;
    }
    /*
    ** Open a stream for playing
    */
    artsdrv_bits = 16;
    arts_st = arts_play_stream(*speed, 16, 1, "vice");
    if (arts_st < 0) {
        artsdrv_bits = 8;
        arts_st = arts_play_stream(*speed, 8, 1, "vice");
        if (arts_st < 0) {
            log_message(LOG_DEFAULT, "Cannot open stream for playing.");
            return 1;
        }
    }

    /*
    ** Try to get correct buffer size.
    */
    arts_stream_set(arts_st, ARTS_P_BUFFER_SIZE, *fragnr * *fragsize * sizeof(SWORD));
    arts_stream_set(arts_st, ARTS_P_BLOCKING, 1);

    *fragsize = arts_stream_get(arts_st, ARTS_P_PACKET_SIZE);
    *fragnr = arts_stream_get(arts_st, ARTS_P_PACKET_COUNT);
    artsdrv_speed = *speed;
    artsdrv_fragsize = *fragsize;
    artsdrv_fragnr = *fragnr;
    artsdrv_bufsize = arts_stream_get(arts_st, ARTS_P_BUFFER_SIZE);
/*    artsdrv_bs=artsdrv_bufsize;*/
    return 0;
}

static int artsdrv_write(SWORD *pbuf, size_t nr)
{
#ifdef WORDS_BIGENDIAN
    int i;

    for (i = 0; i < nr; ++i) {
        pbuf[i] = (pbuf[i] >> 8 ) | ((pbuf[i] & 0x0f) << 8);
    }
#endif /* WORDS_BIGENDIAN */

    arts_write(arts_st, (void*)pbuf, nr * sizeof(SWORD));
    return 0;
}

static int artsdrv_bufferspace(void)
{
    /* arts_stream_get(arts_st,ARTS_P_BUFFER_SPACE) returns space in bytes. */
    return arts_stream_get(arts_st, ARTS_P_BUFFER_SPACE) / sizeof(SWORD);
}

static void artsdrv_close(void)
{
    arts_close_stream(arts_st);
    arts_st = (arts_stream_t)-1;
    arts_free();
}


#if 0
/*
** As the aRts C API doesn't support a DSP_SNDCTL_POST equivalent
** function, i tried to emulate but apparently its not needed so
** i don't use them.
*/
static int artsdrv_suspend(void)
{
    if (arts_st >= 0) {
        arts_close_stream(arts_st);
        arts_st = (arts_stream_t*)-1;
    }

    artsdrv_suspended = 1;
    return 0;
}
#endif

int artsdrv_resume(void)
{
    if (artsdrv_suspended == 1) {
        arts_st = arts_play_stream(artsdrv_speed, artsdrv_bits, 1, "vice");
        arts_stream_set(arts_st, ARTS_P_BUFFER_SIZE, artsdrv_bufsize * sizeof(SWORD));
        artsdrv_suspended = 0;
    }
    return 0;
}

static sound_device_t artsdrv_device =
{
    "arts",
    artsdrv_init,
    artsdrv_write,
    NULL,
    NULL,
    artsdrv_bufferspace,
    artsdrv_close,
    NULL,  /* artsdrv_suspend */
    NULL, /* artsdrv_resume */
    1,
    1
};

int sound_init_arts_device(void)
{
    return sound_register_device(&artsdrv_device);
}
