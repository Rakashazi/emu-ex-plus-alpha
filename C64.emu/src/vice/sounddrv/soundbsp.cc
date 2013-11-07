/*
 * soundbsp.cc - Implementation of the BeOS Media Kit (BSoundPlayer)
 *               sound device.
 *
 * Written by
 *  Marcus Sutton <loggedoubt@gmail.com>
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

#include <SoundPlayer.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "lib.h"
#include "log.h"
#include "sound.h"
#include "util.h"
}
/* ------------------------------------------------------------------------- */

/* the BeOS soundplayer node */
static BSoundPlayer *sound_player;

/* the buffer */
static SWORD *sound_buf;

/* the buffer's length */
static size_t buf_len = 0;

/* the next position to write */
static volatile size_t buf_inptr = 0;

static volatile size_t buf_outptr = 0;

static volatile int buf_full = 0;

/* Size of fragment (bytes).  */
static unsigned int fragment_size;

/* Number of channels */
static unsigned int num_of_channels;

/* ------------------------------------------------------------------------- */

static void bsp_callback(void *param, void *cb_buf, size_t len,
                                const media_raw_audio_format &format)
{
    int amount, total;

    total = 0;
    while (total < len / sizeof(SWORD)) {
        amount = buf_inptr - buf_outptr;
        if (amount <= 0) {
            amount = buf_len - buf_outptr;
        }

        if (amount + total > len / sizeof(SWORD)) {
            amount = len / sizeof(SWORD) - total;
        }

        buf_full = 0;

        if (!amount) {
            memset((SWORD *)cb_buf + total, 0, len - total * sizeof(SWORD));
            return;
        }

        memcpy((SWORD *)cb_buf + total, sound_buf + buf_outptr, amount * sizeof(SWORD));
        total += amount;
        buf_outptr += amount;

        if (buf_outptr == buf_len) {
            buf_outptr = 0;
        }
    }
}

static int bsp_init(const char *param, int *speed,
                                            int *fragsize, int *fragnr, int *channels)
{
    media_raw_audio_format audio_format;

    fragment_size = *fragsize;
    num_of_channels = *channels;

    audio_format.frame_rate = (float)*speed;
    audio_format.channel_count = *channels;
    audio_format.format = media_raw_audio_format::B_AUDIO_SHORT;
    audio_format.byte_order = B_MEDIA_LITTLE_ENDIAN;
    audio_format.buffer_size = (size_t) *fragsize * *fragnr * *channels;
        
    sound_player = new BSoundPlayer(&audio_format, "VICE bsp", &bsp_callback);
    if (sound_player->InitCheck() != B_OK) {
        log_error(LOG_DEFAULT, "sound (bsp_init): Failed to initialize BSoundPlayer");
        return -1;
    }

    buf_len = audio_format.buffer_size;
    buf_inptr = buf_outptr = buf_full = 0;
    sound_buf = (SWORD *)lib_malloc(sizeof(SWORD) * buf_len);
    memset(sound_buf, 0, sizeof(SWORD) * buf_len);

    if (sound_player->Start()) {
        log_error(LOG_DEFAULT, "sound (bsp_init): Failed to start playing");
        return -1;
    }

    sound_player->SetHasData(true);
    return 0;
}

static int bsp_write(SWORD *pbuf, size_t nr)
{
    int total, amount;
    total = 0;


    while (total < nr) {
        amount = buf_outptr - buf_inptr;

        if (amount <= 0) {
            amount = buf_len - buf_inptr;
        }

        if (total + amount > nr) {
            amount = nr - total;
        }

        if (amount <= 0) {
            snooze(1000);
            continue;
        }

        memcpy(sound_buf + buf_inptr, pbuf + total, amount * sizeof(SWORD));
        buf_inptr += amount;
        total += amount;

        if (buf_inptr == buf_len) {
            buf_inptr = 0;
        }
    }

    if (buf_inptr == buf_outptr) {
        buf_full = 1;
    }
        
        return 0;
}

static int bsp_bufferspace(void)
{       
    int amount;

    if (buf_full) {
        amount = buf_len;
    } else {
        amount = buf_inptr - buf_outptr;
    }

    if (amount < 0) {
        amount += buf_len;
    }

    return buf_len - amount;
}

static void bsp_close(void)
{
    delete sound_player;
}

static int bsp_suspend(void)
{
    sound_player->Stop();
    return 0;
}

static int bsp_resume(void)
{
    memset(sound_buf, 0, buf_len);
    if (sound_player->Start()) {
        log_error(LOG_DEFAULT, "sound (bsp_resume): Failed to start playing");
        return -1;
    }
    return 0;
}

static sound_device_t bsp_device =
{
    "bsp",
    bsp_init,
    bsp_write,
    NULL,
    NULL,
    bsp_bufferspace,
    bsp_close,
    bsp_suspend,
    bsp_resume,
    1,
    2
};

int sound_init_bsp_device(void)
{
    return sound_register_device(&bsp_device);
}
