/*
 * clockport-mp3at64.c - ClockPort MP3@64 emulation.
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

#include "vice.h"

#ifdef USE_MPG123

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpg123.h>

#include "clockport.h"
#include "lib.h"
#ifdef MP3AT64_DEBUG
#include "log.h"
#endif
#include "sound.h"
#include "translate.h"
#include "types.h"
#include "uiapi.h"

/* ------------------------------------------------------------------------- */
/*    variables needed                                                       */

#define MP3_INPUT_MAX_FRAME       1500
#define MP3_MAX_SAMPLES_PER_FRAME 1152
#define MP3_BUFFERS               10

static char *clockport_mp3at64_owner = NULL;

static BYTE mp3_frame_buffer[MP3_INPUT_MAX_FRAME];

static SWORD *mp3_output_buffers[MP3_BUFFERS];
static int mp3_output_buffers_size[MP3_BUFFERS];
static int mp3_output_sample_pos = 0;

static int mp3_err = MPG123_OK;
static mpg123_handle *mh = NULL;

static int mp3_output_rate = 44100;

static int mp3_input_rate = 44100;
static int mp3_input_channels = 1;
static int mp3_input_pointer = 0;
static int mp3_input_frame_size = 0;
static int mp3_input_frame_mpeg_version = 0; 
static int mp3_input_frame_layer = 3;
static int mp3_protection = 0;
static int mp3_input_bitrate = 0;
static int mp3_input_padding = 0;

#define MP3_INPUT_STATE_IDLE         0
#define MP3_INPUT_STATE_TAG_HEADER   1
#define MP3_INPUT_STATE_TAG          2
#define MP3_INPUT_STATE_TAG_PLUS     3
#define MP3_INPUT_STATE_MP3_FRAME    4
#define MP3_INPUT_STATE_ID3_HEADER   5
#define MP3_INPUT_STATE_ID3          6

static int mp3_input_data_state = MP3_INPUT_STATE_IDLE;
static BYTE mp3_id3_length[4];
static int mp3_id3_len = 0;

/* ------------------------------------------------------------------------- */

static int mp3_get_free_buffer(void)
{
    int i;

    for (i = 0; i < MP3_BUFFERS; ++i) {
        if (!mp3_output_buffers_size[i]) {
            return i;
        }
    }
    return -1;
}

static void mp3_mono_to_stereo(int block)
{
    SWORD *buffer = lib_malloc(mp3_output_buffers_size[block] * 2);
    int i;

    for (i = 0; i < mp3_output_buffers_size[block]; ++i) {
        buffer[i * 2] = mp3_output_buffers[block][i];
        buffer[(i * 2) + 1] = mp3_output_buffers[block][i];
    }
    lib_free(mp3_output_buffers[block]);
    mp3_output_buffers[block] = buffer;
    mp3_output_buffers_size[block] *= 2;
}

static void mp3_resample(int block, int current_rate)
{
    int i;
    int new_size = mp3_output_buffers_size[block] * mp3_output_rate / current_rate;
    SWORD *buffer = lib_malloc(new_size);

    for (i = 0; i < new_size; ++i) {
        buffer[i] = mp3_output_buffers[block][i * current_rate / mp3_output_rate];
    }
    lib_free(mp3_output_buffers[block]);
    mp3_output_buffers[block] = buffer;
    mp3_output_buffers_size[block] = new_size;
}

void mp3at64_reset(void)
{
    int i;

    for (i = 0; i < MP3_BUFFERS; ++i) {
        if (mp3_output_buffers[i]) {
            lib_free(mp3_output_buffers[i]);
        }
        mp3_output_buffers_size[i] = 0;
        mp3_output_buffers[i] = NULL;
    }
    mp3_input_pointer = 0;
    mp3_input_data_state = MP3_INPUT_STATE_IDLE;
}

static SWORD mp3_get_current_sample(void)
{
    SWORD retval = 0;
    int i;

    if (mp3_output_buffers_size[0]) {
        retval = mp3_output_buffers[0][mp3_output_sample_pos++];
        if (mp3_output_sample_pos >= mp3_output_buffers_size[0]) {
            lib_free(mp3_output_buffers[0]);
            for (i = 1; i < MP3_BUFFERS; ++i) {
                mp3_output_buffers[i - 1] = mp3_output_buffers[i];
                mp3_output_buffers_size[i - 1] = mp3_output_buffers_size[i];
            }
            mp3_output_buffers[MP3_BUFFERS - 1] = NULL;
            mp3_output_buffers_size[MP3_BUFFERS - 1] = 0;
        }
    }
    return retval;
}

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static int clockport_mp3at64_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
static int clockport_mp3at64_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int sound_output_channels, int sound_chip_channels, int *delta_t);
static void clockport_mp3at64_sound_reset(sound_t *psid, CLOCK cpu_clk);
static void clockport_mp3at64_sound_machine_close(sound_t *psid);

static int clockport_mp3at64_sound_machine_cycle_based(void)
{
    return 0;
}

static int clockport_mp3at64_sound_machine_channels(void)
{
    return 1;
}

static sound_chip_t clockport_mp3at64_sound_chip = {
    NULL, /* no open */
    clockport_mp3at64_sound_machine_init,
    clockport_mp3at64_sound_machine_close,
    clockport_mp3at64_sound_machine_calculate_samples,
    NULL, /* no store */
    NULL, /* no read */
    clockport_mp3at64_sound_reset,
    clockport_mp3at64_sound_machine_cycle_based,
    clockport_mp3at64_sound_machine_channels,
    0 /* chip enabled */
};

static WORD clockport_mp3at64_sound_chip_offset = 0;

void clockport_mp3at64_sound_chip_init(void)
{
    clockport_mp3at64_sound_chip_offset = sound_chip_register(&clockport_mp3at64_sound_chip);
}

static int clockport_mp3at64_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
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
    mpg123_open_feed(mh);

    mp3at64_reset();

    return 1;
}

static void clockport_mp3at64_sound_machine_close(sound_t *psid)
{
    int i;
    mpg123_delete(mh);
    mpg123_exit();
    mh = NULL;

    for (i = 0; i < MP3_BUFFERS; ++i) {
        if (mp3_output_buffers[i]) {
            lib_free(mp3_output_buffers[i]);
            mp3_output_buffers[i] = NULL;
            mp3_output_buffers_size[i] = 0;
        }
    }
}

static int clockport_mp3at64_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int soc, int scc, int *delta_t)
{
    int i;
    SWORD sample;

    for (i = 0; i < nr; ++i) {
        switch (soc) {
            default:
            case 1:
                sample = sound_audio_mix(mp3_get_current_sample(), mp3_get_current_sample());
                pbuf[i] = sound_audio_mix(pbuf[i], sample);
                break;
           case 2:
                pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], mp3_get_current_sample());
                pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[i], mp3_get_current_sample());
                break;
        }
    }
    return nr;
}

static void clockport_mp3at64_sound_reset(sound_t *psid, CLOCK cpu_clk)
{
    mp3at64_reset();
}

/* ------------------------------------------------------------------------- */

static void mp3at64_set_i2c_data(BYTE val)
{
    /* TODO */
}

static void mp3at64_set_i2c_clock(BYTE val)
{
    /* TODO */
}

static BYTE mp3at64_read_i2c_data(void)
{
    /* TODO */
    return 0;
}

static BYTE mp3at64_read_i2c_clock(void)
{
    /* TODO */
    return 0;
}

/* ------------------------------------------------------------------------- */

static int mp3_sampling_rates[4][3] = {
    { 44100, 22050, 11025 },
    { 48000, 24000, 12000 },
    { 32000, 16000,  8000 }
};

static int mp3_bitrates[16][3][3] = {
    { {  0,    0,   0 }, {   0,   0,   0 }, {   0,   0,   0 } },
    { {  32,  32,  32 }, {  32,   8,   8 }, {  32,   8,   8 } },
    { {  64,  48,  40 }, {  48,  16,  16 }, {  48,  16,  16 } },
    { {  96,  56,  48 }, {  56,  24,  24 }, {  56,  24,  24 } },
    { { 128,  64,  56 }, {  64,  32,  32 }, {  64,  32,  32 } },
    { { 160,  80,  64 }, {  80,  40,  40 }, {  80,  40,  40 } },
    { { 192,  96,  80 }, {  96,  48,  48 }, {  96,  48,  48 } },
    { { 224, 112,  96 }, { 112,  56,  56 }, { 112,  56,  56 } },
    { { 256, 128, 112 }, { 128,  64,  64 }, { 128,  64,  64 } },
    { { 288, 160, 128 }, { 144,  80,  80 }, { 144,  80,  80 } },
    { { 320, 192, 160 }, { 160,  96,  96 }, { 160,  96,  96 } },
    { { 352, 224, 192 }, { 176, 112, 112 }, { 176, 112, 112 } },
    { { 384, 256, 224 }, { 192, 128, 128 }, { 192, 128, 128 } },
    { { 416, 320, 256 }, { 224, 144, 144 }, { 224, 144, 144 } },
    { { 448, 384, 320 }, { 256, 160, 160 }, { 256, 160, 160 } },
    { {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 } }
};

static int mp3_slot_sizes[3] = {
    4,
    1,
    1
};

static int mp3_sampels_per_frame[3][3] = {
    { 384, 1152, 1152 },
    { 384, 1152,  576 },
    { 384, 1152,  576 }
};

static int mp3_get_layer(void)
{
    switch (mp3_input_frame_layer) {
        case 3:
            return 0;
        case 2:
            return 1;
        case 1:
            return 2;
    }
    return 0;
}

static int mp3_get_mpeg_version(void)
{
    switch (mp3_input_frame_mpeg_version) {
        case 0:
            return 2;
        case 2:
            return 1;
        case 3:
            return 0;
    }
    return 0;
}

static int mp3_get_bitrate(void)
{
    return mp3_bitrates[mp3_input_bitrate][mp3_get_mpeg_version()][mp3_get_layer()];
}

static int mp3_get_sampling_rate(void)
{
    if (mp3_input_rate == 3) {
        return 0;
    }
    return mp3_sampling_rates[mp3_input_rate][mp3_get_mpeg_version()];
}

static int mp3_get_padding_size(void)
{
    if (mp3_input_padding) {
        return mp3_slot_sizes[mp3_get_layer()];
    }
    return 0;
}

static int mp3_get_channels(void)
{
    if (mp3_input_channels == 3) {
        return 1;
    }
    return 2;
}

static int mp3_get_samples_per_frame(void)
{
    return mp3_sampels_per_frame[mp3_get_mpeg_version()][mp3_get_layer()];
}

static int mp3_get_frame_size(void)
{
    return (mp3_get_samples_per_frame() / 8 * (mp3_get_bitrate() * 1000) / mp3_get_sampling_rate()) + mp3_get_padding_size();
}

static int mp3_validate(void)
{
    if (mp3_get_mpeg_version() == -1) {
        return -1;
    }
    if (!mp3_input_frame_layer) {
        return -1;
    }
    if (!mp3_get_bitrate()) {
        return -1;
    }
    if (!mp3_get_sampling_rate()) {
        return -1;
    }
    return 0;
}

static int mp3_frame_is_empty(void)
{
    if (mp3_frame_buffer[0x24] == 'X' && mp3_frame_buffer[0x25] == 'i' && mp3_frame_buffer[0x26] == 'n' && mp3_frame_buffer[0x27] == 'g') {
        return 1;
    }
    if (mp3_frame_buffer[0x24] == 'I' && mp3_frame_buffer[0x25] == 'n' && mp3_frame_buffer[0x26] == 'f' && mp3_frame_buffer[0x27] == 'o') {
        return 1;
    }
    return 0;
}


/* ------------------------------------------------------------------------- */

static void mp3at64_store_mp3_data(BYTE val)
{
    SWORD *buffer = NULL;
    int block;
    int ret;
    size_t size;

    mp3_frame_buffer[mp3_input_pointer] = val;

    switch (mp3_input_data_state) {
        case MP3_INPUT_STATE_IDLE:
            if (val == 0xff) {
                mp3_input_data_state = MP3_INPUT_STATE_MP3_FRAME;
                mp3_input_pointer++;
            }
            if (val == 'T') {
                mp3_input_data_state = MP3_INPUT_STATE_TAG_HEADER;
                mp3_input_pointer++;
            }
            if (val == 'I') {
                mp3_input_data_state = MP3_INPUT_STATE_ID3_HEADER;
                mp3_input_pointer++;
            }
            break;
        case MP3_INPUT_STATE_TAG_HEADER:
            switch (mp3_input_pointer) {
                case 1:
                    if (val == 'A') {
                        mp3_input_pointer++;
                    } else {
                        if (val == 0xff) {
                            mp3_input_data_state = MP3_INPUT_STATE_MP3_FRAME;
                        } else {
                            mp3_input_pointer = 0;
                            mp3_input_data_state = MP3_INPUT_STATE_IDLE;
                        }
                    }
                    break;
                case 2:
                    if (val == 'G') {
                        mp3_input_pointer++;
                    } else {
                        if (val == 0xff) {
                            mp3_input_pointer = 1;
                            mp3_input_data_state = MP3_INPUT_STATE_MP3_FRAME;
                        } else {
                            mp3_input_pointer = 0;
                            mp3_input_data_state = MP3_INPUT_STATE_IDLE;
                        }
                    }
                    break;
                case 3:
                    if (val == '+') {
                        mp3_input_pointer++;
                        mp3_input_data_state = MP3_INPUT_STATE_TAG_PLUS;
                    } else {
                        mp3_input_data_state = MP3_INPUT_STATE_TAG;
                    }
                    break;
            }
            break;
        case MP3_INPUT_STATE_TAG_PLUS:
            mp3_input_pointer++;
            if (mp3_input_pointer == 355) {
                mp3_input_pointer = 0;
                mp3_input_data_state = MP3_INPUT_STATE_IDLE;
            }
            break;
        case MP3_INPUT_STATE_TAG:
            mp3_input_pointer++;
            if (mp3_input_pointer == 128) {
                mp3_input_pointer = 0;
                mp3_input_data_state = MP3_INPUT_STATE_IDLE;
            }
            break;
        case MP3_INPUT_STATE_ID3_HEADER:
            switch (mp3_input_pointer) {
                case 1:
                    if (val == 'D') {
                        mp3_input_pointer++;
                    } else {
                        if (val == 0xff) {
                            mp3_input_data_state = MP3_INPUT_STATE_MP3_FRAME;
                        } else {
                            mp3_input_pointer = 0;
                            mp3_input_data_state = MP3_INPUT_STATE_IDLE;
                        }
                    }
                    break;
                case 2:
                    if (val == '3') {
                        mp3_input_pointer++;
                        mp3_input_data_state = MP3_INPUT_STATE_ID3;
                        mp3_id3_length[0] = 0;
                        mp3_id3_length[1] = 0;
                        mp3_id3_length[2] = 0;
                        mp3_id3_length[3] = 0;
                    } else {
                        if (val == 0xff) {
                            mp3_input_data_state = MP3_INPUT_STATE_MP3_FRAME;
                        } else {
                            mp3_input_pointer = 0;
                            mp3_input_data_state = MP3_INPUT_STATE_IDLE;
                        }
                    }
                    break;
            }
            break;
        case MP3_INPUT_STATE_ID3:
            switch (mp3_input_pointer) {
                case 3:
                case 4:
                case 5:
                    mp3_input_pointer++;
                    break;
                case 6:
                case 7:
                case 8:
                case 9:
                    if (val & 0x80) {
                        mp3_input_pointer = 0;
                        mp3_input_data_state = MP3_INPUT_STATE_IDLE;
                    } else {
                        mp3_id3_length[mp3_input_pointer - 6] = val;
                        mp3_input_pointer++;
                        if (mp3_input_pointer == 10) {
                            mp3_id3_len = (mp3_id3_length[0] << 21) | (mp3_id3_length[1] << 14) | (mp3_id3_length[2] << 7) | mp3_id3_length[3];
                        }
                    }
                    break;
                default:
                    mp3_input_pointer++;
                    if (mp3_input_pointer == mp3_id3_len + 10) {
                        mp3_input_pointer = 0;
                        mp3_input_data_state = MP3_INPUT_STATE_IDLE;
                    }
                    break;
            }
            break;
        case MP3_INPUT_STATE_MP3_FRAME:
            switch (mp3_input_pointer) {
                case 1:
                    if ((val & 0xe0) == 0xe0) {
                        mp3_input_frame_mpeg_version = (val & 0x18) >> 3;
                        mp3_input_frame_layer = (val & 0x06) >> 1;
                        mp3_protection = val & 1;
                        mp3_input_pointer++;
                    } else {
                        mp3_input_pointer = 0;
                        mp3_input_data_state = MP3_INPUT_STATE_IDLE;
                    }
                    break;
                case 2:
                    mp3_input_bitrate = (val & 0xf0) >> 4;
                    mp3_input_rate = (val & 0x0c) >> 2;
                    mp3_input_padding = (val & 0x02) >> 1;
                    /* ignoring private bit */
                    mp3_input_pointer++;
                    break;
                case 3:
                    mp3_input_channels = (val & 0xc0) >> 6;
                    /* ignoring mode extension */
                    /* ignoring copyright bit */
                    /* ignoring original bit */
                    /* ignoring emphasis */
                    if (!mp3_validate()) {
                        mp3_input_pointer++;
                        mp3_input_frame_size = mp3_get_frame_size();
                    } else {
                        mp3_input_pointer = 0;
                        mp3_input_data_state = MP3_INPUT_STATE_IDLE;
                    }
                    break;
                default:
                    mp3_input_pointer++;
                    if (mp3_input_pointer == mp3_input_frame_size) {
                        if (mp3_frame_is_empty()) {
                            mp3_input_pointer = 0;
                            mp3_input_data_state = MP3_INPUT_STATE_IDLE;
                        } else {
                            block = mp3_get_free_buffer();
                            if (block != -1) {
                                buffer = lib_malloc(MP3_MAX_SAMPLES_PER_FRAME * sizeof(SWORD) * 2);
                                ret = mpg123_decode(mh, mp3_frame_buffer, mp3_input_frame_size, (unsigned char *)buffer, MP3_MAX_SAMPLES_PER_FRAME * 2, &size);
                                if (ret != MPG123_ERR && ret != MPG123_NEED_MORE) {
                                    mp3_output_buffers[block] = buffer;
                                    mp3_output_buffers_size[block] = size / 2;
                                    if (mp3_get_channels() != 2) {
                                        mp3_mono_to_stereo(block);
                                    }
                                    if (mp3_get_sampling_rate() != mp3_output_rate) {
                                        mp3_resample(block, mp3_get_sampling_rate());
                                    }
                                }
                            }
                            mp3_input_pointer = 0;
                            mp3_input_data_state = MP3_INPUT_STATE_IDLE;
                        }
                    }
                    break;
            }
    }
}

static BYTE mp3at64_read_mp3_status(void)
{
    if (mp3_get_free_buffer() != -1) {
        return 0x80;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static void clockport_mp3at64_store(WORD address, BYTE val, void *context)
{
    switch (address) {
        case 4:
            mp3at64_store_mp3_data(val);
            break;
        case 5:
#ifdef MP3AT64_DEBUG
            log_warning(LOG_DEFAULT, "storing i2c data: %d", val);
#endif
            mp3at64_set_i2c_data(val);
            break;
        case 6:
#ifdef MP3AT64_DEBUG
            log_warning(LOG_DEFAULT, "storing i2c clock: %d", val);
#endif
            mp3at64_set_i2c_clock(val);
            break;
    }
}

static BYTE clockport_mp3at64_read(WORD address, int *valid, void *context)
{
    switch (address) {
        case 4:
            *valid = 1;
            return mp3at64_read_mp3_status();
        case 5:
            *valid = 1;
            return mp3at64_read_i2c_data();
        case 6:
            *valid = 1;
            return mp3at64_read_i2c_clock();
        default:
            *valid = 0;
    }
    return 0;
}

static BYTE clockport_mp3at64_peek(WORD address, void *context)
{
    switch (address) {
        case 4:
            return mp3at64_read_mp3_status();
        case 5:
            return mp3at64_read_i2c_data();
        case 6:
            return mp3at64_read_i2c_clock();
    }
    return 0;
}

static void clockport_mp3at64_reset(void *context)
{
    mp3at64_reset();
}

static int clockport_mp3at64_dump(void *context)
{
    /* TODO */
    return 0;
}

static void clockport_mp3at64_close(struct clockport_device_s *device)
{
    if (clockport_mp3at64_sound_chip.chip_enabled) {
        clockport_mp3at64_sound_chip.chip_enabled = 0;
        clockport_mp3at64_owner = NULL;
        lib_free(device);
    }
}

/* ------------------------------------------------------------------------- */

int clockport_mp3at64_init(void)
{
    mp3at64_reset();

    return 0;
}

void clockport_mp3at64_shutdown(void)
{
    mp3at64_reset();
}

clockport_device_t *clockport_mp3at64_open_device(char *owner)
{
    clockport_device_t *retval = NULL;
    if (clockport_mp3at64_sound_chip.chip_enabled) {
        ui_error(translate_text(IDGS_CLOCKPORT_MP3AT64_IN_USE_BY_S), clockport_mp3at64_owner);
        return NULL;
    }
    retval = lib_malloc(sizeof(clockport_device_t));
    retval->owner = owner;
    retval->devicenr = 0;
    retval->store = clockport_mp3at64_store;
    retval->read = clockport_mp3at64_read;
    retval->peek = clockport_mp3at64_peek;
    retval->reset = clockport_mp3at64_reset;
    retval->dump = clockport_mp3at64_dump;
    retval->close = clockport_mp3at64_close;
    retval->device_context = NULL;

    mp3at64_reset();

    clockport_mp3at64_sound_chip.chip_enabled = 1;

    return retval;
}

#endif /* #ifdef USE_MPG123 */
