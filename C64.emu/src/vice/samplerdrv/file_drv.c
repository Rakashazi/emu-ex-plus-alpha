/*
 * file_drv.c - File based audio input driver.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

/* WAV files tested and working:
 - 22050 Hz, 8bit PCM, stereo
 - 11025 Hz, 16bit PCM, mono
 - 22050 Hz, 16bit PCM, mono
 - 23456 Hz, 16bit PCM, mono
 - 44100 Hz, 16bit PCM, mono
 - 11025 Hz, 16bit PCM, stereo
 - 22050 Hz, 16bit PCM, stereo
 - 23456 Hz, 16bit PCM, stereo
 - 44100 Hz, 16bit PCM, stereo
 - 22050 Hz, 24bit PCM, stereo
 - 22050 Hz, 32bit PCM, stereo
 - 22050 Hz, 32bit float, stereo
 - 22050 Hz, 64bit float, stereo
 - 44010 Hz, a-law, stereo
 - 44010 Hz, u-law, stereo
 */

/* VOC files tested and working:
 - 22050 Hz, 8bit PCM, stereo
 - 22050 Hz, 16bit PCM, mono
 - 11025 Hz, 16bit PCM, stereo
 - 22050 Hz, 16bit PCM, stereo
 - 23456 Hz, 16bit PCM, stereo
 - 44100 Hz, 16bit PCM, stereo
 - 22050 Hz, a-law, stereo
 - 22050 Hz, u-law, stereo
 */

/* IFF files tested and working:
 - 22050 Hz, 8bit PCM, mono
 - 11025 Hz, 8bit PCM, stereo
 - 22050 Hz, 8bit PCM, stereo
 - 23456 Hz, 8bit PCM, stereo
 - 44100 Hz, 8bit PCM, stereo
 */

/* AIFF files tested and working:
 - 22050 Hz, 8bit PCM, stereo
 - 22050 Hz, 16bit PCM, mono
 - 11025 Hz, 16bit PCM, stereo
 - 22050 Hz, 16bit PCM, stereo
 - 44100 Hz, 16bit PCM, stereo
 - 22050 Hz, 24bit PCM, stereo
 - 22050 Hz, 32bit PCM, stereo
 */

/* AIFC files tested and working:
 - 22050 Hz, 8bit PCM, stereo
 - 22050 Hz, 32bit float, stereo
 - 22050 Hz, 64bit float, stereo
 - 22050 Hz, a-law, stereo
 - 22050 Hz, u-law, stereo
 */

#include "vice.h"

/* #define DEBUG_FILEDRV 1 */

#include <string.h> /* for memcpy */
#include <math.h>

#ifdef USE_MPG123
#include <mpg123.h>
#endif

#ifdef USE_FLAC
#include <FLAC/stream_decoder.h>
#endif

#ifdef USE_VORBIS
#include <vorbis/vorbisfile.h>
#endif

#include "types.h"

#include "cmdline.h"
#include "file_drv.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "sampler.h"
#include "translate.h"
#include "util.h"

static log_t filedrv_log = LOG_ERR;

static char *sample_name = NULL;

/* In the future the filename can be set from either commandline or gui */
#define SAMPLE_NAME "inputsound"

#define AUDIO_TYPE_UNKNOWN  -1
#define AUDIO_TYPE_PCM       0
#define AUDIO_TYPE_FLOAT     1
#define AUDIO_TYPE_ALAW      2
#define AUDIO_TYPE_ULAW      3
#define AUDIO_TYPE_PCM_AMIGA 4
#define AUDIO_TYPE_PCM_BE    5
#define AUDIO_TYPE_FLOAT_BE  6

static unsigned sample_size = 0;
static int sound_sampling_started = 0;

static unsigned int sound_sample_frame_start;
static unsigned int old_frame;
static unsigned int sound_frames_per_sec;
static unsigned int sound_cycles_per_frame;
static unsigned int sound_samples_per_frame;

static int current_channels = 0;

static BYTE *file_buffer = NULL;
static unsigned int file_pointer = 0;
static unsigned int file_size = 0;

static int sound_audio_type = 0;
static unsigned int sound_audio_channels = 0;
static unsigned int sound_audio_rate = 0;
static unsigned int sound_audio_bits = 0;

/* ---------------------------------------------------------------------- */

static int wav_fmt_extension_bytes = 0;

static BYTE *sample_buffer1 = NULL;
static BYTE *sample_buffer2 = NULL;

static SWORD decode_ulaw(BYTE sample)
{
    SWORD t;

    sample = ~sample;

    t = ((sample & 0xf) << 3) + 0x84;
    t <<= ((unsigned)sample & 0x70) >> 4;

    return ((sample & 0x80) ? (0x84 - t) : (t - 0x84));
}

static SWORD decode_alaw(BYTE sample)
{
    SWORD t;
    SWORD seg;

    sample ^= 0x55;

    t = (sample & 0xf) << 4;
    seg = ((unsigned)sample & 0x70) >> 4;
    switch (seg) {
        case 0:
            t += 8;
            break;
        case 1:
            t += 0x108;
            break;
        default:
            t += 0x108;
            t <<= seg - 1;
    }
    return ((sample & 0x80) ? t : -t);
}

static int convert_alaw_buffer(int size, int channels)
{
    unsigned int frame_size = sound_audio_bits * sound_audio_channels / 8;
    unsigned int i;
    signed char sample;

    sample_size = size / frame_size;

    sample_buffer1 = lib_malloc(sample_size);
    if (channels == SAMPLER_OPEN_STEREO) {
        if (sound_audio_channels == 2) {
            sample_buffer2 = lib_malloc(sample_size);
        } else {
            sample_buffer2 = sample_buffer1;
        }
    }

    for (i = 0; i < sample_size; ++i) {
        sample = file_buffer[file_pointer + (i * frame_size)];
        sample_buffer1[i] = (BYTE)(decode_alaw(sample) >> 8) + 0x80;
        if (sound_audio_channels == 2 && channels == SAMPLER_OPEN_STEREO) {
            sample = file_buffer[file_pointer + (i * frame_size) + 1];
            sample_buffer2[i] = (BYTE)(decode_alaw(sample) >> 4) + 0x80;
        }
    }
    lib_free(file_buffer);
    file_buffer = NULL;
    return 0;
}

static int convert_ulaw_buffer(int size, int channels)
{
    unsigned int frame_size = sound_audio_bits * sound_audio_channels / 8;
    unsigned int i;
    signed char sample;

    sample_size = size / frame_size;

    sample_buffer1 = lib_malloc(sample_size);
    if (channels == SAMPLER_OPEN_STEREO) {
        if (sound_audio_channels == 2) {
            sample_buffer2 = lib_malloc(sample_size);
        } else {
            sample_buffer2 = sample_buffer1;
        }
    }

    for (i = 0; i < sample_size; ++i) {
        sample = file_buffer[file_pointer + (i * frame_size)];
        sample_buffer1[i] = (BYTE)(decode_ulaw(sample) >> 8) + 0x80;
        if (sound_audio_channels == 2 && channels == SAMPLER_OPEN_STEREO) {
            sample = file_buffer[file_pointer + (i * frame_size) + 1];
            sample_buffer2[i] = (BYTE)(decode_ulaw(sample) >> 3) + 0x80;
        }
    }
    lib_free(file_buffer);
    file_buffer = NULL;
    return 0;
}

static int convert_pcm_buffer(int size, int channels)
{
    unsigned int frame_size = sound_audio_bits * sound_audio_channels / 8;
    unsigned int i;

    sample_size = size / frame_size;

    sample_buffer1 = lib_malloc(sample_size);
    if (channels == SAMPLER_OPEN_STEREO) {
        if (sound_audio_channels == 2) {
            sample_buffer2 = lib_malloc(sample_size);
        } else {
            sample_buffer2 = sample_buffer1;
        }
    }

    for (i = 0; i < sample_size; ++i) {
        if (sound_audio_type == AUDIO_TYPE_PCM_BE) {
            sample_buffer1[i] = file_buffer[file_pointer + (i * frame_size)];
        } else {
            sample_buffer1[i] = file_buffer[file_pointer + (i * frame_size) + (sound_audio_bits / 8) - 1];
        }
        if (sound_audio_bits != 8 || sound_audio_type == AUDIO_TYPE_PCM_AMIGA || sound_audio_type == AUDIO_TYPE_PCM_BE) {
            sample_buffer1[i] += 0x80;
        }
        if (sound_audio_channels == 2 && channels == SAMPLER_OPEN_STEREO) {
            if (sound_audio_type == AUDIO_TYPE_PCM_BE) {
                sample_buffer2[i] = file_buffer[file_pointer + (i * frame_size) + (frame_size / 2)];
            } else {
                sample_buffer2[i] = file_buffer[file_pointer + (i * frame_size) + (frame_size / 2) + (sound_audio_bits / 8) - 1];
            }
            if (sound_audio_bits != 8 || sound_audio_type == AUDIO_TYPE_PCM_AMIGA || sound_audio_type == AUDIO_TYPE_PCM_BE) {
                sample_buffer2[i] += 0x80;
            }
        }
    }
    lib_free(file_buffer);
    file_buffer = NULL;
    return 0;
}

/* FIXME: endianess */
static int convert_float_buffer(int size, int channels)
{
    unsigned int frame_size = sound_audio_bits * sound_audio_channels / 8;
    unsigned int i;
    unsigned char c[sizeof(float)];
    float f;
    SDWORD sample;

    sample_size = size / frame_size;

    sample_buffer1 = lib_malloc(sample_size);
    if (channels == SAMPLER_OPEN_STEREO) {
        if (sound_audio_channels == 2) {
            sample_buffer2 = lib_malloc(sample_size);
        } else {
            sample_buffer2 = sample_buffer1;
        }
    }

    for (i = 0; i < sample_size; ++i) {
        if (sound_audio_type == AUDIO_TYPE_FLOAT_BE) {
            c[3] = file_buffer[file_pointer + (i * frame_size)];
            c[2] = file_buffer[file_pointer + (i * frame_size) + 1];
            c[1] = file_buffer[file_pointer + (i * frame_size) + 2];
            c[0] = file_buffer[file_pointer + (i * frame_size) + 3];
        } else {
            c[0] = file_buffer[file_pointer + (i * frame_size)];
            c[1] = file_buffer[file_pointer + (i * frame_size) + 1];
            c[2] = file_buffer[file_pointer + (i * frame_size) + 2];
            c[3] = file_buffer[file_pointer + (i * frame_size) + 3];
        }
        memcpy(&f, c, sizeof(float));
        f *= 0x7fffffff;
        sample = (SDWORD)f;
        sample_buffer1[i] = (BYTE)((sample >> 24) + 0x80);
        if (sound_audio_channels == 2 && channels == SAMPLER_OPEN_STEREO) {
            if (sound_audio_type == AUDIO_TYPE_FLOAT_BE) {
                c[3] = file_buffer[file_pointer + (i * frame_size) + 4];
                c[2] = file_buffer[file_pointer + (i * frame_size) + 5];
                c[1] = file_buffer[file_pointer + (i * frame_size) + 6];
                c[0] = file_buffer[file_pointer + (i * frame_size) + 7];
            } else {
                c[0] = file_buffer[file_pointer + (i * frame_size) + 4];
                c[1] = file_buffer[file_pointer + (i * frame_size) + 5];
                c[2] = file_buffer[file_pointer + (i * frame_size) + 6];
                c[3] = file_buffer[file_pointer + (i * frame_size) + 7];
            }
            memcpy(&f, c, sizeof(float));
            f *= 0x7fffffff;
            sample = (SDWORD)f;
            sample_buffer2[i] = (BYTE)((sample >> 24) + 0x80);
        }
    }
    lib_free(file_buffer);
    file_buffer = NULL;
    return 0;
}

/* FIXME: endianess */
static int convert_double_buffer(int size, int channels)
{
    unsigned int frame_size = sound_audio_bits * sound_audio_channels / 8;
    unsigned int i;
    unsigned char c[sizeof(double)];
    double f;
    SDWORD sample;

    sample_size = size / frame_size;

    sample_buffer1 = lib_malloc(sample_size);
    if (channels == SAMPLER_OPEN_STEREO) {
        if (sound_audio_channels == 2) {
            sample_buffer2 = lib_malloc(sample_size);
        } else {
            sample_buffer2 = sample_buffer1;
        }
    }

    for (i = 0; i < sample_size; ++i) {
        if (sound_audio_type == AUDIO_TYPE_FLOAT_BE) {
            c[7] = file_buffer[file_pointer + (i * frame_size)];
            c[6] = file_buffer[file_pointer + (i * frame_size) + 1];
            c[5] = file_buffer[file_pointer + (i * frame_size) + 2];
            c[4] = file_buffer[file_pointer + (i * frame_size) + 3];
            c[3] = file_buffer[file_pointer + (i * frame_size) + 4];
            c[2] = file_buffer[file_pointer + (i * frame_size) + 5];
            c[1] = file_buffer[file_pointer + (i * frame_size) + 6];
            c[0] = file_buffer[file_pointer + (i * frame_size) + 7];
        } else {
            c[0] = file_buffer[file_pointer + (i * frame_size)];
            c[1] = file_buffer[file_pointer + (i * frame_size) + 1];
            c[2] = file_buffer[file_pointer + (i * frame_size) + 2];
            c[3] = file_buffer[file_pointer + (i * frame_size) + 3];
            c[4] = file_buffer[file_pointer + (i * frame_size) + 4];
            c[5] = file_buffer[file_pointer + (i * frame_size) + 5];
            c[6] = file_buffer[file_pointer + (i * frame_size) + 6];
            c[7] = file_buffer[file_pointer + (i * frame_size) + 7];
        }
        memcpy(&f, c, sizeof(double));
        f *= 0x7fffffff;
        sample = (SDWORD)f;
        sample_buffer1[i] = (BYTE)((sample >> 24) + 0x80);
        if (sound_audio_channels == 2 && channels == SAMPLER_OPEN_STEREO) {
            if (sound_audio_type == AUDIO_TYPE_FLOAT_BE) {
                c[7] = file_buffer[file_pointer + (i * frame_size) + 8];
                c[6] = file_buffer[file_pointer + (i * frame_size) + 9];
                c[5] = file_buffer[file_pointer + (i * frame_size) + 10];
                c[4] = file_buffer[file_pointer + (i * frame_size) + 11];
                c[3] = file_buffer[file_pointer + (i * frame_size) + 12];
                c[2] = file_buffer[file_pointer + (i * frame_size) + 13];
                c[1] = file_buffer[file_pointer + (i * frame_size) + 14];
                c[0] = file_buffer[file_pointer + (i * frame_size) + 15];
            } else {
                c[0] = file_buffer[file_pointer + (i * frame_size) + 8];
                c[1] = file_buffer[file_pointer + (i * frame_size) + 9];
                c[2] = file_buffer[file_pointer + (i * frame_size) + 10];
                c[3] = file_buffer[file_pointer + (i * frame_size) + 11];
                c[4] = file_buffer[file_pointer + (i * frame_size) + 12];
                c[5] = file_buffer[file_pointer + (i * frame_size) + 13];
                c[6] = file_buffer[file_pointer + (i * frame_size) + 14];
                c[7] = file_buffer[file_pointer + (i * frame_size) + 15];
            }
            memcpy(&f, c, sizeof(double));
            f *= 0x7fffffff;
            sample = (SDWORD)f;
            sample_buffer2[i] = (BYTE)((sample >> 24) + 0x80);
        }
    }
    lib_free(file_buffer);
    file_buffer = NULL;
    return 0;
}

/* ---------------------------------------------------------------------- */

static void check_and_skip_chunk(void)
{
    unsigned int size = 0;
    int skip_chunk = 0;

    /* if the current chunk is of type 'LIST' we need to skip it */
    if (file_buffer[file_pointer] == 0x4C && file_buffer[file_pointer + 1] == 0x49 && file_buffer[file_pointer + 2] == 0x53 && file_buffer[file_pointer + 3] == 0x54) {
        skip_chunk = 1;
#if DEBUG_FILEDRV
        log_warning(filedrv_log, "LIST chunk found, skipping");
#endif
    }

    /* if the current chunk is of type 'PEAK' we need to skip it */
    if (file_buffer[file_pointer] == 0x50 && file_buffer[file_pointer + 1] == 0x45 && file_buffer[file_pointer + 2] == 0x41 && file_buffer[file_pointer + 3] == 0x4B) {
        skip_chunk = 1;
#if DEBUG_FILEDRV
        log_warning(filedrv_log, "PEAK chunk found, skipping");
#endif
    }

    /* if the current chunk is of type 'fact' we need to skip it */
    if (file_buffer[file_pointer] == 0x66 && file_buffer[file_pointer + 1] == 0x61 && file_buffer[file_pointer + 2] == 0x63 && file_buffer[file_pointer + 3] == 0x74) {
        skip_chunk = 1;
#if DEBUG_FILEDRV
        log_warning(filedrv_log, "fact chunk found, skipping");
#endif
    }

    if (skip_chunk) {
        file_pointer += 4;
        size = (file_buffer[file_pointer + 3] << 24) | (file_buffer[file_pointer + 2] << 16) | (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
        file_pointer += size + 4;
    }
}

static int valid_wav_ftm_chunk_size(void)
{
    if (file_buffer[file_pointer] == 0x10 && file_buffer[file_pointer + 1] == 0 && file_buffer[file_pointer + 2] == 0 && file_buffer[file_pointer + 3] == 0) {
        wav_fmt_extension_bytes = 0;
        return 1;
    }
    if (file_buffer[file_pointer] == 0x12 && file_buffer[file_pointer + 1] == 0 && file_buffer[file_pointer + 2] == 0 && file_buffer[file_pointer + 3] == 0) {
        wav_fmt_extension_bytes = 2;
        return 1;
    }
    if (file_buffer[file_pointer] == 0x28 && file_buffer[file_pointer + 1] == 0 && file_buffer[file_pointer + 2] == 0 && file_buffer[file_pointer + 3] == 0) {
        wav_fmt_extension_bytes = 24;
        return 1;
    }
    return 0;
}

static int handle_wav_file(int channels)
{
    unsigned int size = 0;
    unsigned int bps = 0;

    if (file_size < 8) {
        log_error(filedrv_log, "file size smaller than 8 bytes.");
        return -1;
    }

    /* sanity check header indicated size with loaded size */
    size = (file_buffer[7] << 24) | (file_buffer[6] << 16) | (file_buffer[5] << 8) | file_buffer[4];
    if (size != file_size - 8) {
        log_error(filedrv_log, "header reported size not what was expected: header says: %d, filesize - 8 is %d.", size, file_size - 8);
        return -1;
    }

    /* next needs to be 'WAVE' */
    if (file_buffer[8] != 0x57 || file_buffer[9] != 0x41 || file_buffer[10] != 0x56 || file_buffer[11] != 0x45) {
        log_error(filedrv_log, "WAVE not found at expected header position, found: %X %X %X %X.", file_buffer[8], file_buffer[9], file_buffer[10], file_buffer[11]);
        return -1;
    }

    file_pointer = 12;

    check_and_skip_chunk();

    /* current chunk now needs to be 'fmt ' */
    if (file_buffer[file_pointer] != 0x66 || file_buffer[file_pointer + 1] != 0x6D || file_buffer[file_pointer + 2] != 0x74 || file_buffer[file_pointer + 3] != 0x20) {
        log_error(filedrv_log, "'fmt ' chunk not found in the expected header position, %X %X %X %X", file_buffer[file_pointer], file_buffer[file_pointer + 1], file_buffer[file_pointer + 2], file_buffer[file_pointer + 3]);
        return -1;
    }
    file_pointer += 4;

    /* chunk size needs to be 0x10 */
    if (!valid_wav_ftm_chunk_size()) {
        log_error(filedrv_log, "unexpected chunk size %2X%2X%2X%2X", file_buffer[file_pointer + 3], file_buffer[file_pointer + 2], file_buffer[file_pointer + 1], file_buffer[file_pointer]);
        return -1;
    }
    file_pointer += 4;

    /* get the audio format 1: PCM (8/16/24/32bit), 3: float (32/64bit), 6: Alaw, 7: Ulaw */
    if (file_buffer[file_pointer] == 1 && file_buffer[file_pointer + 1] == 0) {
        sound_audio_type = AUDIO_TYPE_PCM;
    } else if (file_buffer[file_pointer] == 3 && file_buffer[file_pointer + 1] == 0) {
        sound_audio_type = AUDIO_TYPE_FLOAT;
    } else if (file_buffer[file_pointer] == 6 && file_buffer[file_pointer + 1] == 0) {
        sound_audio_type = AUDIO_TYPE_ALAW;
    } else if (file_buffer[file_pointer] == 7 && file_buffer[file_pointer + 1] == 0) {
        sound_audio_type = AUDIO_TYPE_ULAW;
    } else {
        log_error(filedrv_log, "unexpected audio format : %2X%2X", file_buffer[file_pointer + 1], file_buffer[file_pointer]);
        return -1;
    }
    file_pointer += 2;

    /* channels used in the file */
    sound_audio_channels = (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    if (sound_audio_channels == 0 || sound_audio_channels > 2) {
        log_error(filedrv_log, "unexpected amount of audio channels : %d", sound_audio_channels);
        return -1;
    }
    file_pointer +=2;

    /* sample rate used in file */
    sound_audio_rate = (file_buffer[file_pointer + 3] << 24) | (file_buffer[file_pointer + 2] << 16) | (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    if (sound_audio_rate == 0) {
        log_error(filedrv_log, "audio rate is 0");
        return -1;
    }
    file_pointer += 4;

    /* get 1st instance of bits per sample */
    bps = (file_buffer[file_pointer + 3] << 24) | (file_buffer[file_pointer + 2] << 16) | (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    sound_audio_bits = (bps / (sound_audio_rate * sound_audio_channels)) * 8;
    file_pointer += 4;

    /* get 2nd instance of bits per sample */
    bps = (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    bps = bps * 8 / sound_audio_channels;
    if (bps != sound_audio_bits) {
        log_error(filedrv_log, "First instance of bps does not match second instance: %d %d", sound_audio_bits, bps);
        return -1;
    }
    file_pointer += 2;

    /* get real instance of bits per sample */
    bps = (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    if (bps != sound_audio_bits) {
        log_error(filedrv_log, "First instance of bps does not match real instance: %d %d", sound_audio_bits, bps);
        return -1;
    }
    file_pointer += 2;

    if (wav_fmt_extension_bytes) {
        file_pointer += wav_fmt_extension_bytes;
    }

    check_and_skip_chunk();
    check_and_skip_chunk();
    check_and_skip_chunk();

    /* current chunk now needs to be 'data' */
    if (file_buffer[file_pointer] != 0x64 || file_buffer[file_pointer + 1] != 0x61 || file_buffer[file_pointer + 2] != 0x74 || file_buffer[file_pointer + 3] != 0x61) {
        log_error(filedrv_log, "data chunk not found at expected header position: %X%X%X%X", file_buffer[file_pointer], file_buffer[file_pointer + 1], file_buffer[file_pointer + 2], file_buffer[file_pointer + 3]);
        return -1;
    }
    file_pointer += 4;

    /* get remaining size */
    size = (file_buffer[file_pointer + 3] << 24) | (file_buffer[file_pointer + 2] << 16) | (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    if (size != file_size - (file_pointer + 4)) {
        log_error(filedrv_log, "data chunk size does not match remaining file size: %d %d", size, file_size - (file_pointer + 4));
        return -1;
    }
    file_pointer += 4;

    switch (sound_audio_type) {
        case AUDIO_TYPE_PCM:
            return convert_pcm_buffer(size, channels);
        case AUDIO_TYPE_FLOAT:
            switch (sound_audio_bits) {
                case 32:
                    return convert_float_buffer(size, channels);
                case 64:
                    return convert_double_buffer(size, channels);
                default:
                    log_error(filedrv_log, "Unhandled float format : %d", sound_audio_bits);
                    return -1;
            }
        case AUDIO_TYPE_ALAW:
            return convert_alaw_buffer(size, channels);
        case AUDIO_TYPE_ULAW:
            return convert_ulaw_buffer(size, channels);
        default:
            log_error(filedrv_log, "unhandled audio type");
            return -1;
    }
    return -1;
}

static int is_wav_file(void)
{
    if (file_size < 4) {
        return 0;
    }

    /* Check for wav header signature */
    if (file_buffer[0] == 0x52 && file_buffer[1] == 0x49 && file_buffer[2] == 0x46 && file_buffer[3] == 0x46) {
        return 1;
    }
    return 0;
}

/* ---------------------------------------------------------------------- */

static BYTE *voc_buffer1 = NULL;
static BYTE *voc_buffer2 = NULL;
static unsigned int voc_buffer_size;

static int voc_handle_sound_1(int channels)
{
    unsigned int size;
    BYTE fd;
    BYTE codec;
    unsigned int rem;

    if (file_pointer + 6 > file_size) {
        log_error(filedrv_log, "Voc file size too small");
        return -1;
    }

    ++file_pointer;

    size = (file_buffer[file_pointer + 2] << 16) | (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    if (file_pointer + size > file_size) {
        log_error(filedrv_log, "Reported voc file size too big: %X %X", file_size, size);
        return -1;
    }
    file_pointer += 3;

    rem = (file_size - file_pointer) % 0x1000000;

    if (rem == size || rem == size - 1 || rem == size + 1) {
        size = (file_size - file_pointer) - 1;
    }

    if (!sound_audio_rate) {
        fd = file_buffer[file_pointer];
        codec = file_buffer[file_pointer + 1];

        switch (codec) {
            case 0:
                sound_audio_type = AUDIO_TYPE_PCM;
                sound_audio_bits = 8;
                break;
            case 4:
                sound_audio_type = AUDIO_TYPE_PCM;
                sound_audio_bits = 16;
                break;
            case 6:
                sound_audio_type = AUDIO_TYPE_ALAW;
                sound_audio_bits = 16;
                break;
            case 7:
                sound_audio_type = AUDIO_TYPE_ULAW;
                sound_audio_bits = 16;
                break;
            default:
                log_error(filedrv_log, "Unhandled voc file codec: %X", codec);
                return -1;
                break;
        }
        sound_audio_rate = 1000000 / (256 - fd);
    }
    file_pointer += 2;
    size -= 2;

    if (voc_buffer1) {
        lib_free(voc_buffer1);
        voc_buffer1 = NULL;
        return -1;
    }

    voc_buffer1 = lib_malloc(size);
    memcpy(voc_buffer1, file_buffer + file_pointer, size);
    file_pointer += size;
    voc_buffer_size = size;

    return 0;
}

static int voc_handle_sound_2(int channels)
{
    unsigned int size;

    if (file_pointer + 6 > file_size) {
        log_error(filedrv_log, "Voc file too small");
        return -1;
    }

    ++file_pointer;

    size = (file_buffer[file_pointer + 2] << 16) | (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    if (file_pointer + size > file_size) {
        log_error(filedrv_log, "Reported voc sound 2 block size too big: %X %X", size, file_size);
        return -1;
    }
    file_pointer += 5;
    size -= 2;
    
    if (!voc_buffer1) {
        return -1;
    }
    voc_buffer2 = lib_malloc(voc_buffer_size + size);
    memcpy(voc_buffer2, voc_buffer1, voc_buffer_size);
    memcpy(voc_buffer2 + voc_buffer_size, file_buffer + file_pointer, size);
    lib_free(voc_buffer1);
    voc_buffer1 = voc_buffer2;
    voc_buffer2 = NULL;
    voc_buffer_size += size;
    file_pointer += size;

    return 0;
}

static int voc_handle_silence(int channels)
{
    unsigned int size;

    if (file_pointer + 7 > file_size) {
        log_error(filedrv_log, "Voc file too small");
        return -1;
    }

    ++file_pointer;

    size = (file_buffer[file_pointer + 2] << 16) | (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    if (size != 3) {
        log_error(filedrv_log, "Voc silence block size mismatch: %X", size);
        return -1;
    }

    file_pointer += 3;

    size = (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];

    if (sound_audio_type == AUDIO_TYPE_UNKNOWN) {
        log_error(filedrv_log, "Found voc silence block before any information headers");
        return -1;
    }

    if (sound_audio_type == AUDIO_TYPE_PCM) {
        size *= (sound_audio_bits / 8);
    }
    size *= sound_audio_channels;

    voc_buffer2 = lib_malloc(voc_buffer_size + size);

    if (sound_audio_bits == 8) {
        memset(voc_buffer2 + voc_buffer_size, 0x80, size);
    } else {
        memset(voc_buffer2 + voc_buffer_size, 0, size);
    }

    if (voc_buffer1) {
        memcpy(voc_buffer2, voc_buffer1, voc_buffer_size);
        lib_free(voc_buffer1);
    }
    voc_buffer1 = voc_buffer2;
    voc_buffer2 = NULL;
    voc_buffer_size += size;
    file_pointer += 3;
    return 0;
}

static int voc_handle_sound_9(int channels)
{
    unsigned int size;
    WORD codec;
    unsigned int rem;

    if (file_pointer + 16 > file_size) {
        log_error(filedrv_log, "Voc file too small");
        return -1;
    }

    ++file_pointer;

    size = (file_buffer[file_pointer + 2] << 16) | (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    if (size + file_pointer > file_size) {
        log_error(filedrv_log, "Reported voc block 9 size too big: %X %X", size, file_size - file_pointer);
        return -1;
    }

    file_pointer += 3;

    rem = (file_size - file_pointer) % 0x1000000;

    if (rem == size || rem == size - 1 || rem == size + 1) {
        size = (file_size - file_pointer) - 1;
    }

    sound_audio_rate = (file_buffer[file_pointer + 3] << 24) | (file_buffer[file_pointer + 2] << 16) | (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    if (!sound_audio_rate) {
        log_error(filedrv_log, "Reported voc sound block audio rate is 0");
        return -1;
    }

    file_pointer += 4;
    size -= 4;

    sound_audio_bits = file_buffer[file_pointer];

    switch (sound_audio_bits) {
        case 8:
        case 16:
        case 24:
        case 32:
            break;
        default:
            log_error(filedrv_log, "Reported voc bits per sample is not 8, 16, 24 or 32");
            return -1;
    }

    ++file_pointer;
    --size;

    sound_audio_channels = file_buffer[file_pointer];
    if (sound_audio_channels < 1 || sound_audio_channels > 2) {
        log_error(filedrv_log, "Reported voc channels not 1 or 2");
        return -1;
    }

    ++file_pointer;
    --size;

    codec = (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];

    switch (codec) {
        case 0:
        case 4:
            sound_audio_type = AUDIO_TYPE_PCM;
            break;
        case 6:
            sound_audio_type = AUDIO_TYPE_ALAW;
            break;
        case 7:
            sound_audio_type = AUDIO_TYPE_ULAW;
            break;
        default:
            log_error(filedrv_log, "Unknown voc sound block codec");
            return -1;
            break;
    }

    file_pointer += 6;
    size -= 6;

    voc_buffer2 = lib_malloc(voc_buffer_size + size);

    if (voc_buffer1) {
        memcpy(voc_buffer2, voc_buffer1, voc_buffer_size);
        lib_free(voc_buffer1);
    }
    memcpy(voc_buffer2 + voc_buffer_size, file_buffer + file_pointer, size);
    voc_buffer1 = voc_buffer2;
    voc_buffer2 = NULL;
    voc_buffer_size += size;
    file_pointer += size;

    return 0;
}

static int voc_handle_extra_info(int channels)
{
    unsigned int size;
    WORD fd;
    BYTE codec;

    if (file_pointer + 8 > file_size) {
        log_error(filedrv_log, "Voc file too small");
        return -1;
    }

    ++file_pointer;

    size = (file_buffer[file_pointer + 2] << 16) | (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    if (size != 4) {
        log_error(filedrv_log, "Reported voc extra info block size is not 4");
        return -1;
    }
    file_pointer += 3;

    fd = (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    codec = file_buffer[file_pointer + 2];

    switch (codec) {
        case 0:
            sound_audio_type = AUDIO_TYPE_PCM;
            sound_audio_bits = 8;
            break;
        case 4:
            sound_audio_type = AUDIO_TYPE_PCM;
            sound_audio_bits = 16;
            break;
        case 6:
            sound_audio_type = AUDIO_TYPE_ALAW;
            sound_audio_bits = 16;
            break;
        case 7:
            sound_audio_type = AUDIO_TYPE_ULAW;
            sound_audio_bits = 16;
            break;
        default:
            log_error(filedrv_log, "Reported voc extra info block codec is unknown");
            return -1;
            break;
    }
    sound_audio_channels = file_buffer[file_pointer + 3] + 1;
    sound_audio_rate = 256000000 / (sound_audio_channels * (65536 - fd));
    file_pointer += 4;
    return 0;
}

static int voc_handle_text(void)
{
    unsigned int size;

    if (file_pointer + 4 > file_size) {
        log_error(filedrv_log, "Voc file too small");
        return -1;
    }
    ++file_pointer;
    size = (file_buffer[file_pointer + 2] << 16) | (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    file_pointer += 3;

    if (file_pointer + size > file_size) {
        log_error(filedrv_log, "Reported voc text block size too big");
        return -1;
    }
    if (size) {
        file_pointer += size;
    }
    return 0;
}

static int voc_handle_ignore(unsigned int amount)
{
    unsigned int size;

    if (amount + 4 + file_pointer > file_size) {
        log_error(filedrv_log, "Voc file too small");
        return -1;
    }
    ++file_pointer;
    size = (file_buffer[file_pointer + 2] << 16) | (file_buffer[file_pointer + 1] << 8) | file_buffer[file_pointer];
    if (size != amount) {
        log_error(filedrv_log, "Unexpected voc block size: %X", size);
        return -1;
    }
    file_pointer += 3;
    if (amount) {
        file_pointer += amount;
    }
    return 0;
}

static int handle_voc_file(int channels)
{
    WORD version;
    WORD check;
    int end_of_stream = 0;
    int err = 0;

    sound_audio_channels = 0;
    sound_audio_rate = 0;
    sound_audio_bits = 0;
    sound_audio_type = AUDIO_TYPE_UNKNOWN;

    if (file_buffer[19] != 0x1A) {
        log_error(filedrv_log, "Voc file $1A signature not found");
        return -1;
    }

    if (file_buffer[20] != 0x1A || file_buffer[21] != 0) {
        log_error(filedrv_log, "Incorrect voc file header length : %X", (file_buffer[21] << 8) | file_buffer[20]);
        return -1;
    }

    version = (file_buffer[23] << 8) | file_buffer[22];
    check = (file_buffer[25] << 8) | file_buffer[24];

    if (check != ~version + 0x1234) {
        log_error(filedrv_log, "VOC file header checksum incorrect: %4X %4X", check, version);
        return -1;
    }

    file_pointer = 26;

    while (file_pointer <= file_size && !end_of_stream) {
        switch (file_buffer[file_pointer]) {
            case 0:
                end_of_stream = 1;
                break;
            case 1:
                err = voc_handle_sound_1(channels);
                break;
            case 2:
                err = voc_handle_sound_2(channels);
                break;
            case 3:
                err = voc_handle_silence(channels);
                break;
            case 4:
            case 6:
                err = voc_handle_ignore(2);
                break;
            case 5:
                err = voc_handle_text();
                break;
            case 7:
                err = voc_handle_ignore(0);
                break;
            case 8:
                err = voc_handle_extra_info(channels);
                break;
            case 9:
                err = voc_handle_sound_9(channels);
                break;
            default:
                log_error(filedrv_log, "Unknown VOC block type : %2X", file_buffer[file_pointer]);
                return -1;
        }
        if (err) {
            if (voc_buffer1) {
                lib_free(voc_buffer1);
                voc_buffer1 = NULL;
            }
            return -1;
        }
    }

    lib_free(file_buffer);
    file_buffer = NULL;

    file_buffer = voc_buffer1;
    voc_buffer1 = NULL;

    file_pointer = 0;
    file_size = voc_buffer_size;

    switch (sound_audio_type) {
        case AUDIO_TYPE_PCM:
            return convert_pcm_buffer(file_size, channels);
        case AUDIO_TYPE_ALAW:
            return convert_alaw_buffer(file_size, channels);
        case AUDIO_TYPE_ULAW:
            return convert_ulaw_buffer(file_size, channels);
        default:
            log_error(filedrv_log, "unhandled audio type");
            return -1;
    }
    return -1;
}

static int is_voc_file(void)
{
    char header[] = { 0x43, 0x72, 0x65, 0x61, 0x74, 0x69, 0x76, 0x65, 0x20, 0x56, 0x6F, 0x69, 0x63, 0x65, 0x20, 0x46, 0x69, 0x6C, 0x65 };
    size_t i;

    if (file_size < 26) {
        return 0;
    }

    /* Check for voc header signature */
    for (i = 0; i < sizeof(header); ++i) {
        if (file_buffer[i] != header[i]) {
            return 0;
        }
    }
    return 1;
}

/* ---------------------------------------------------------------------- */

static unsigned int iff_samples = 0;

static int iff_handle_chan(void)
{
    DWORD stereo;

    file_pointer += 4;

    if (file_buffer[file_pointer] != 0 || file_buffer[file_pointer + 1] != 0 || file_buffer[file_pointer + 2] != 0 || file_buffer[file_pointer + 3] != 4) {
        log_error(filedrv_log, "Reported iff chan chunk size is not 4");
        return -1;
    }

    file_pointer += 4;

    stereo = (file_buffer[file_pointer] << 24) | (file_buffer[file_pointer + 1] << 16) | (file_buffer[file_pointer + 2] << 8) | file_buffer[file_pointer + 3];

    if (stereo == 6 || stereo == 0x6000000) {
        sound_audio_channels = 2;
    }

    file_pointer += 4;

    return 0;
}

static int iff_handle_body(int channels)
{
    DWORD size;

    file_pointer += 4;

    size = (file_buffer[file_pointer] << 24) | (file_buffer[file_pointer + 1] << 16) | (file_buffer[file_pointer + 2] << 8) | file_buffer[file_pointer + 3];

    if (!size) {
        log_error(filedrv_log, "Reported iff body chunk size is 0");
        return -1;
    }

    file_pointer += 4;

    return convert_pcm_buffer(size, channels);
}

static int handle_iff_file(int channels)
{
    DWORD size;
    DWORD header;
    int body_found = 0;
    int err = 0;

    sound_audio_type = AUDIO_TYPE_PCM_AMIGA;
    sound_audio_channels = 1;
    sound_audio_bits = 8;

    size = (file_buffer[4] << 24) | (file_buffer[5] << 16) | (file_buffer[6] << 8) | file_buffer[7];

    if (size != file_size - 8) {
        log_error(filedrv_log, "Reported iff total size mismatch : %X, %X", (unsigned int)size, file_size - 8);
        return -1;
    }

    size = (file_buffer[16] << 24) | (file_buffer[17] << 16) | (file_buffer[18] << 8) | file_buffer[19];
    if (size != 20) {
        log_error(filedrv_log, "Reported iff header size is not 20");
        return -1;
    }

    iff_samples = (file_buffer[20] << 24) | (file_buffer[21] << 16) | (file_buffer[22] << 8) | file_buffer[23];
    if (!iff_samples) {
        log_error(filedrv_log, "Reported iff amount of samples is 0");
        return -1;
    }

    if (file_buffer[24] != 0 || file_buffer[25] != 0 || file_buffer[26] != 0 || file_buffer[27] != 0) {
        return -1;
    }

    if (file_buffer[28] != 0 || file_buffer[29] != 0 || file_buffer[30] != 0 || file_buffer[31] != 0) {
        return -1;
    }

    sound_audio_rate = (file_buffer[32] << 8) | file_buffer[33];
    if (!sound_audio_rate) {
        log_error(filedrv_log, "Reported iff bitrate  is 0");
        return -1;
    }

    if (file_buffer[34] != 1) {
        return -1;
    }

    if (file_buffer[35]) {
        return -1;
    }

    file_pointer = 40;

    while (file_pointer <= file_size && !body_found) {
        if (file_pointer + 8 > file_size) {
            log_error(filedrv_log, "Iff file too small");
            return -1;
        }

        header = (file_buffer[file_pointer] << 24) | (file_buffer[file_pointer + 1] << 16) | (file_buffer[file_pointer + 2] << 8) | file_buffer[file_pointer + 3];

        switch (header) {
            case 0x424F4459:
                err = iff_handle_body(channels);
                body_found = 1;
                break;
            case 0x4348414E:
                err = iff_handle_chan();
                break;
            default:
                file_pointer += 4;
                size = (file_buffer[file_pointer] << 24) | (file_buffer[file_pointer + 1] << 16) | (file_buffer[file_pointer + 2] << 8) | file_buffer[file_pointer + 3];
                file_pointer += 4;
                if (file_pointer + size > file_size) {
                    log_error(filedrv_log, "Iff file too small");
                    return -1;
                }
                file_pointer += size;
        }
        if (err) {
            return -1;
        }
    }
    if (!body_found) {
        return -1;
    }
    return 0;
}

static int is_iff_file(void)
{
    if (file_size < 16) {
        return 0;
    }

    /* Check for iff header signature */
    if (file_buffer[0] == 0x46 && file_buffer[1] == 0x4F && file_buffer[2] == 0x52 && file_buffer[3] == 0x4D) {
        if (file_buffer[8] == 0x38 && file_buffer[9] == 0x53 && file_buffer[10] == 0x56 && file_buffer[11] == 0x58) {
            if (file_buffer[12] == 0x56 && file_buffer[13] == 0x48 && file_buffer[14] == 0x44 && file_buffer[15] == 0x52) {
                return 1;
            }
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------- */

#define U2F(u) (((double)((long)(u - 2147483647L - 1))) + 2147483648.0)

#ifndef HUGE_VAL
#define HUGE_VAL HUGE
#endif

double float80tofloat64(unsigned char* bytes)
{
    double f;
    int expon;
    unsigned long hiMant, loMant;
    
    expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
    hiMant = ((unsigned long)(bytes[2] & 0xFF) << 24) | ((unsigned long)(bytes[3] & 0xFF) << 16) | ((unsigned long)(bytes[4] & 0xFF) << 8) | ((unsigned long)(bytes[5] & 0xFF));
    loMant = ((unsigned long)(bytes[6] & 0xFF) << 24) | ((unsigned long)(bytes[7] & 0xFF) << 16) | ((unsigned long)(bytes[8] & 0xFF) << 8) | ((unsigned long)(bytes[9] & 0xFF));

    if (!expon && !hiMant && !loMant) {
        f = 0;
    } else {
        if (expon == 0x7FFF) {
            f = HUGE_VAL;
        } else {
            expon -= 16383;
            f  = ldexp(U2F(hiMant), expon -= 31);
            f += ldexp(U2F(loMant), expon -= 32);
        }
    }

    if (bytes[0] & 0x80) {
        return -f;
    }
    return f;
}

static int aiff_handle_ssnd(int channels)
{
    DWORD size;
    int i;

    file_pointer += 4;

    size = (file_buffer[file_pointer] << 24) | (file_buffer[file_pointer + 1] << 16) | (file_buffer[file_pointer + 2] << 8) | file_buffer[file_pointer + 3];
    file_pointer += 4;
    if (file_pointer + size > file_size) {
        log_error(filedrv_log, "SSND chunk bigger than remaining size : %X %X", (unsigned int)size, file_size - file_pointer);
        return -1;
    }

    for (i = 0; i < 8; ++i) {
        if (file_buffer[file_pointer + i]) {
            log_error(filedrv_log, "SSND secondary parameters not 0");
            return -1;
        }
    }

    file_pointer += 8;
    size -= 8;

    return convert_pcm_buffer(size, channels);
}

static int aiff_handle_comm(void)
{
    DWORD size;
    double f64;
    unsigned char f80[10];
    int i;

    file_pointer += 4;

    size = (file_buffer[file_pointer] << 24) | (file_buffer[file_pointer + 1] << 16) | (file_buffer[file_pointer + 2] << 8) | file_buffer[file_pointer + 3];
    if (size != 18) {
        log_error(filedrv_log, "COMM chunk size not 18: %d", (int)size);
        return -1;
    }

    file_pointer += 4;

    sound_audio_channels = (file_buffer[file_pointer] << 8) | file_buffer[file_pointer + 1];
    if (sound_audio_channels < 1 || sound_audio_channels > 2) {
        log_error(filedrv_log, "COMM channels not 1 or 2 : %d", sound_audio_channels);
        return -1;
    }

    file_pointer += 6;

    sound_audio_bits = (file_buffer[file_pointer] << 8) | file_buffer[file_pointer + 1];
    switch (sound_audio_bits) {
        case 8:
        case 16:
        case 24:
        case 32:
            break;
        default:
            log_error(filedrv_log, "COMM bits not 8, 16, 24 or 32 : %d", sound_audio_bits);
            return -1;
    }

    file_pointer += 2;

    for (i = 0; i < 10; ++i) {
        f80[i] = file_buffer[file_pointer + i];  
    }

    f64 = float80tofloat64(f80);
    sound_audio_rate = (unsigned int)f64;
    if (!sound_audio_rate) {
        log_error(filedrv_log, "COMM audio rate is 0");
        return -1;
    }

    file_pointer += 10;
    sound_audio_type = AUDIO_TYPE_PCM_BE;

    return 0;
}

static int handle_aiff_file(int channels)
{
    DWORD size;
    DWORD header;
    int ssnd_found = 0;
    int err = 0;

    size = (file_buffer[4] << 24) | (file_buffer[5] << 16) | (file_buffer[6] << 8) | file_buffer[7];

    if (size != file_size - 8) {
        log_error(filedrv_log, "AIFF size is wrong : %X %X", (unsigned int)size, file_size - 8);
        return -1;
    }

    file_pointer = 12;

    while (file_pointer <= file_size && !ssnd_found) {
        if (file_pointer + 8 > file_size) {
            log_error(filedrv_log, "Aiff file too small");
            return -1;
        }

        header = (file_buffer[file_pointer] << 24) | (file_buffer[file_pointer + 1] << 16) | (file_buffer[file_pointer + 2] << 8) | file_buffer[file_pointer + 3];

        switch (header) {
            case 0x53534E44:
#ifdef DEBUG_FILEDRV
                log_warning(filedrv_log, "handling AIFF SSND chunk");
#endif
                err = aiff_handle_ssnd(channels);
                ssnd_found = 1;
                break;
            case 0x434F4D4D:
#ifdef DEBUG_FILEDRV
                log_warning(filedrv_log, "handling AIFF COMM chunk");
#endif
                err = aiff_handle_comm();
                break;
            default:
                file_pointer += 4;
                size = (file_buffer[file_pointer] << 24) | (file_buffer[file_pointer + 1] << 16) | (file_buffer[file_pointer + 2] << 8) | file_buffer[file_pointer + 3];
                if (size % 2) {
                    ++size;
                }
                file_pointer += 4;
                if (file_pointer + size > file_size) {
                    log_error(filedrv_log, "Aiff file too small");
                    return -1;
                }
                file_pointer += size;
        }
        if (err) {
            return -1;
        }
    }
    if (!ssnd_found) {
        return -1;
    }
    return 0;
}

static int is_aiff_file(void)
{
    if (file_size < 12) {
        return 0;
    }

    /* Check for aiff header signature */
    if (file_buffer[0] == 0x46 && file_buffer[1] == 0x4F && file_buffer[2] == 0x52 && file_buffer[3] == 0x4D) {
        if (file_buffer[8] == 0x41 && file_buffer[9] == 0x49 && file_buffer[10] == 0x46 && file_buffer[11] == 0x46) {
            return 1;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------- */

static int aifc_handle_ssnd(int channels)
{
    DWORD size;
    int i;

    file_pointer += 4;

    size = (file_buffer[file_pointer] << 24) | (file_buffer[file_pointer + 1] << 16) | (file_buffer[file_pointer + 2] << 8) | file_buffer[file_pointer + 3];
    file_pointer += 4;
    if (file_pointer + size > file_size) {
        log_error(filedrv_log, "SSND chunk bigger than remaining size : %X %X", (unsigned int)size, file_size - file_pointer);
        return -1;
    }

    for (i = 0; i < 8; ++i) {
        if (file_buffer[file_pointer + i]) {
            log_error(filedrv_log, "SSND secondary parameters not 0");
            return -1;
        }
    }

    file_pointer += 8;
    size -= 8;

    switch (sound_audio_type) {
        case AUDIO_TYPE_PCM:
            return convert_pcm_buffer(size, channels);
        case AUDIO_TYPE_FLOAT_BE:
            switch (sound_audio_bits) {
                case 32:
                    return convert_float_buffer(size, channels);
                case 64:
                    return convert_double_buffer(size, channels);
                default:
                    log_error(filedrv_log, "Unhandled float format : %d", sound_audio_bits);
                    return -1;
            }
        case AUDIO_TYPE_ALAW:
            return convert_alaw_buffer(size, channels);
        case AUDIO_TYPE_ULAW:
            return convert_ulaw_buffer(size, channels);
        default:
            log_error(filedrv_log, "unhandled audio type");
            return -1;
    }
    return -1;
}

static int aifc_handle_comm(void)
{
    DWORD size;
    DWORD type;
    double f64;
    unsigned char f80[10];
    int i;

    file_pointer += 4;

    size = (file_buffer[file_pointer] << 24) | (file_buffer[file_pointer + 1] << 16) | (file_buffer[file_pointer + 2] << 8) | file_buffer[file_pointer + 3];
    file_pointer += 4;
    if (size + file_pointer > file_size) {
        log_error(filedrv_log, "Aifc file too small");
        return -1;
    }

    sound_audio_channels = (file_buffer[file_pointer] << 8) | file_buffer[file_pointer + 1];
    if (sound_audio_channels < 1 || sound_audio_channels > 2) {
        log_error(filedrv_log, "COMM channels not 1 or 2 : %d", sound_audio_channels);
        return -1;
    }

    file_pointer += 6;
    size -= 6;

    sound_audio_bits = (file_buffer[file_pointer] << 8) | file_buffer[file_pointer + 1];
    switch (sound_audio_bits) {
        case 8:
        case 16:
        case 24:
        case 32:
        case 64:
            break;
        default:
            log_error(filedrv_log, "COMM bits not 8, 16, 24, 32 or 64 : %d", sound_audio_bits);
            return -1;
    }

    file_pointer += 2;
    size -= 2;

    for (i = 0; i < 10; ++i) {
        f80[i] = file_buffer[file_pointer + i];  
    }

    f64 = float80tofloat64(f80);
    sound_audio_rate = (unsigned int)f64;
    if (!sound_audio_rate) {
        log_error(filedrv_log, "COMM audio rate is 0");
        return -1;
    }

    file_pointer += 10;
    size -= 10;

    type = (file_buffer[file_pointer] << 24) | (file_buffer[file_pointer + 1] << 16) | (file_buffer[file_pointer + 2] << 8) | file_buffer[file_pointer + 3];

    switch (type) {
        case 0x616C6177:
            sound_audio_type = AUDIO_TYPE_ALAW;
            break;
        case 0x464C3332:
        case 0x464C3634:
            sound_audio_type = AUDIO_TYPE_FLOAT_BE;
            break;
        case 0x72617720:
            sound_audio_type = AUDIO_TYPE_PCM;
            break;
        case 0x756C6177:
            sound_audio_type = AUDIO_TYPE_ULAW;
            break;
        default:
            log_error(filedrv_log, "Unknown aifc audio type");
            return -1;
    }
    file_pointer += size;

    return 0;
}

static int handle_aifc_file(int channels)
{
    DWORD size;
    DWORD header;
    int ssnd_found = 0;
    int err = 0;

    size = (file_buffer[4] << 24) | (file_buffer[5] << 16) | (file_buffer[6] << 8) | file_buffer[7];

    if (size != file_size - 8) {
        log_error(filedrv_log, "AIFC size is wrong : %X %X", (unsigned int)size, file_size - 8);
        return -1;
    }

    file_pointer = 12;

    while (file_pointer <= file_size && !ssnd_found) {
        if (file_pointer + 8 > file_size) {
            log_error(filedrv_log, "Aifc file too small");
            return -1;
        }

        header = (file_buffer[file_pointer] << 24) | (file_buffer[file_pointer + 1] << 16) | (file_buffer[file_pointer + 2] << 8) | file_buffer[file_pointer + 3];

        switch (header) {
            case 0x53534E44:
#ifdef DEBUG_FILEDRV
                log_warning(filedrv_log, "handling AIFC SSND chunk");
#endif
                err = aifc_handle_ssnd(channels);
                ssnd_found = 1;
                break;
            case 0x434F4D4D:
#ifdef DEBUG_FILEDRV
                log_warning(filedrv_log, "handling AIFC COMM chunk");
#endif
                err = aifc_handle_comm();
                break;
            default:
                file_pointer += 4;
                size = (file_buffer[file_pointer] << 24) | (file_buffer[file_pointer + 1] << 16) | (file_buffer[file_pointer + 2] << 8) | file_buffer[file_pointer + 3];
                if (size % 2) {
                    ++size;
                }
                file_pointer += 4;
                if (file_pointer + size > file_size) {
                    log_error(filedrv_log, "Aifc file too small");
                    return -1;
                }
                file_pointer += size;
        }
        if (err) {
            return -1;
        }
    }
    if (!ssnd_found) {
        return -1;
    }
    return 0;
}

static int is_aifc_file(void)
{
    if (file_size < 12) {
        return 0;
    }

    /* Check for aifc header signature */
    if (file_buffer[0] == 0x46 && file_buffer[1] == 0x4F && file_buffer[2] == 0x52 && file_buffer[3] == 0x4D) {
        if (file_buffer[8] == 0x41 && file_buffer[9] == 0x49 && file_buffer[10] == 0x46 && file_buffer[11] == 0x43) {
            return 1;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------- */

#ifdef USE_MPG123
static int mp3_err = MPG123_OK;
static mpg123_handle *mh = NULL;

static int handle_mp3_file(int channels)
{
    int mp3_channels = 0;
    int mp3_encoding = 0;
    long mp3_rate = 0;
    off_t buffer_size = 0;
    size_t done = 0;

    mp3_err = mpg123_getformat(mh, &mp3_rate, &mp3_channels, &mp3_encoding);
    if (mp3_err != MPG123_OK) {
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        log_error(filedrv_log, "Unknown mp3 format");
        return -1;
    }

    /* Should not happen, but just in case */
    if (mp3_encoding != MPG123_ENC_SIGNED_16) {
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        log_error(filedrv_log, "Mp3 encoding is not signed 16bit");
        return -1;
    }

    /* lock format */
    mpg123_format_none(mh);
    mpg123_format(mh, mp3_rate, mp3_channels, mp3_encoding);

    mpg123_scan(mh);
    buffer_size = mpg123_length(mh);

    lib_free(file_buffer);
    file_size = buffer_size * 2 * mp3_channels;
    file_buffer = lib_malloc(file_size);

    mp3_err = mpg123_read(mh, file_buffer, file_size, &done);

    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();

    file_pointer = 0;

    sound_audio_type = AUDIO_TYPE_PCM;
    sound_audio_channels = mp3_channels;
    sound_audio_rate = mp3_rate;
    sound_audio_bits = 16;

    return convert_pcm_buffer(file_size, channels);
}

static int is_mp3_file(void)
{
    mp3_err = mpg123_init();
    if (mp3_err != MPG123_OK) {
        return 0;
    }
    mh = mpg123_new(NULL, &mp3_err);
    if (!mh) {
        mpg123_exit();
        return 0;
    }
    mp3_err = mpg123_open(mh, sample_name);
    if (mp3_err != MPG123_OK) {
        mpg123_delete(mh);
        mpg123_exit();
        return 0;
    }
    return 1;
}
#endif

/* ---------------------------------------------------------------------- */

#ifdef USE_FLAC
static FLAC__uint64 flac_total_samples = 0;
static unsigned int flac_sample_rate = 0;
static unsigned int flac_channels = 0;
static unsigned int flac_bps = 0;

static FLAC__uint32 flac_total_size = 0;
static BYTE *flac_buffer = NULL;
static unsigned int flac_size = 0;

static void flac_buffer_add(FLAC__uint32 raw)
{
    WORD sample = 0;

    switch (flac_bps) {
        case 8:
            sample = (WORD)((raw & 0xFF) << 8);
            break;
        case 16:
            sample = (WORD)(raw & 0xFFFF);
            break;
        case 24:
            sample = (WORD)((raw & 0xFFFF00) >> 8);
            break;
        case 32:
            sample = (WORD)((raw & 0xFFFF0000) >> 16);
            break;
    }
    if (flac_size + 2 > flac_total_size) {
#ifdef DEBUG_FILEDRV
        log_warning(filedrv_log, "flac buffer overflow");       
#endif
    } else {
        flac_buffer[flac_size + 1] = (BYTE)(sample >> 8);
        flac_buffer[flac_size] = (BYTE)(sample & 0xFF);
        flac_size += 2;
    }
}

static FLAC__StreamDecoderWriteStatus flac_write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
    size_t i;

    if(frame->header.number.sample_number == 0) {
        flac_total_size = (FLAC__uint32)(flac_total_samples * flac_channels * (flac_bps / 8));

        if (!flac_total_samples) {
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }

        if (flac_channels < 1 || flac_channels > 2) {
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }

        switch (flac_bps) {
            case 8:
            case 16:
            case 24:
            case 32:
                break;
            default:
                return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }

        if (frame->header.channels != flac_channels) {
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }

        flac_buffer = lib_malloc(flac_total_size);
        flac_size = 0;
    }

    if (buffer[0] == NULL) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    if (flac_channels == 2) {
        if (buffer [1] == NULL) {
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }
    }

    for (i = 0; i < frame->header.blocksize; ++i) {
        flac_buffer_add(buffer[0][i]);
        if (flac_channels == 2) {
            flac_buffer_add(buffer[1][i]);
        }
    }

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void flac_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        flac_total_samples = metadata->data.stream_info.total_samples;
        flac_sample_rate = metadata->data.stream_info.sample_rate;
        flac_channels = metadata->data.stream_info.channels;
        flac_bps = metadata->data.stream_info.bits_per_sample;
    }
}

static void flac_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
}

static int handle_flac_file(int channels)
{
    FLAC__bool ok = true;
    FLAC__StreamDecoder *decoder = NULL;
    FLAC__StreamDecoderInitStatus init_status;

    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        log_error(filedrv_log, "Cannot init flac decoder");
        return -1;
    }

    (void)FLAC__stream_decoder_set_md5_checking(decoder, true);

    init_status = FLAC__stream_decoder_init_file(decoder, sample_name, flac_write_callback, flac_metadata_callback, flac_error_callback, NULL);
    if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        FLAC__stream_decoder_delete(decoder);
        log_error(filedrv_log, "Cannot init flac decoder");
        return -1;
    }

    ok = FLAC__stream_decoder_process_until_end_of_stream(decoder);
    FLAC__stream_decoder_delete(decoder);

    if (!ok) {
        if (flac_buffer) {
            lib_free(flac_buffer);
            flac_buffer = NULL;
        }
        log_error(filedrv_log, "Error during flac stream decode");
        return -1;
    }

    lib_free(file_buffer);
    file_buffer = flac_buffer;
    flac_buffer = NULL;
    file_size = flac_total_size;
    file_pointer = 0;
    sound_audio_type = AUDIO_TYPE_PCM;
    sound_audio_channels = flac_channels;
    sound_audio_rate = flac_sample_rate;
    sound_audio_bits = 16;
    
    return convert_pcm_buffer(file_size, channels);
}

static int is_flac_file(void)
{
    if (file_size < 4) {
        return 0;
    }
    if (file_buffer[0] == 0x66 && file_buffer[1] == 0x4C && file_buffer[2] == 0x61 && file_buffer[3] == 0x43) {
        return 1;
    }
    return 0;
}
#endif

/* ---------------------------------------------------------------------- */

#ifdef USE_VORBIS
static int handle_vorbis_file(int channels)
{
    OggVorbis_File ov;
    int i;
    ogg_int64_t pcmlength;
    BYTE *vorbis_buffer;
    int dummy;
    int error;
    vorbis_info *vi;

    error = ov_fopen(sample_name, &ov);
    if (error < 0) {
        log_error(filedrv_log, "Unable to open ogg/vorbis file");
        return -1;
    }

    if (!ov_seekable(&ov)) {
        ov_clear(&ov);
        log_error(filedrv_log, "The ogg/vorbis file is not seekable");
        return -1;
    }

    for (i = 0; i < ov.links; i++) {
        vi = ov_info(&ov, i);
        sound_audio_channels = vi->channels;
        if (sound_audio_channels < 1 || sound_audio_channels > 2) {
            ov_clear(&ov);
            log_error(filedrv_log, "The ogg/vorbis file channels is not 1 or 2");
            return -1;
        }
        sound_audio_rate = vi->rate;
        sound_audio_bits = 16;
        sound_audio_type = AUDIO_TYPE_PCM;
    }

    pcmlength = ov_pcm_total(&ov, -1);
    vorbis_buffer = lib_malloc(pcmlength * sound_audio_channels * 2);
    i = 0;
    while (i < pcmlength * sound_audio_channels * 2){
        int ret = ov_read(&ov, (char*)vorbis_buffer + i, (pcmlength * 2 * sound_audio_channels) - i, 0, 2, 1, &dummy);
        if (ret < 0) {
            ov_clear(&ov);
            lib_free(vorbis_buffer);
            vorbis_buffer = NULL;
            log_error(filedrv_log, "Error reading ogg/vorbis stream");
            return -1;
        }
        if (ret) {
            i += ret;
        } else {
            pcmlength = i / (2 * sound_audio_channels);
        }
    }

    ov_clear(&ov);

    lib_free(file_buffer);
    file_buffer = vorbis_buffer;
    file_size = pcmlength * 2 * sound_audio_channels;
    file_pointer = 0;

    return convert_pcm_buffer(file_size, channels);
}

static int is_vorbis_file(void)
{
    if (file_size < 36) {
        return 0;
    }
    if (file_buffer[0] == 0x4F && file_buffer[1] == 0x67 && file_buffer[2] == 0x67 && file_buffer[3] == 0x53) {
        if (file_buffer[29] == 0x76 && file_buffer[30] == 0x6F && file_buffer[31] == 0x72) {
            if (file_buffer[32] == 0x62 && file_buffer[33] == 0x69 && file_buffer[34] == 0x73) {
                return 1;
            }
        }
    }
    return 0;
}
#endif

/* ---------------------------------------------------------------------- */

static int handle_file_type(int channels)
{
    /* Check for wav file */
    if (is_wav_file()) {
#ifdef DEBUG_FILEDRV
        log_warning(filedrv_log, "filetype recognized as a WAVE file, starting parsing.");
#endif
        return handle_wav_file(channels);
    }

    /* Check for voc file */
    if (is_voc_file()) {
#ifdef DEBUG_FILEDRV
        log_warning(filedrv_log, "filetype recognized as a VOC file, starting parsing.");
#endif
        return handle_voc_file(channels);
    }

    /* Check for iff file */
    if (is_iff_file()) {
#ifdef DEBUG_FILEDRV
        log_warning(filedrv_log, "filetype recognized as an IFF file, starting parsing.");
#endif
        return handle_iff_file(channels);
    }

    /* Check for aiff file */
    if (is_aiff_file()) {
#ifdef DEBUG_FILEDRV
        log_warning(filedrv_log, "filetype recognized as an AIFF file, starting parsing.");
#endif
        return handle_aiff_file(channels);
    }

    /* Check for aiff file */
    if (is_aifc_file()) {
#ifdef DEBUG_FILEDRV
        log_warning(filedrv_log, "filetype recognized as an AIFC file, starting parsing.");
#endif
        return handle_aifc_file(channels);
    }

#ifdef USE_FLAC
    /* Check for flac file */
    if (is_flac_file()) {
#ifdef DEBUG_FILEDRV
        log_warning(filedrv_log, "filetype recognized as a FLAC file, starting parsing.");
#endif
        return handle_flac_file(channels);
    }
#endif

#ifdef USE_VORBIS
    /* Check for ogg/vorbis file */
    if (is_vorbis_file()) {
#ifdef DEBUG_FILEDRV
        log_warning(filedrv_log, "filetype recognized as an ogg/vorbis file, starting parsing.");
#endif
        return handle_vorbis_file(channels);
    }
#endif

#ifdef USE_MPG123
    /* Check for mp3 file */
    if (is_mp3_file()) {
#ifdef DEBUG_FILEDRV
        log_warning(filedrv_log, "filetype recognized as an MP3 file, starting parsing.");
#endif
        return handle_mp3_file(channels);
    }
#endif

    log_error(filedrv_log, "filetype was not handled.");
    return -1;
}

/* ---------------------------------------------------------------------- */

static void file_load_sample(int channels)
{
    FILE *sample_file = NULL;
    int err = 0;

    sample_file = fopen(sample_name, "rb");
    if (sample_file) {
        fseek(sample_file, 0, SEEK_END);
        file_size = ftell(sample_file);
        fseek(sample_file, 0, SEEK_SET);
        file_buffer = lib_malloc(file_size);
        if (fread(file_buffer, 1, file_size, sample_file) != file_size) {
            log_warning(filedrv_log, "Unexpected end of data in '%s'.", sample_name);
        }
        fclose(sample_file);
        err = handle_file_type(channels);
        if (!err) {
            sound_sampling_started = 0;
            sound_cycles_per_frame = machine_get_cycles_per_frame();
            sound_frames_per_sec = machine_get_cycles_per_second() / sound_cycles_per_frame;
            sound_samples_per_frame = sound_audio_rate / sound_frames_per_sec;
            current_channels = channels;
        } else {
            lib_free(file_buffer);
            file_buffer = NULL;
            log_error(filedrv_log, "Unknown file type for '%s'.", sample_name);
        }
    } else {
        log_error(filedrv_log, "Cannot open sampler file: '%s'.", sample_name);
    }
}

static void file_free_sample(void)
{
    if (sample_buffer1) {
        if (sample_buffer2) {
            if (sample_buffer1 != sample_buffer2) {
                lib_free(sample_buffer2);
            }
            sample_buffer2 = NULL;
        }
        lib_free(sample_buffer1);
        sample_buffer1 = NULL;
    }
    sound_sampling_started = 0;
}

/* ---------------------------------------------------------------------- */

static int set_sample_name(const char *name, void *param)
{
    if (sample_name != NULL && name != NULL && strcmp(name, sample_name) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    if (sample_buffer1) {
        file_free_sample();
        util_string_set(&sample_name, name);
        file_load_sample(current_channels);
    } else {
        util_string_set(&sample_name, name);
    }

    return 0;
}

static const resource_string_t resources_string[] = {
    { "SampleName", "", RES_EVENT_NO, NULL,
      &sample_name, set_sample_name, NULL },
    RESOURCE_STRING_LIST_END
};

static int sampler_file_resources_init(void)
{
    return resources_register_string(resources_string);
}


static void sampler_file_resources_shutdown(void)
{
    if (sample_name != NULL) {
        lib_free(sample_name);
    }
}


/* ---------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-samplename", SET_RESOURCE, 1,
      NULL, NULL, "SampleName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_SAMPLE_NAME,
      NULL, NULL },
    CMDLINE_LIST_END
};

int sampler_file_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------- */

/* For now channel is ignored */
static BYTE file_get_sample(int channel)
{
    unsigned int current_frame = 0;
    unsigned int current_cycle = 0;
    unsigned int frame_diff = 0;
    unsigned int frame_sample = 0;

    if (!sample_buffer1) {
        return 0x80;
    }
    if (!sound_sampling_started) {
        sound_sampling_started = 1;
        old_frame = maincpu_clk / sound_cycles_per_frame;
        return sample_buffer1[0];
    }
    current_frame = maincpu_clk / sound_cycles_per_frame;
    current_cycle = maincpu_clk % sound_cycles_per_frame;

    if (current_frame > old_frame) {
        frame_diff = current_frame - old_frame;
        while (frame_diff) {
            --frame_diff;
            ++old_frame;
            sound_sample_frame_start += sound_samples_per_frame;
            if (sound_sample_frame_start >= sample_size) {
                sound_sample_frame_start -= sample_size;
            }
        }
    }
    frame_sample = current_cycle * sound_samples_per_frame / sound_cycles_per_frame;

    return sample_buffer1[(frame_sample + sound_sample_frame_start) % sample_size];
}

static void file_shutdown(void)
{
    if (sample_buffer1) {
        file_free_sample();
    }
}

static void file_reset(void)
{
    if (sample_buffer1) {
        sound_sampling_started = 0;
    }
}

static sampler_device_t file_device =
{
    "media file input",
    file_load_sample,
    file_free_sample,
    file_get_sample,
    file_shutdown,
    sampler_file_resources_init,
    sampler_file_cmdline_options_init,
    file_reset
};

void fileaudio_init(void)
{
    filedrv_log = log_open("Sampler Filedrv");

    sampler_device_register(&file_device, SAMPLER_DEVICE_FILE);
}


void fileaudio_shutdown(void)
{
    sampler_file_resources_shutdown();
}


