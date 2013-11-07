/*
 * soundbeos.cc - Implementation of the BeOS sound device.
 *
 * Written by
 *  Andreas Matthies <andreas.matthies@gmx.net>
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

#ifdef __HAIKU__
/* Workaround an issue in the Haiku headers. */
#include <List.h>
#endif
#include <PushGameSound.h>
#include <SoundPlayer.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "log.h"
#include "sound.h"
#include "util.h"
}
/* ------------------------------------------------------------------------- */

/* the BeOS soundplayer node */
static BPushGameSound *game_sound;

/* the cyclic buffer */
static SWORD *soundbuffer;

/* the buffer's length */
static size_t bufferlength;

/* the next position to write */
static size_t write_position;

/* Size of fragment (bytes).  */
static unsigned int fragment_size;

/* Number of channels */
static unsigned int num_of_channels;

/* ------------------------------------------------------------------------- */

static int beos_init(const char *param, int *speed,
                                                int *fragsize, int *fragnr, int *channels)
{
        gs_audio_format audio_format;

        fragment_size = *fragsize;
        num_of_channels = *channels;
        
        audio_format.frame_rate = (float)*speed;
        audio_format.channel_count = *channels;
        audio_format.format = gs_audio_format::B_GS_S16;
        audio_format.byte_order = B_MEDIA_LITTLE_ENDIAN;
        audio_format.buffer_size = (size_t) *fragsize * *fragnr * *channels;
        
        game_sound = new BPushGameSound(*fragsize,
                                        &audio_format, *fragnr);
        if (game_sound->InitCheck() != B_OK) {
                log_error(LOG_DEFAULT, "sound (beos_init): Failed to initialize Be's PushGameSound");
                return -1;
        }

        if (game_sound->LockForCyclic((void **)&soundbuffer, &bufferlength) 
                        == BPushGameSound::lock_failed) {
                        log_error(LOG_DEFAULT, "sound (beos_init): LockForCyclic failed");
                        return -1;
        }
        memset(soundbuffer, 0, bufferlength);
        game_sound->StartPlaying();
        
        write_position = game_sound->CurrentPosition();

    return 0;
}

extern CLOCK clk;

static int beos_write(SWORD *pbuf, size_t nr)
{
        int i,count;
        SWORD *p;
        
        count = nr / fragment_size;
#if 0
        while (game_sound->CurrentPosition()*num_of_channels == write_position);
#endif
        for (i=0; i<count; i++) {
                p = (SWORD*) (soundbuffer+write_position);
                memcpy(p,pbuf,fragment_size*2);
                write_position += fragment_size;
                if (write_position*2 >= bufferlength)
                        write_position = 0;
                pbuf+=fragment_size;
        }
        
        return 0;
}

static int beos_bufferspace(void)
{       
        int ret;
        int current = game_sound->CurrentPosition();

        ret = current - (write_position / num_of_channels);
        if (ret < 0)
                ret += (bufferlength/(2*num_of_channels));

        return ret;                     
}

static void beos_close(void)
{
        delete game_sound;
}

static int beos_suspend(void)
{
    game_sound->StopPlaying();
    game_sound->UnlockCyclic();
    return 0;
}

static int beos_resume(void)
{
        if (game_sound->LockForCyclic((void **)&soundbuffer, &bufferlength) 
                        == BPushGameSound::lock_failed) {
                        log_error(LOG_DEFAULT, "sound (beos_resume): LockForCyclic failed");
                        return -1;
        }
        memset(soundbuffer, 0, bufferlength);
        game_sound->StartPlaying();
    return 0;
}

static sound_device_t beos_device =
{
    "beos",
    beos_init,
    beos_write,
    NULL,
    NULL,
    beos_bufferspace,
    beos_close,
    beos_suspend,
    beos_resume,
    1,
    2
};

int sound_init_beos_device(void)
{
    return sound_register_device(&beos_device);
}
