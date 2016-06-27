/*
 * soundsdl.c - Implementation of the Simple Directmedia Layer sound device
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Daniel Aarno <macbishop@users.sourceforge.net>
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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

#include "vice_sdl.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "lib.h"
#include "sound.h"

#ifdef ANDROID_COMPILE
#include "loader.h"
#endif

static SWORD *sdl_buf = NULL;
static SDL_AudioSpec sdl_spec;
static volatile int sdl_inptr = 0;
static volatile int sdl_outptr = 0;
static volatile int sdl_full = 0;
static int sdl_len = 0;

static void sdl_callback(void *userdata, Uint8 *stream, int len)
{
    int amount, total;
    total = 0;

#ifdef ANDROID_COMPILE
    if ((!sdl_full) && (sdl_inptr == sdl_outptr)) {
        if (userdata) {
            *(short *)userdata = 0;
        }
        return;
	}
#endif

    while (total < (int)(len / sizeof(SWORD))) {
        amount = sdl_inptr - sdl_outptr;
        if (amount <= 0) {
            amount = sdl_len - sdl_outptr;
        }

        if (amount + total > (int)(len / sizeof(SWORD))) {
            amount = len / sizeof(SWORD) - total;
        }

        sdl_full = 0;

        if (!amount) {
            memset(stream + total * sizeof(SWORD), 0, len - total * sizeof(SWORD));
#ifdef ANDROID_COMPILE
            if (userdata) {
                *(short *)userdata = len / sizeof(SWORD);
            }
#endif
            return;
        }

        memcpy(stream + total * sizeof(SWORD), sdl_buf + sdl_outptr, amount * sizeof(SWORD));
        total += amount;
        sdl_outptr += amount;

        if (sdl_outptr == sdl_len) {
            sdl_outptr = 0;
        }
    }
#ifdef ANDROID_COMPILE
    if (userdata) {
        *(short *)userdata = total;
    }
#endif
}

static int sdl_init(const char *param, int *speed,
                    int *fragsize, int *fragnr, int *channels)
{
    SDL_AudioSpec spec;
    int nr;

    memset(&spec, 0, sizeof(spec));
    spec.freq = *speed;
    spec.format = AUDIO_S16;
    spec.channels = *channels;
    spec.samples = *fragsize * 2;
    spec.callback = sdl_callback;

    /* NOTE: on some backends the first (input/desired) spec passed to SDL_OpenAudio
     *       may also get modified! because of this we can not use the spec struct
     *       later to retrieve the original desired values.
     * 
     *       also apparently when the backend is pulseaudio, the number of samples
     *       will ALWAYS get divided by two for some reason - using larger buffers
     *       in the config may or may not be needed in that case.
     * 
     *       see eg http://forums.libsdl.org/viewtopic.php?t=9248&sid=92130a5b4cfd7fd713e076e122d7e2a1
     *       to get an idea of the whole mess
     */
    if (SDL_OpenAudio(&spec, &sdl_spec)) {
        return 1;
    }

    if ((sdl_spec.format != AUDIO_S16 && sdl_spec.format != AUDIO_S16MSB) || sdl_spec.channels != *channels) {
        SDL_CloseAudio();
        return 1;
    }

    /* recalculate the number of fragments since the frag size might
     * have changed and we want to keep approximately the same
     * buffersize */
    nr = ((*fragnr) * (*fragsize)) / sdl_spec.samples;

    sdl_len = sdl_spec.samples * nr;
    sdl_inptr = sdl_outptr = sdl_full = 0;
    sdl_buf = lib_calloc(sdl_len, sizeof(SWORD));

    if (!sdl_buf) {
        SDL_CloseAudio();
        return 1;
    }

    *speed = sdl_spec.freq;
    *fragsize = sdl_spec.samples;
    *fragnr = nr;
    SDL_PauseAudio(0);
    return 0;
}

#if defined(WORDS_BIGENDIAN) && (!defined(HAVE_SWAB) || defined(__BEOS__))
#if !defined(AMIGA_MORPHOS) && !defined(AMIGA_M68K)
void swab(void *src, void *dst, size_t length)
{
    const char *from = src;
    char *to = dst;
    size_t ptr;

    for (ptr = 1; ptr < length; ptr += 2) {
        char p = from[ptr];
        char q = from[ptr - 1];
        to[ptr - 1] = p;
        to[ptr] = q;
    }

    if (ptr == length) {
        to[ptr - 1] = 0;
    }
}
#else
#define swab(src, dst, length)                    \
    do {                                          \
        const char *from = src;                   \
        char *to = dst;                           \
        size_t ptr;                               \
                                                  \
        for (ptr = 1; ptr < (length); ptr += 2) { \
            char p = from[ptr];                   \
            char q = from[ptr - 1];               \
            to[ptr - 1] = p;                      \
            to[ptr] = q;                          \
        }                                         \
        if (ptr == (length)) {                    \
            to[ptr - 1] = 0;                      \
        }                                         \
    } while (0)
#endif
#endif

#ifdef ANDROID_COMPILE
void loader_writebuffer()
{
    int total;

    for(;;) {
        int old_sdl_outptr = sdl_outptr;

        total = sdl_inptr - sdl_outptr;
        if (total <= 0) {
            total = sdl_len - sdl_outptr + sdl_inptr;
        }
        if (total > (sdl_spec.samples << 1)) {
            Android_AudioWriteBuffer();
        } else {
            break;
        }

        if (sdl_outptr == old_sdl_outptr) {
            break;
        }
    };
}
#endif

static int sdl_write(SWORD *pbuf, size_t nr)
{
    int total, amount;
    total = 0;

#ifdef WORDS_BIGENDIAN
    if (sdl_spec.format != AUDIO_S16MSB) {
        /* Swap bytes if we're on a big-endian machine, like the Macintosh */
        swab(pbuf, pbuf, sizeof(SWORD) * nr);
    }
#endif

    while (total < (int)nr) {
        amount = sdl_outptr - sdl_inptr;

        if (amount <= 0) {
            amount = sdl_len - sdl_inptr;
        }

        if (total + amount > (int)nr) {
            amount = nr - total;
        }

        if (amount <= 0) {
            SDL_Delay(5);
            continue;
        }

        memcpy(sdl_buf + sdl_inptr, pbuf + total, amount * sizeof(SWORD));
        sdl_inptr += amount;
        total += amount;

        if (sdl_inptr == sdl_len) {
            sdl_inptr = 0;
        }
    }

    if (sdl_inptr == sdl_outptr) {
        sdl_full = 1;
    }

    return 0;
}

static int sdl_bufferspace(void)
{
    int amount;

    if (sdl_full) {
        amount = sdl_len;
    } else {
        amount = sdl_inptr - sdl_outptr;
    }

    if (amount < 0) {
        amount += sdl_len;
    }

    return sdl_len - amount;
}

static void sdl_close(void)
{
    SDL_CloseAudio();
    lib_free(sdl_buf);
    sdl_buf = NULL;
    sdl_inptr = sdl_outptr = sdl_len = sdl_full = 0;
}

static int sdl_suspend(void)
{
    SDL_PauseAudio(1);
    sdl_full = 0;
    return 0;
}

static int sdl_resume(void)
{
    SDL_PauseAudio(0);
    return 0;
}

static sound_device_t sdl_device =
{
    "sdl",
    sdl_init,
    sdl_write,
    NULL,
    NULL,
    sdl_bufferspace,
    sdl_close,
    sdl_suspend,
    sdl_resume,
    1,
    2
};

int sound_init_sdl_device(void)
{
    return sound_register_device(&sdl_device);
}
