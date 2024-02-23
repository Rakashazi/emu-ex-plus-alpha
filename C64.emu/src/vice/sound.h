/*
 * sound.h - General code for the sound interface
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

#ifndef VICE_SOUND_H
#define VICE_SOUND_H

#include "vice.h"

#include <stdio.h>

#include "types.h"


/* This define switches the sound system sample calculation
   to use the new and experimental float based sound system */
/* #define SOUND_SYSTEM_FLOAT */

/* OSS: check if needed defines are present */
#ifdef USE_OSS

#if defined(HAVE_LINUX_SOUNDCARD_H)
#include <linux/soundcard.h>
#endif

#if defined(HAVE_MACHINE_SOUNDCARD_H)
#include <machine/soundcard.h>
#endif

#if defined(HAVE_SYS_SOUNDCARD_H)
#include <sys/soundcard.h>
#endif

#if defined(HAVE_SOUNDCARD_H)
#include <soundcard.h>
#endif

#if !defined(AFMT_U8) || !defined(AFMT_S16_LE)
#undef USE_OSS
#endif

#endif

/* Fragment sizes */
enum {
    SOUND_FRAGMENT_VERY_SMALL = 0,
    SOUND_FRAGMENT_SMALL,
    SOUND_FRAGMENT_MEDIUM,
    SOUND_FRAGMENT_LARGE,
    SOUND_FRAGMENT_VERY_LARGE
};

/* Sound output modes */
enum {
    SOUND_OUTPUT_SYSTEM = 0,
    SOUND_OUTPUT_MONO,
    SOUND_OUTPUT_STEREO
};

/* Sound device amounts */
enum {
    SOUND_1_DEVICE = 1,
    SOUND_2_DEVICES,
    SOUND_3_DEVICES,
    SOUND_4_DEVICES,
    SOUND_5_DEVICES,
    SOUND_6_DEVICES,
    SOUND_7_DEVICES,
    SOUND_8_DEVICES
};

/* Sound channels */
enum {
    SOUND_CHANNEL_1 = 1,
    SOUND_CHANNEL_2,
    SOUND_CHANNELS_1_AND_2
};

/* Sound defaults.  */
#if defined(MACOS_COMPILE)
#define SOUND_SAMPLE_RATE 48000
#define SOUND_SAMPLE_BUFFER_SIZE 20
#define SOUND_FRAGMENT_SIZE SOUND_FRAGMENT_VERY_SMALL
#else
#define SOUND_SAMPLE_RATE 48000
#define SOUND_SAMPLE_BUFFER_SIZE 30
#define SOUND_FRAGMENT_SIZE SOUND_FRAGMENT_MEDIUM
#endif

#define SOUND_OUTPUT_CHANNELS_MAX 2

#define SOUND_CHIP_CHANNELS_MAX 8

/** \brief  Maximum number of SIDs supported by the emulation.
 */
#define SOUND_SIDS_MAX 8

/** \brief  Maximum number of SIDs supported by PSID files
 *
 * Maximum number of SIDs for .psid files and thus VSID.
 */
#define SOUND_SIDS_MAX_PSID 3

#define SOUND_CHIPS_MAX 20


/* largest value in the UIs. also used by VSID as default */
#define SOUND_SAMPLE_MAX_BUFFER_SIZE    350

/* Sound device types */
enum {
    SOUND_RECORD_DEVICE = 0,
    SOUND_PLAYBACK_DEVICE,
    SOUND_MOVIE_RECORD_DEVICE
};

/* Sound playback device ID numbers */
enum {
    SOUND_DEVICE_PLAYBACK_PULSE = 0,
    SOUND_DEVICE_PLAYBACK_ALSA,
    SOUND_DEVICE_PLAYBACK_COREAUDIO,
    SOUND_DEVICE_PLAYBACK_SUN_NETBSD,
    SOUND_DEVICE_PLAYBACK_DX,
    SOUND_DEVICE_PLAYBACK_WMM,
    SOUND_DEVICE_PLAYBACK_BEOS,
    SOUND_DEVICE_PLAYBACK_BSP,
    SOUND_DEVICE_PLAYBACK_SDL,
    SOUND_DEVICE_PLAYBACK_DUMMY,

    /* This item always needs to be at the end */
    SOUND_DEVICE_PLAYBACK_MAX
};

/* Sound record device ID numbers */
enum {
    SOUND_DEVICE_RECORD_FS = 0,
    SOUND_DEVICE_RECORD_DUMP,
    SOUND_DEVICE_RECORD_WAV,
    SOUND_DEVICE_RECORD_VOC,
    SOUND_DEVICE_RECORD_IFF,
    SOUND_DEVICE_RECORD_AIFF,
    SOUND_DEVICE_RECORD_MP3,
    SOUND_DEVICE_RECORD_FLAC,
    SOUND_DEVICE_RECORD_OGG,

    /* This item always needs to be at the end */
    SOUND_DEVICE_RECORD_MAX
};

/* Sound movie record device ID numbers */
#define SOUND_DEVICE_MOVIE_RECORD_SOUNDMOVIE   0
#define SOUND_DEVICE_MOVIE_RECORD_MAX          1

extern int sound_state_changed;
extern int sound_playdev_reopen;
extern int sid_state_changed;

/* device structure */
typedef struct sound_device_s {
    /* name of the device */
    const char *name;
    /* init -routine to be called at device initialization. Should use
       suggested values if possible or return new values if they cannot be
       used */
    int (*init)(const char *param, int *speed, int *fragsize, int *fragnr, int *channels);
    /* send number of bytes to the soundcard. it is assumed to block if kernel buffer is full */
    int (*write)(int16_t *pbuf, size_t nr);
    /* dump-routine to be called for every write to SID */
    int (*dump)(uint16_t addr, uint8_t byte, CLOCK clks);
    /* flush-routine to be called every frame */
    int (*flush)(char *state);
    /* return number of samples currently available in the kernel buffer */
    int (*bufferspace)(void);
    /* close and cleanup device */
    void (*close)(void);
    /* suspend device */
    int (*suspend)(void);
    /* resume device */
    int (*resume)(void);
    /* is attenuation needed on suspend or not */
    int need_attenuation;
    /* maximum amount of channels */
    int max_channels;
    /* Can this device be relied on as the emulator timing source */
    bool is_timing_source;
} sound_device_t;

typedef struct sound_register_devices_s {
    const char *name;
    const char *ui_display_name;
    int (*init)(void);
    int device_type;
} sound_register_devices_t;

typedef struct sound_desc_s {
    const char *name;
    const char *description;
    int device_type;
} sound_desc_t;

#ifndef SOUND_SYSTEM_FLOAT
static inline int16_t sound_audio_mix(int ch1, int ch2)
{
    if (ch1 == 0) {
        return (int16_t)ch2;
    }

    if (ch2 == 0) {
        return (int16_t)ch1;
    }

    if ((ch1 > 0 && ch2 < 0) || (ch1 < 0 && ch2 > 0)) {
        return (int16_t)(ch1 + ch2);
    }

    if (ch1 > 0) {
        return (int16_t)((ch1 + ch2) - (ch1 * ch2 / 32768));
    }

    return (int16_t)-((-(ch1) + -(ch2)) - (-(ch1) * -(ch2) / 32768));
}
#endif

sound_desc_t *sound_get_valid_devices(int type, int sort);

/* external functions for vice */
void sound_init(unsigned int clock_rate, unsigned int ticks_per_frame);
void sound_reset(void);
bool sound_flush(void);
void sound_suspend(void);
void sound_resume(void);
int sound_open(void);
void sound_close(void);
void sound_set_relative_speed(int value);
void sound_set_warp_mode(int value);
void sound_set_machine_parameter(long clock_rate, long ticks_per_frame);
void sound_snapshot_prepare(void);
void sound_snapshot_finish(void);

int sound_resources_init(void);
void sound_resources_shutdown(void);
int sound_cmdline_options_init(void);


/* device initialization prototypes */
int sound_init_alsa_device(void);
int sound_init_dummy_device(void);
int sound_init_dump_device(void);
int sound_init_fs_device(void);
int sound_init_wav_device(void);
int sound_init_sdl_device(void);
int sound_init_sun_device(void);
int sound_init_dx_device(void);
int sound_init_beos_device(void);
int sound_init_bsp_device(void);
int sound_init_wmm_device(void);
int sound_init_movie_device(void);
int sound_init_coreaudio_device(void);
int sound_init_voc_device(void);
int sound_init_iff_device(void);
int sound_init_aiff_device(void);
int sound_init_mp3_device(void);
int sound_init_flac_device(void);
int sound_init_vorbis_device(void);
int sound_init_pulse_device(void);

/* internal function for sound device registration */
int sound_register_device(const sound_device_t *pdevice);

/* other internal functions used around sound -code */
int sound_read(uint16_t addr, int chipno);
void sound_store(uint16_t addr, uint8_t val, int chipno);
long sound_sample_position(void);
int sound_dump(int chipno);

/* functions and structs implemented by each machine */
typedef struct sound_s sound_t;

char *sound_machine_dump_state(sound_t *psid);
void sound_machine_enable(int enable);

unsigned int sound_device_num(void);
const char *sound_device_name(unsigned int num);

sound_t *sound_get_psid(unsigned int channel);

#ifdef SOUND_SYSTEM_FLOAT
/* This structure is used by sound producing chips/devices to indicate the left/right mixing in stereo mode per chip channel */
typedef struct sound_chip_mixing_spec_s {

    /* left channel volume of a mono render stream for stereo output, can be used to put the sound left, right, or both, can also be used for panning */
    int left_channel_volume;

    /* right channel volume of a mono render stream for stereo output, can be used to put the sound left, right, or both, can also be used for panning */
    int right_channel_volume;

} sound_chip_mixing_spec_t;
#endif

/* This structure is used by sound producing chips/devices */
typedef struct sound_chip_s {
    /* sound chip open function */
    sound_t *(*open)(int chipno);

    /* sound chip init function */
    int (*init)(sound_t *psid, int speed, int cycles_per_sec);

    /* sound chip close function */
    void (*close)(sound_t *psid);

#ifdef SOUND_SYSTEM_FLOAT
    /* sound chip calculate samples function */
    int (*calculate_samples)(sound_t **psid, float *pbuf, int nr, int sound_chip_channels, CLOCK *delta_t);
#else
    /* sound chip calculate samples function */
    int (*calculate_samples)(sound_t **psid, int16_t *pbuf, int nr, int sound_output_channels, int sound_chip_channels, CLOCK *delta_t);
#endif

    /* sound chip store function */
    void (*store)(sound_t *psid, uint16_t addr, uint8_t val);

    /* sound chip read function */
    uint8_t (*read)(sound_t *psid, uint16_t addr);

    /* sound chip reset function */
    void (*reset)(sound_t *psid, CLOCK cpu_clk);

    /* sound chip 'is_cycle_based()' function */
    int (*cycle_based)(void);

    /* sound chip 'get_amount_of_channels()' function */
    int (*channels)(void);

#ifdef SOUND_SYSTEM_FLOAT
    /* specs for mixing mono chip streams to a stereo stream, stereo channel placement */
    sound_chip_mixing_spec_t *sound_chip_channel_mixing;
#endif

    /* sound chip enabled flag */
    int chip_enabled;

} sound_chip_t;

uint16_t sound_chip_register(sound_chip_t *chip);

typedef struct sound_dac_s {
    float output;
    float alpha;
    int value;
} sound_dac_t;

void sound_dac_init(sound_dac_t *dac, int speed);

#ifdef SOUND_SYSTEM_FLOAT
int sound_dac_calculate_samples(sound_dac_t *dac, float *pbuf, int value, int nr);
#else
int sound_dac_calculate_samples(sound_dac_t *dac, int16_t *pbuf, int value, int nr, int soc, int cs);
#endif

/* recording related functions, equivalent to screenshot_... */
void sound_stop_recording(void);
int sound_is_recording(void);

#define MASTER_VOLUME_MAX       100 /* 100% */
#define MASTER_VOLUME_ONE       100 /* 100% */
#define MASTER_VOLUME_DEFAULT   MASTER_VOLUME_MAX

#include <viceSoundAPI.h>

#endif
