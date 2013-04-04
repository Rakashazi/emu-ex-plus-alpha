/*
 * soundallegro.c - Implementation of the Allegro sound device.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

/* This implements a pseudo-streaming device using the Allegro library.  It
   is not real streaming, though, as it basically cannot tell underflowing
   from overflowing.  */

#include "vice.h"

#include <stdio.h>
#include <allegro.h>            /* Must come after <stdio.h>.  */
#include <dpmi.h>

#include "lib.h"
#include "log.h"
#include "sound.h"

/* ------------------------------------------------------------------------- */

/* Flag: have we already initialized Allegro?  */
/*static int allegro_startup_done;*/

/* Audio buffer.  */
static SAMPLE *buffer;

/* Buffer size (bytes).  */
static unsigned int buffer_len;

/* Voice playing the buffer.  */
static int voice;

/* Size of fragment (bytes).  */
static unsigned int fragment_size;

/* Write position in the buffer.  */
static unsigned int buffer_offset;

/* Flag: have we been suspended?  */
static int been_suspended;

/* Number of samples already written; if this value is greater than the
   buffer size, it's equal to the buffer size.  This is a hack for the first
   few writes.  */
static unsigned int written_samples;

/* ------------------------------------------------------------------------- */

static int allegro_startup(unsigned int freq)
{
    log_debug("Starting up Allegro sound...  ");

    remove_sound();

    set_config_int("sound", "sb_freq", (int) freq);

    detect_digi_driver(DIGI_AUTODETECT);
    reserve_voices(1, 0);

    if (install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL) != 0) {
        log_debug("Failed: %s\n", allegro_error);
        return -1;
    }

    /* This is not a good idea, as the user might want to specify this from the
       setup program.  */
    /* set_volume(255, 0); */

    log_debug("OK: %s, %s\n", digi_driver->name, digi_driver->desc);
    return 0;
}

/* (`allegro_init()' is already defined by Allegro itself.)  */
static int allegro_init_sound(const char *param, int *speed,
                              int *fragsize, int *fragnr, int *channels)
{
    unsigned int i;

    /* No stereo capability. */
    *channels = 1;

    if (allegro_startup(*speed) < 0) {
        return 1;
    }

    fragment_size = *fragsize * sizeof(SWORD);

    buffer_len = fragment_size * *fragnr;
    buffer = create_sample(16, 0, *speed, buffer_len / sizeof(SWORD));

    for (i = 0; i < buffer_len / 2; i++) {
        *((WORD *)buffer->data + i) = 0x8000;
    }

    voice = allocate_voice(buffer);
    if (voice < 0) {
        log_debug("Cannot allocate Allegro voice!\n");
        destroy_sample(buffer);
        return 1;
    }

    buffer_offset = 0;
    been_suspended = 1;
    written_samples = 0;

    voice_set_playmode(voice, PLAYMODE_LOOP);
    voice_set_volume(voice, 255);
    voice_set_pan(voice, 128);

    return 0;
}

static int allegro_write(SWORD *pbuf, size_t nr)
{
    static int counter;
    unsigned int i, count;
    /*unsigned int write_size;*/

    counter++;

    /* XXX: Assumes `nr' is multiple of `fragment_size'.  This is always the
       case with the current implementation.  */
    count = nr / (fragment_size / sizeof(SWORD));

    /* Write one fragment at a time.  FIXME: This could be faster.  */
    for (i = 0; i < count; i++, pbuf += fragment_size / sizeof(SWORD)) {
        if (!been_suspended) {
            unsigned int write_end;

            /* XXX: We do not use module here because we assume we always write
               full fragments.  */
            write_end = buffer_offset + fragment_size - 1;

            /* Block if we are at the position the soundcard is playing.
               Notice that we also assume that the part of the buffer we are
               going to lock is small enough to fit in the safe space.  */
            while (1) {
                unsigned int pos = sizeof(SWORD) * voice_get_position(voice);
                unsigned int pos2 = pos + fragment_size;

                if (pos2 < buffer_len) {
                    if (buffer_offset >= pos2 || write_end < pos) {
                        break;
                    }
                } else {
                    pos2 -= buffer_len;
                    if (write_end < pos && buffer_offset >= pos2) {
                        break;
                    }
                }
            }
        }

        /* Write fragment.  */
        {
            unsigned int j;
            WORD *p = (WORD *)(buffer->data + buffer_offset);

            /* XXX: Maybe the SID engine could already produce samples in
               unsigned format as we need them here?  */
            for (j = 0; j < fragment_size / sizeof(SWORD); j++) {
                p[j] = pbuf[j] + 0x8000;
            }
        }

        buffer_offset += fragment_size;
        if (buffer_offset >= buffer_len) {
            buffer_offset = 0;
        }

        if (been_suspended) {
            been_suspended = 0;
            voice_set_position(voice, 0);
            voice_start(voice);
        }
    }

    written_samples += nr;
    if (written_samples > buffer_len) {
        written_samples = buffer_len;
    }

    return 0;
}

static int allegro_bufferspace(void)
{
    int ret, pos;

    /* voice_get_position returns current position in samples. */
    pos = voice_get_position(voice) * sizeof(SWORD);
    ret = buffer_offset - pos;
    if (ret < 0) {
        ret += buffer_len;
    }

    ret /= sizeof(SWORD);

    if (ret > (int)written_samples) {
        ret = written_samples;
    }

    return buffer_len / sizeof(SWORD) - ret;
}

static void allegro_close(void)
{
    voice_stop(voice);
    deallocate_voice(voice);
    destroy_sample(buffer);
}

static int allegro_suspend(void)
{
    voice_stop(voice);
    been_suspended = 1;
    return 0;
}

static int allegro_resume(void)
{
    buffer_offset = 0;
    written_samples = 0;
    return 0;
}

static sound_device_t allegro_device =
{
    "allegro",
    allegro_init_sound,
    allegro_write,
    NULL,
    NULL,
    allegro_bufferspace,
    allegro_close,
    allegro_suspend,
    allegro_resume,
    1,
    1
};

int sound_init_allegro_device(void)
{
    log_debug("Initializing Allegro sound device.\n");
    return sound_register_device(&allegro_device);
}
