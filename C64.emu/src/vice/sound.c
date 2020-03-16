/*
 * sound.c - General code for the sound interface.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Resource and cmdline code by
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "archdep.h"
#include "clkguard.h"
#include "cmdline.h"
#include "debug.h"
#include "fixpoint.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "monitor.h"
#include "resources.h"
#include "sound.h"
#include "types.h"
#include "uiapi.h"
#include "util.h"
#include "vsync.h"
#include "math.h"
#include "ui.h"


static log_t sound_log = LOG_ERR;

/* ------------------------------------------------------------------------- */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* ------------------------------------------------------------------------- */

typedef struct sound_register_devices_s {
    char *name;
    int (*init)(void);
    int is_playback_device;
} sound_register_devices_t;

/* This table is used to specify the order of inits of the playback and recording devices */
static sound_register_devices_t sound_register_devices[] = {

    /* the "native" platform specific drivers should come first, sorted by
       priority (most wanted first) */

#ifdef USE_PULSE
    { "pulse", sound_init_pulse_device, SOUND_PLAYBACK_DEVICE },
#endif
#ifdef USE_ARTS
    { "arts", sound_init_arts_device, SOUND_PLAYBACK_DEVICE },
#endif
#ifdef USE_ALSA
    { "alsa", sound_init_alsa_device, SOUND_PLAYBACK_DEVICE },
#endif
#ifdef USE_COREAUDIO
    { "coreaudio", sound_init_coreaudio_device, SOUND_PLAYBACK_DEVICE },
#endif

/* Don't use the NetBSD/SUN sound driver for OpenBSD */
#if defined(HAVE_SYS_AUDIOIO_H) && !defined(__OpenBSD__)
#if defined(__NetBSD__)
    { "netbsd", sound_init_sun_device, SOUND_PLAYBACK_DEVICE },
#else
    /* Really? */
    { "sun", sound_init_sun_device, SOUND_PLAYBACK_DEVICE },
#endif /* __NetBSD__*/
#endif /* HAVE_SYS_AUDIOIO_H */

#ifdef WIN32_COMPILE
#ifdef USE_DXSOUND
    { "dx", sound_init_dx_device, SOUND_PLAYBACK_DEVICE },
#endif
#if !defined(__XBOX__)
    { "wmm", sound_init_wmm_device, SOUND_PLAYBACK_DEVICE },
#endif
#endif

#if defined(__OS2__) && !defined(USE_SDLUI)
    { "dart", sound_init_dart_device, SOUND_PLAYBACK_DEVICE },
#endif

#ifdef BEOS_COMPILE
    { "beos", sound_init_beos_device, SOUND_PLAYBACK_DEVICE },
    { "bsp", sound_init_bsp_device, SOUND_PLAYBACK_DEVICE },
#endif

#if defined(AMIGA_SUPPORT) && defined(HAVE_DEVICES_AHI_H)
    { "ahi", sound_init_ahi_device, SOUND_PLAYBACK_DEVICE },
#endif

    /* SDL driver last, after all platform specific ones */
#ifdef USE_SDL_AUDIO
    { "sdl", sound_init_sdl_device, SOUND_PLAYBACK_DEVICE },
#endif

    /* the dummy device acts as a "guard" against the drivers that create files,
       since the list will be searched top-down, and the dummy driver always
       works, no files will be created accidently */
    { "dummy", sound_init_dummy_device, SOUND_PLAYBACK_DEVICE },

#ifndef EMU_EX_PLATFORM
    { "fs", sound_init_fs_device, SOUND_RECORD_DEVICE },
    { "dump", sound_init_dump_device, SOUND_RECORD_DEVICE },
    { "wav", sound_init_wav_device, SOUND_RECORD_DEVICE },
    { "voc", sound_init_voc_device, SOUND_RECORD_DEVICE },
    { "iff", sound_init_iff_device, SOUND_RECORD_DEVICE },
    { "aiff", sound_init_aiff_device, SOUND_RECORD_DEVICE },
#endif

#ifdef USE_LAMEMP3
    { "mp3", sound_init_mp3_device, SOUND_RECORD_DEVICE },
#endif

#ifdef USE_FLAC
    { "flac", sound_init_flac_device, SOUND_RECORD_DEVICE },
#endif

#ifdef USE_VORBIS
    { "ogg", sound_init_vorbis_device, SOUND_RECORD_DEVICE },
#endif

#ifndef EMU_EX_PLATFORM
    { "soundmovie", sound_init_movie_device, SOUND_RECORD_DEVICE },
#endif
    { NULL, NULL, 0 }
};

/* ------------------------------------------------------------------------- */

static uint16_t offset = 0;

static sound_chip_t *sound_calls[20];

uint16_t sound_chip_register(sound_chip_t *chip)
{
    assert(chip != NULL);

    sound_calls[offset >> 5] = chip;
    offset += 0x20;

    assert((offset >> 5) < 20);

    return offset - 0x20;
}

/* ------------------------------------------------------------------------- */

static sound_t *sound_machine_open(int chipno)
{
    sound_t *retval = NULL;
    int i;

    for (i = 0; i < (offset >> 5); i++) {
        if (sound_calls[i]->open) {
            retval = sound_calls[i]->open(chipno);
        }
    }
    return retval;
}

static int sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    int retval = 1;
    int i;

    for (i = 0; i < (offset >> 5); i++) {
        if (sound_calls[i]->init) {
            retval &= sound_calls[i]->init(psid, speed, cycles_per_sec);
        }
    }
    return retval;
}

static void sound_machine_close(sound_t *psid)
{
    int i;

    for (i = 0; i < (offset >> 5); i++) {
        if (sound_calls[i]->close) {
            sound_calls[i]->close(psid);
        }
    }
}

/*
    There is some inconsistency about when the buffer should be overwritten and
    when mixed. Usually it's overwritten by SID and other cycle based engines,
    and mixed by non-cycle based engines. On pet unfortunately it is always
    mixed (unless SID is enabled) resulting in looping and distorted sound. As
    a quick bandaid the memset was added below. This should be really cleaned
    up someday.
*/
static int sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int soc, int scc, int *delta_t)
{
    int i;
    int temp;

    if (sound_calls[0]->cycle_based() || (!sound_calls[0]->cycle_based() && sound_calls[0]->chip_enabled)) {
        temp = sound_calls[0]->calculate_samples(psid, pbuf, nr, soc, scc, delta_t);
    } else {
        memset(pbuf, 0, nr * sizeof(int16_t) * soc); /* FIXME: see above */
        temp = nr;
    }

    for (i = 1; i < (offset >> 5); i++) {
        if (sound_calls[i]->chip_enabled) {
            sound_calls[i]->calculate_samples(psid, pbuf, temp, soc, scc, delta_t);
        }
    }
    return temp;
}

static void sound_machine_store(sound_t *psid, uint16_t addr, uint8_t val)
{
    if (sound_calls[addr >> 5]->store) {
        sound_calls[addr >> 5]->store(psid, (uint16_t)(addr & 0x1f), val);
    }
}

static uint8_t sound_machine_read(sound_t *psid, uint16_t addr)
{
    if (sound_calls[addr >> 5]->read) {
        return sound_calls[addr >> 5]->read(psid, (uint16_t)(addr & 0x1f));
    }
    return 0;
}

static void sound_machine_reset(sound_t *psid, CLOCK cpu_clk)
{
    int i;

    for (i = 0; i < (offset >> 5); i++) {
        if (sound_calls[i]->reset) {
            sound_calls[i]->reset(psid, cpu_clk);
        }
    }
}

static int sound_machine_cycle_based(void)
{
    int i;
    int retval = 0;

    for (i = 0; i < (offset >> 5); i++) {
        retval |= sound_calls[i]->cycle_based();
    }
    return retval;
}

static int sound_machine_channels(void)
{
    int i;
    int retval = 0;
    int temp;

    for (i = 0; i < (offset >> 5); i++) {
        temp = sound_calls[i]->channels();
        if (temp > retval) {
            retval = temp;
        }
    }
    return retval;
}

/* ------------------------------------------------------------------------- */

/* Resource handling -- Added by Ettore 98-04-26.  */

/* FIXME: We need sanity checks!  And do we really need all of these
   `sound_close()' calls?  */

static int playback_enabled;           /* app_resources.sound */
static int sample_rate;                /* app_resources.soundSampleRate */
static char *device_name = NULL;       /* app_resources.soundDeviceName */
static char *device_arg = NULL;        /* app_resources.soundDeviceArg */
static char *recorddevice_name = NULL; /* app_resources.soundDeviceName */
static char *recorddevice_arg = NULL;  /* app_resources.soundDeviceArg */
static int buffer_size;                /* app_resources.soundBufferSize */
static int suspend_time;               /* app_resources.soundSuspendTime */
static int speed_adjustment_setting;   /* app_resources.soundSpeedAdjustment */
static int volume;
static int amp;
static int fragment_size;
static int output_option;

/* divisors for fragment size calculation */
static int fragment_divisor[] = {
    32, /* 100ms / 32 = 0.625ms */
    16, /* 100ms / 16 = 1.25ms */
     8, /* 100ms / 8 = 2.5ms */
     4, /* 100ms / 4 = 5ms */
     2, /* 100ms / 2 = 10 ms */
     1  /* 100ms / 1 = 20 ms, actually unused (since it is not practical) */
};

static char *playback_devices_cmdline = NULL;
static char *record_devices_cmdline = NULL;

/* I need this to serialize close_sound and enablesound/sound_open in
   the OS/2 Multithreaded environment                              */
static int sdev_open = FALSE;

/* I need this to serialize close_sound and enablesound/sound_open in
   the OS/2 Multithreaded environment                              */
int sound_state_changed;
int sid_state_changed;

/* Sample based or cycle based sound engine. */
static int cycle_based = 0;

static int set_output_option(int val, void *param)
{
    switch (val) {
        case SOUND_OUTPUT_SYSTEM:
        case SOUND_OUTPUT_MONO:
        case SOUND_OUTPUT_STEREO:
            break;
        default:
            return -1;
    }

    if (output_option != val) {
        output_option = val;
        sound_state_changed = TRUE;
    }
    return 0;
}

static int set_playback_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (val) {
        vsync_disable_timer();
    }

    playback_enabled = val;
    sound_machine_enable(playback_enabled);
    return 0;
}

static int set_sample_rate(int val, void *param)
{
    if (val <= 0) {
        return -1;
    }

    sample_rate = val;
    sound_state_changed = TRUE;
    return 0;
}

static int set_device_name(const char *val, void *param)
{
    if (!val || val[0] == '\0') {
        /* Use the default sound device */
#ifdef BEOS_COMPILE
        if (archdep_is_haiku()) {
            util_string_set(&device_name, "bsp");
        } else
#endif
        {
            util_string_set(&device_name, sound_register_devices[0].name);
        }
    } else {
        util_string_set(&device_name, val);
    }
    sound_state_changed = TRUE;
    return 0;
}

static int set_device_arg(const char *val, void *param)
{
    util_string_set(&device_arg, val);
    sound_state_changed = TRUE;
    return 0;
}

static int set_recorddevice_name(const char *val, void *param)
{
    util_string_set(&recorddevice_name, val);
    sound_state_changed = TRUE;
    return 0;
}

static int set_recorddevice_arg(const char *val, void *param)
{
    util_string_set(&recorddevice_arg, val);
    sound_state_changed = TRUE;
    return 0;
}

static int set_buffer_size(int val, void *param)
{
    if (val > 0) {
        buffer_size = val;
    } else {
        if (machine_class == VICE_MACHINE_VSID) {
            buffer_size = SOUND_SAMPLE_MAX_BUFFER_SIZE;
        } else {
            buffer_size = SOUND_SAMPLE_BUFFER_SIZE;
        }
    }

    sound_state_changed = TRUE;
    return 0;
}

static int set_fragment_size(int val, void *param)
{
    if (val < SOUND_FRAGMENT_VERY_SMALL) {
        val = SOUND_FRAGMENT_VERY_SMALL;
    } else if (val > SOUND_FRAGMENT_VERY_LARGE) {
        val = SOUND_FRAGMENT_VERY_LARGE;
    }
    fragment_size = val;
    sound_state_changed = TRUE;
    return 0;
}

static int set_suspend_time(int val, void *param)
{
    suspend_time = val;

    if (suspend_time < 0) {
        suspend_time = 0;
    }

    sound_state_changed = TRUE;
    return 0;
}

static int set_speed_adjustment_setting(int val, void *param)
{
    if (val == SOUND_ADJUST_DEFAULT) {
        speed_adjustment_setting = SOUND_ADJUST_EXACT;
    } else {
        switch (val) {
            case SOUND_ADJUST_FLEXIBLE:
            case SOUND_ADJUST_ADJUSTING:
            case SOUND_ADJUST_EXACT:
                break;
            default:
                return -1;
        }
        speed_adjustment_setting = val;
    }

    return 0;
}

static int set_volume(int val, void *param)
{
    volume = val;

    if (volume < 0) {
        volume = 0;
    }

    if (volume > 100) {
        volume = 100;
    }

    amp = (int)((exp((double)volume / 100.0 * log(2.0)) - 1.0) * 4096.0);

    ui_display_volume(volume);

    return 0;
}

static const resource_string_t resources_string[] = {
    { "SoundDeviceName", "", RES_EVENT_NO, NULL,
      &device_name, set_device_name, NULL },
    { "SoundDeviceArg", "", RES_EVENT_NO, NULL,
      &device_arg, set_device_arg, NULL },
    { "SoundRecordDeviceName", "", RES_EVENT_STRICT, (resource_value_t)"",
      &recorddevice_name, set_recorddevice_name, NULL },
    { "SoundRecordDeviceArg", "", RES_EVENT_NO, NULL,
      &recorddevice_arg, set_recorddevice_arg, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "Sound", 1, RES_EVENT_SAME, NULL,
      (void *)&playback_enabled, set_playback_enabled, NULL },
    { "SoundSampleRate", SOUND_SAMPLE_RATE, RES_EVENT_NO, NULL,
      (void *)&sample_rate, set_sample_rate, NULL },
    { "SoundBufferSize", 0, RES_EVENT_NO, NULL,
      (void *)&buffer_size, set_buffer_size, NULL },
    { "SoundFragmentSize", ARCHDEP_SOUND_FRAGMENT_SIZE, RES_EVENT_NO, NULL,
      (void *)&fragment_size, set_fragment_size, NULL },
    { "SoundSuspendTime", 0, RES_EVENT_NO, NULL,
      (void *)&suspend_time, set_suspend_time, NULL },
    { "SoundSpeedAdjustment", SOUND_ADJUST_EXACT, RES_EVENT_NO, NULL,
      (void *)&speed_adjustment_setting, set_speed_adjustment_setting, NULL },
    { "SoundVolume", 100, RES_EVENT_NO, NULL,
      (void *)&volume, set_volume, NULL },
    { "SoundOutput", ARCHDEP_SOUND_OUTPUT_MODE, RES_EVENT_NO, NULL,
      (void *)&output_option, set_output_option, NULL },
    RESOURCE_INT_LIST_END
};

int sound_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void sound_resources_shutdown(void)
{
    lib_free(device_name);
    lib_free(device_arg);
    lib_free(recorddevice_name);
    lib_free(recorddevice_arg);
    lib_free(playback_devices_cmdline);
    lib_free(record_devices_cmdline);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-sound", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Sound", (resource_value_t)1,
      NULL, "Enable sound playback" },
    { "+sound", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Sound", (resource_value_t)0,
      NULL, "Disable sound playback" },
    { "-soundrate", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SoundSampleRate", NULL,
      "<value>", "Set sound sample rate to <value> Hz" },
    { "-soundbufsize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SoundBufferSize", NULL,
      "<value>", "Set sound buffer size to <value> msec" },
    { "-soundfragsize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SoundFragmentSize", NULL,
      "<value>", "Set sound fragment size (0: very small, 1: small, 2: medium, 3: large, 4: very large)" },
    { "-soundsync", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SoundSpeedAdjustment", NULL,
      "<sync>", "Set sound speed adjustment (0: flexible, 1: adjusting, 2: exact)" },
    { "-soundoutput", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SoundOutput", NULL,
      "<output mode>", "Sound output mode: (0: system decides mono/stereo, 1: always mono, 2: always stereo)" },
    { "-soundsuspend", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SoundSuspendTime", NULL,
      "<Seconds>", "Specify the pause interval when audio underflows (clicks) happen. 0 means no pause is done." },
    { "-soundvolume", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SoundVolume", NULL,
      "<Volume>", "Specify the sound volume (0..100)" },
    CMDLINE_LIST_END
};

static cmdline_option_t devs_cmdline_options[] =
{
    { "-sounddev", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SoundDeviceName", NULL,
      "<Name>", NULL },
    { "-soundarg", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SoundDeviceArg", NULL,
      "<args>", "Specify initialization parameters for sound driver" },
    { "-soundrecdev", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SoundRecordDeviceName", NULL,
      "<Name>", NULL },
    { "-soundrecarg", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SoundRecordDeviceArg", NULL,
      "<args>", "Specify initialization parameters for recording sound driver" },
    CMDLINE_LIST_END
};

int sound_cmdline_options_init(void)
{
    int i;
    int started_playback = 0;
    int started_record = 0;
    char *temp = NULL;

    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    playback_devices_cmdline = lib_strdup("Specify sound driver. (");
    record_devices_cmdline = lib_strdup("Specify recording sound driver. (");

    for (i = 0; sound_register_devices[i].name; i++) {
        if (sound_register_devices[i].is_playback_device) {
            if (started_playback) {
                temp = util_concat(playback_devices_cmdline, "/", sound_register_devices[i].name, NULL);
            } else {
                temp = util_concat(playback_devices_cmdline, sound_register_devices[i].name, NULL);
                started_playback = 1;
            }
            lib_free(playback_devices_cmdline);
            playback_devices_cmdline = temp;
        } else {
            if (started_record) {
                temp = util_concat(record_devices_cmdline, "/", sound_register_devices[i].name, NULL);
            } else {
                temp = util_concat(record_devices_cmdline, sound_register_devices[i].name, NULL);
                started_record = 1;
            }
            lib_free(record_devices_cmdline);
            record_devices_cmdline = temp;
        }
    }
    temp = util_concat(playback_devices_cmdline, ")", NULL);
    lib_free(playback_devices_cmdline);
    playback_devices_cmdline = temp;

    temp = util_concat(record_devices_cmdline, ")", NULL);
    lib_free(record_devices_cmdline);
    record_devices_cmdline = temp;

    devs_cmdline_options[0].description = playback_devices_cmdline;
    devs_cmdline_options[2].description = record_devices_cmdline;

    return cmdline_register_options(devs_cmdline_options);
}

/* ------------------------------------------------------------------------- */

/* timing constants */
static unsigned int cycles_per_sec;
static unsigned int cycles_per_rfsh;
static double rfsh_per_sec;

/* Speed in percent, tracks relative_speed from vsync.c */
static int speed_percent;

/* Flag: Is warp mode enabled?  */
static int warp_mode_enabled;

typedef struct {
    /* Number of sound output channels */
    int sound_output_channels;

    /* Number of sound chip channels (for multiple SIDs) */
    int sound_chip_channels;

    /* sid itself */
    sound_t *psid[SOUND_SIDS_MAX];

    /* number of clocks between each sample. used value */
    soundclk_t clkstep;

    /* number of clocks between each sample. original value */
    soundclk_t origclkstep;

    /* factor between those two clksteps */
    soundclk_t clkfactor;

    /* time of last sample generated */
    soundclk_t fclk;

    /* time of last write to sid. used for pdev->dump() */
    CLOCK wclk;

    /* time of last call to sound_run_sound() */
    CLOCK lastclk;

    /* sample buffer */
    int16_t buffer[SOUND_CHANNELS_MAX * SOUND_BUFSIZE];

    /* sample buffer pointer */
    int bufptr;

    /* pointer to playback device structure in use */
    sound_device_t *playdev;

    /* pointer to playback device structure in use */
    sound_device_t *recdev;

    /* number of samples in a fragment */
    int fragsize;

    /* number of fragments in kernel buffer */
    int fragnr;

    /* number of samples in kernel buffer */
    int bufsize;

    /* constants related to adjusting sound */
    int prevused;
    int prevfill;

    /* is the device suspended? */
    int issuspended;
    int16_t lastsample[SOUND_CHANNELS_MAX];
} snddata_t;

static snddata_t snddata;

/* device registration code */
#define MAX_SOUND_DEVICES 24

static sound_device_t *sound_devices[MAX_SOUND_DEVICES];

static int sound_device_count = 0;

int sound_register_device(sound_device_t *pdevice)
{
    if (sound_device_count < MAX_SOUND_DEVICES) {
        sound_devices[sound_device_count] = pdevice;
        sound_device_count++;
    } else {
        log_error(sound_log, "available sound devices exceed VICEs storage");
    }

    return 0;
}

unsigned int sound_device_num(void)
{
    return sound_device_count;
}

const char *sound_device_name(unsigned int num)
{
    return sound_devices[num]->name;
}


/* code to disable sid for a given number of seconds if needed */
static time_t disabletime;

static void suspendsound(const char *reason)
{
    disabletime = time(0);
    log_warning(sound_log, "suspend, disabling sound for %d secs (%s)",
                suspend_time, reason);
    sound_state_changed = TRUE;
}

static void enablesound(void)
{
    time_t diff;
    if (!disabletime) {
        return;
    }
    diff = time(0) - disabletime;
    if (diff < 0 || diff >= (time_t)suspend_time) {
        disabletime = 0;
    }
}

/* close sid device and show error dialog */
static int sound_error(const char *msg)
{
    sound_close();

    if (console_mode || video_disabled_mode) {
        log_message(sound_log, "%s", msg);
    } else {
        char *txt = lib_msprintf("Sound: %s", msg);
        ui_error(txt);
        lib_free(txt);
    }

    playback_enabled = 0;

    if (!console_mode) {
        ui_update_menus();
    }

    return 1;
}

static int16_t *temp_buffer = NULL;
static int temp_buffer_size = 0;

static int16_t *realloc_buffer(int size)
{
    if (temp_buffer_size < size) {
        temp_buffer = lib_realloc(temp_buffer, size);
        if (temp_buffer) {
            temp_buffer_size = size;
            memset(temp_buffer, 0, size);
        } else {
            temp_buffer_size = 0;
        }
    }
    return temp_buffer;
}

/* Fill buffer with last sample.
 rise  < 0 : attenuation
 rise == 0 : constant value
 rise  > 0 : gain
*/
static void fill_buffer(int size, int rise)
{
    int c, i;
    int16_t *p;
    double factor;

    p = realloc_buffer(size * sizeof(int16_t) * snddata.sound_output_channels);
    if (!p) {
        return;
    }

    for (c = 0; c < snddata.sound_output_channels; c++) {
        for (i = 0; i < size; i++) {
            if (rise < 0) {
                factor = (double)(size - i) / size;
            } else {
                if (rise > 0) {
                    factor = (double)i / size;
                } else {
                    factor = 1.0;
                }
            }

            p[i * snddata.sound_output_channels + c] = (int16_t)(snddata.lastsample[c] * factor);
        }
    }

    i = snddata.playdev->write(p, size * snddata.sound_output_channels);
    if (i) {
        sound_error("write to sound device failed.");
    }
}


/* open SID engine */
static int sid_open(void)
{
    int c;

    for (c = 0; c < snddata.sound_chip_channels; c++) {
        if (!(snddata.psid[c] = sound_machine_open(c))) {
            return sound_error("Cannot open SID engine");
        }
    }

    return 0;
}

/* initialize SID engine */
static int sid_init(void)
{
    int c, speed, speed_factor;

    /* Special handling for cycle based as opposed to sample based sound
       engines. reSID is cycle based. */
    cycle_based = sound_machine_cycle_based();

    /* "No limit" doesn't make sense for cycle based sound engines,
       which have a fixed sampling rate. */
    speed_factor = speed_percent ? speed_percent : 100;
    speed = sample_rate * 100 / speed_factor;

    for (c = 0; c < snddata.sound_chip_channels; c++) {
        if (!sound_machine_init(snddata.psid[c], speed, cycles_per_sec)) {
            return sound_error("Cannot initialize SID engine");
        }
    }

    snddata.clkstep = SOUNDCLK_CONSTANT(cycles_per_sec) / sample_rate;

    snddata.origclkstep = snddata.clkstep;
    snddata.clkfactor = SOUNDCLK_CONSTANT(1.0);
    snddata.fclk = SOUNDCLK_CONSTANT(maincpu_clk);
    snddata.wclk = maincpu_clk;
    snddata.lastclk = maincpu_clk;

    return 0;
}

/* close SID engine */
static void sid_close(void)
{
    int c;
    for (c = 0; c < snddata.sound_chip_channels; c++) {
        if (snddata.psid[c]) {
            sound_machine_close(snddata.psid[c]);
            snddata.psid[c] = NULL;
        }
    }
}

sound_t *sound_get_psid(unsigned int channel)
{
    return snddata.psid[channel];
}

/* open sound device */
int sound_open(void)
{
    int c, i, j;
    int channels_cap;
    int channels;
    sound_device_t *pdev, *rdev;
    char *playname, *recname;
    char *playparam, *recparam;
    char *err;
    int speed;
    int fragsize;
    int fragnr;
    char frag_str[8];
    double bufsize;

    if (suspend_time > 0 && disabletime) {
        return 1;
    }

    /* Opening the sound device and initializing the sound engine
       might take some time. */
    vsync_suspend_speed_eval();

    /* Second SID. */
    snddata.sound_chip_channels = sound_machine_channels();

    playname = device_name;
    if (playname && playname[0] == '\0') {
        playname = NULL;
    }

    playparam = device_arg;
    if (playparam && playparam[0] == '\0') {
        playparam = NULL;
    }

    recname = recorddevice_name;
    if (recname && recname[0] == '\0') {
        recname = NULL;
    }

    recparam = recorddevice_arg;
    if (recparam && recparam[0] == '\0') {
        recparam = NULL;
    }

    /* Calculate buffer size in seconds. */
    bufsize = ((buffer_size < 1 || buffer_size > 1000)
               ? SOUND_SAMPLE_BUFFER_SIZE : buffer_size) / 1000.0;
    speed = (sample_rate < 8000 || sample_rate > 96000)
            ? SOUND_SAMPLE_RATE : sample_rate;

    switch (output_option) {
        case SOUND_OUTPUT_SYSTEM:
        default:
            channels = (snddata.sound_chip_channels >= 2) ? 2 : 1;
            break;
        case SOUND_OUTPUT_MONO:
            channels = 1;
            break;
        case SOUND_OUTPUT_STEREO:
            channels = 2;
            break;
    }

    /* find pdev */
    for (i = 0; (pdev = sound_devices[i]); i++) {
        if (!playname || (pdev->name && !strcasecmp(playname, pdev->name))) {
            break;
        }
    }

    /* Calculate reasonable fragments. Target is 2 fragments per frame,
     * which gives a reasonable number of fillable audio chunks to avoid
     * ugly situation where a full frame refresh needs to occur before more
     * audio is generated. It also improves the estimate of optimal frame
     * length for vsync, which is closely tied to audio and uses the fragment
     * information to calculate it. */
    /* note: in practise it is actually better to use fragments that are as
     *       small as possible, as that will allow the whole system to catch up
     *       faster and compensate errors better. */
    #ifdef EMU_EX_PLATFORM
    fragsize = 1;
    #else
    fragsize = speed / ((rfsh_per_sec < 1.0) ? 1 : ((int)rfsh_per_sec))
               / fragment_divisor[fragment_size];
    if (pdev) {
        if (channels <= pdev->max_channels) {
            fragsize *= channels;
        }
    }
    #endif

    for (i = 1; 1 << i < fragsize; i++) {
    }
    fragsize = 1 << i;
    fragnr = (int)((speed * bufsize + fragsize - 1) / fragsize);
    if (fragnr < 3) {
        fragnr = 3;
    }

    if (pdev) {
        if (pdev->init) {
            channels_cap = channels;
            if (pdev->init(playparam, &speed, &fragsize, &fragnr, &channels_cap)) {
                err = lib_msprintf("initialization failed for device `%s'.", pdev->name);
                sound_error(err);
                lib_free(err);
                return 1;
            }
            if (channels_cap != channels) {
                if (output_option != SOUND_OUTPUT_MONO) {
                    log_warning(sound_log, "sound device lacks stereo capability, switching to mono output");
                }
                snddata.sound_output_channels = 1;
            } else {
                snddata.sound_output_channels = channels;
            }
        }
        snddata.issuspended = 0;

        for (c = 0; c < snddata.sound_output_channels; c++) {
            snddata.lastsample[c] = 0;
        }

        snddata.playdev = pdev;
        snddata.fragsize = fragsize;
        snddata.fragnr = fragnr;
        snddata.bufsize = fragsize * fragnr;
        snddata.bufptr = 0;
        /* log_message isn't guarenteed to handle "%f" */
        sprintf(frag_str, "%.1f", (1000.0 * fragsize / speed));
        log_message(sound_log,
                    "Opened device `%s', speed %dHz, fragment size %sms, buffer size %dms%s",
                    pdev->name, speed, frag_str,
                    (int)(1000.0 * snddata.bufsize / speed),
                    snddata.sound_output_channels > 1 ? ", stereo" : "");
        sample_rate = speed;

        if (sid_open() != 0 || sid_init() != 0) {
            return 1;
        }

        sid_state_changed = FALSE;

        /* Fill up the sound hardware buffer. */
        if (pdev->bufferspace) {
            /* Fill to bufsize - fragsize. */
            j = pdev->bufferspace() - snddata.fragsize;
            if (j > 0) {
                /* Whole fragments. */
                j -= j % snddata.fragsize;

                fill_buffer(j, 0);
            }
        }
    } else {
        err = lib_msprintf("device '%s' not found or not supported.", playname);
        sound_error(err);
        lib_free(err);
        return 1;
    }

    /* now the playback sound device is open */
    sdev_open = TRUE;
    sound_state_changed = FALSE;

    for (i = 0; (rdev = sound_devices[i]); i++) {
        if (recname && rdev->name && !strcasecmp(recname, rdev->name)) {
            break;
        }
    }

    if (recname && rdev == NULL) {
        ui_error("Recording device %s doesn't exist!", recname);
    }

    if (rdev) {
        if (rdev == pdev) {
            ui_error("Recording device must be different from playback device");
            resources_set_string("SoundRecordDeviceName", "");
            return 0;
        }

        if (rdev->bufferspace != NULL) {
            ui_error("Warning! Recording device %s seems to be a realtime device!");
        }

        if (rdev->init) {
            channels_cap = snddata.sound_output_channels;
            if (rdev->init(recparam, &speed, &fragsize, &fragnr, &channels_cap)) {
                ui_error("initialization failed for device `%s'.", rdev->name);
                resources_set_string("SoundRecordDeviceName", "");
                return 0;
            }

            if (sample_rate != speed
                || snddata.fragsize != fragsize
                || snddata.fragnr != fragnr
                || snddata.sound_output_channels != channels_cap) {
                ui_error("The recording device doesn't support current sound parameters");
                rdev->close();
                resources_set_string("SoundRecordDeviceName", "");
            } else {
                snddata.recdev = rdev;
                log_message(sound_log, "Opened recording device device `%s'", rdev->name);
            }
        }
    }
    return 0;
}

/* close sid */
void sound_close(void)
{
    if (snddata.playdev) {
        log_message(sound_log, "Closing device `%s'", snddata.playdev->name);
        if (snddata.playdev->close) {
            snddata.playdev->close();
        }
        snddata.playdev = NULL;
    }

    if (snddata.recdev) {
        log_message(sound_log, "Closing recording device `%s'", snddata.recdev->name);
        if (snddata.recdev->close) {
            snddata.recdev->close();
        }
        snddata.recdev = NULL;
    }

    sid_close();

    snddata.prevused = snddata.prevfill = 0;

    sdev_open = FALSE;
    sound_state_changed = FALSE;

    if (temp_buffer) {
        lib_free(temp_buffer);
        temp_buffer = NULL;
    }

    /* Closing the sound device might take some time, and displaying
       UI dialogs certainly does. */
    vsync_suspend_speed_eval();
}

/* run sid */
static int sound_run_sound(void)
{
    int nr = 0, i;
    int delta_t = 0;
    int16_t *bufferptr;
    static int overflow_warning_count = 0;

    /* XXX: implement the exact ... */
    if (!playback_enabled || (suspend_time > 0 && disabletime)) {
        return 1;
    }

    if (!snddata.playdev) {
        i = sound_open();
        if (i) {
            return i;
        }
    }

    /* Handling of cycle based sound engines. */
    if (cycle_based) {
        delta_t = maincpu_clk - snddata.lastclk;
        bufferptr = snddata.buffer + snddata.bufptr * snddata.sound_output_channels;
        nr = sound_machine_calculate_samples(snddata.psid,
                                             bufferptr,
                                             SOUND_BUFSIZE - snddata.bufptr,
                                             snddata.sound_output_channels,
                                             snddata.sound_chip_channels,
                                             &delta_t);
        if (delta_t) {
            if (overflow_warning_count < 25) {
                log_warning(sound_log, "%s", "Sound buffer overflow (cycle based)");
                overflow_warning_count++;
            } else {
                if (overflow_warning_count == 25) {
                    log_warning(sound_log, "Buffer overflow warning repeated 25 times, will now be ignored");
                    overflow_warning_count++;
                }
            }
        }
    } else {
        /* Handling of sample based sound engines. */
        nr = (int)((SOUNDCLK_CONSTANT(maincpu_clk) - snddata.fclk)
                   / snddata.clkstep);
        if (!nr) {
            return 0;
        }
        if (snddata.bufptr + nr > SOUND_BUFSIZE) {
#ifndef ANDROID_COMPILE
            return sound_error("Sound buffer overflow.");
#else
            return 0;
#endif
        }
        bufferptr = snddata.buffer + snddata.bufptr * snddata.sound_output_channels;
        sound_machine_calculate_samples(snddata.psid,
                                        bufferptr,
                                        nr,
                                        snddata.sound_output_channels,
                                        snddata.sound_chip_channels,
                                        &delta_t);
        snddata.fclk += nr * snddata.clkstep;
    }

    if (amp < 4096) {
        if (amp) {
            for (i = 0; i < (nr * snddata.sound_output_channels); i++) {
                bufferptr[i] = bufferptr[i] * amp / 4096;
            }
        } else {
            memset(bufferptr, 0, nr * snddata.sound_output_channels * sizeof(int16_t));
        }
    }

    snddata.bufptr += nr;
    snddata.lastclk = maincpu_clk;

    return 0;
}

/* reset sid */
void sound_reset(void)
{
    int c;

    snddata.fclk = SOUNDCLK_CONSTANT(maincpu_clk);
    snddata.wclk = maincpu_clk;
    snddata.lastclk = maincpu_clk;
    snddata.bufptr = 0;         /* ugly hack! */
    for (c = 0; c < snddata.sound_chip_channels; c++) {
        if (snddata.psid[c]) {
            sound_machine_reset(snddata.psid[c], maincpu_clk);
        }
    }
}

static void prevent_clk_overflow_callback(CLOCK sub, void *data)
{
    int c;

    snddata.lastclk -= sub;
    snddata.fclk -= SOUNDCLK_CONSTANT(sub);
    snddata.wclk -= sub;
    for (c = 0; c < snddata.sound_chip_channels; c++) {
        if (snddata.psid[c]) {
            sound_machine_prevent_clk_overflow(snddata.psid[c], sub);
        }
    }
}

/* flush all generated samples from buffer to sounddevice. adjust sid runspeed
   to match real running speed of program */
double sound_flush()
{
    int c, i, nr, space = 0, used;
    int j;
    static int drained_warning_count = 0;
    char *state;
    static time_t prev;
    time_t now;

    if (!playback_enabled) {
        if (sdev_open) {
            sound_close();
        }
        return 0;
    }

    if (sound_state_changed) {
        if (sdev_open) {
            sound_close();
        }
        sound_state_changed = FALSE;
    }

    if (suspend_time > 0) {
        enablesound();
    }
    if (sound_run_sound()) {
        return 0;
    }

    if (sid_state_changed) {
        if (sid_init() != 0) {
            return 0;
        }
        sid_state_changed = FALSE;
    }

    if (warp_mode_enabled && snddata.recdev == NULL) {
        snddata.bufptr = 0;
        return 0;
    }
    sound_resume();

    if (snddata.playdev->flush) {
        state = sound_machine_dump_state(snddata.psid[0]);
        i = snddata.playdev->flush(state);
        lib_free(state);
        if (i) {
            sound_error("cannot flush.");
            return 0;
        }
    }

    /* Calculate the number of samples to flush - whole fragments. */
    nr = snddata.bufptr - snddata.bufptr % snddata.fragsize;
    if (!nr) {
        return 0;
    }

    /* adjust speed */
    if (snddata.playdev->bufferspace) {
        space = snddata.playdev->bufferspace();
        if (space < 0 || space > snddata.bufsize) {
            log_warning(sound_log, "fragment problems %d %d", space, snddata.bufsize);
            sound_error("fragment problems.");
            return 0;
        }
        /* we only write complete fragments, sound drivers that can tell
         * better accuracy aren't utilized at this stage. */
        space -= space % snddata.fragsize;

        used = snddata.bufsize - space;
        /* buffer emptied during vsync? Looks like underrun. */
        if (used < snddata.fragsize) {
            if (suspend_time > 0) {
                now = time(0);
                if (now == prev) {
                    suspendsound("buffer overruns");
                    return 0;
                }
                prev = now;
            }

            /* Calculate unused space in buffer, accounting for data we are
             * about to write. */
            j = snddata.bufsize - nr;

            /* Fill up sound hardware buffer. */
            if (j > 0) {
                fill_buffer(j, 0);
            }
            snddata.prevfill = j;

            /* Fresh start for vsync. */
            if (drained_warning_count < 25) {
                log_warning(sound_log, "Buffer drained");
                drained_warning_count++;
            } else {
                if (drained_warning_count == 25) {
                    log_warning(sound_log, "Buffer drained warning repeated 25 times, will now be ignored");
                    drained_warning_count++;
                }
            }
            vsync_sync_reset();
        }
        if (cycle_based || speed_adjustment_setting != SOUND_ADJUST_ADJUSTING) {
            if (speed_percent > 0) {
                snddata.clkfactor = SOUNDCLK_CONSTANT(speed_percent) / 100;
            }
        } else {
            if (snddata.prevfill) {
                snddata.prevused = used;
            }
            snddata.clkfactor = SOUNDCLK_MULT(snddata.clkfactor,
                                              SOUNDCLK_CONSTANT(1.0)
                                              + (SOUNDCLK_CONSTANT(0.9)
                                                 * (used - snddata.prevused))
                                              / snddata.bufsize);
        }
        snddata.prevused = used;
        snddata.prevfill = 0;

        if (!cycle_based && speed_adjustment_setting != SOUND_ADJUST_EXACT
            && snddata.recdev == NULL) {
            snddata.clkfactor = SOUNDCLK_MULT(snddata.clkfactor,
                                              SOUNDCLK_CONSTANT(0.9)
                                              + ((used + nr)
                                                 * SOUNDCLK_CONSTANT(0.12))
                                              / snddata.bufsize);
        }
        snddata.clkstep = SOUNDCLK_MULT(snddata.origclkstep,
                                        snddata.clkfactor);
        if (SOUNDCLK_CONSTANT(cycles_per_rfsh) / snddata.clkstep
            >= snddata.bufsize) {
            if (suspend_time > 0) {
                suspendsound("running too slow");
            } else {
                sound_error("running too slow.");
            }
            return 0;
        }
        /* Not all sound drivers block during writing. We must avoid
         * overwriting. */
        if (nr > space) {
            nr = space; /* warning: "space" may have become 0 due to fragment size
                           alignment */
        }
    }

    if (nr) {
        /* Flush buffer, all channels are already mixed into it. */
        if (snddata.playdev->write(snddata.buffer, nr * snddata.sound_output_channels)) {
            sound_error("write to sound device failed.");
            return 0;
        }

        if (snddata.recdev) {
            if (snddata.recdev->write(snddata.buffer, nr * snddata.sound_output_channels)) {
                sound_error("write to sound device failed.");
                return 0;
            }
        }
    }

    /* "No Limit" speed support: nuke the accumulated buffer. */
    if (speed_percent == 0) {
        nr = snddata.bufptr;
    }

    snddata.bufptr -= nr;

    for (c = 0; c < snddata.sound_output_channels; c++) {
        snddata.lastsample[c] = snddata.buffer[(nr - 1) * snddata.sound_output_channels + c];
        for (i = 0; i < snddata.bufptr; i++) {
            snddata.buffer[i * snddata.sound_output_channels + c] =
                snddata.buffer[(i + nr) * snddata.sound_output_channels + c];
        }
    }

    if (snddata.playdev->bufferspace
        && (cycle_based || speed_adjustment_setting == SOUND_ADJUST_EXACT))
    {
        /* finetune VICE timer */
        /* Read bufferspace() just before returning to minimize the possibility
           of getting interrupted before vsync delay calculation. */
        /* Aim for utilization of bufsize - fragsize. */
        int remspace =
            snddata.playdev->bufferspace() - snddata.bufptr;
        /* Return delay in seconds. */
        return (double)remspace / sample_rate;
    }

    return 0;
}

/* suspend sid (eg. before pause) */
void sound_suspend(void)
{
    if (!snddata.playdev) {
        return;
    }

    if (snddata.playdev->write && !snddata.issuspended
        && snddata.playdev->need_attenuation) {
        /* fill buffer, but avoid overwriting */
        if (!snddata.playdev->bufferspace
            || snddata.playdev->bufferspace() >= snddata.fragsize) {
            fill_buffer(snddata.fragsize, -1);
        } else {
            log_warning(sound_log, "Buffer full during suspend");
        }
        /* fill_buffer() can call sound_close() */
        if (!snddata.playdev) {
            return;
        }
    }

    if (snddata.playdev->suspend && !snddata.issuspended) {
        if (snddata.playdev->suspend()) {
            return;
        }
    }
    snddata.issuspended = 1;
}

/* resume sid */
void sound_resume(void)
{
    if (!snddata.playdev) {
        return;
    }

    if (snddata.issuspended) {
        if (snddata.playdev->resume) {
            snddata.issuspended = snddata.playdev->resume();
        } else {
            snddata.issuspended = 0;
        }

        if (snddata.playdev->write && !snddata.issuspended
            && snddata.playdev->need_attenuation) {
            fill_buffer(snddata.fragsize, 1);
        }
    }
}

/* set PAL/NTSC clock speed */
void sound_set_machine_parameter(long clock_rate, long ticks_per_frame)
{
    sid_state_changed = TRUE;

    cycles_per_sec = clock_rate;
    cycles_per_rfsh = ticks_per_frame;
    rfsh_per_sec = (1.0 / ((double)cycles_per_rfsh / (double)cycles_per_sec));
}

/* initialize sid at program start -time */
void sound_init(unsigned int clock_rate, unsigned int ticks_per_frame)
{
    char *devlist, *tmplist;
    int i;

    sound_log = log_open("Sound");

    sound_state_changed = FALSE;
    sid_state_changed = FALSE;

    cycles_per_sec = clock_rate;
    cycles_per_rfsh = ticks_per_frame;
    rfsh_per_sec = (1.0 / ((double)cycles_per_rfsh / (double)cycles_per_sec));

    clk_guard_add_callback(maincpu_clk_guard, prevent_clk_overflow_callback,
                           NULL);

    devlist = lib_strdup("");

    for (i = 0; sound_register_devices[i].name; i++) {
        sound_register_devices[i].init();
        tmplist = lib_msprintf("%s %s", devlist, sound_register_devices[i].name);
        lib_free(devlist);
        devlist = tmplist;
    }

    log_message(sound_log, "Available sound devices:%s", devlist);

    lib_free(devlist);
}

long sound_sample_position(void)
{
    return (snddata.clkstep == 0)
           ? 0 : (long)((SOUNDCLK_CONSTANT(maincpu_clk) - snddata.fclk)
                        / snddata.clkstep);
}

int sound_dump(int chipno)
{
    if (chipno >= snddata.sound_chip_channels) {
        return -1;
    }
    mon_out("%s\n", sound_machine_dump_state(snddata.psid[chipno]));
    return 0;
}

int sound_read(uint16_t addr, int chipno)
{
    if (sound_run_sound()) {
        return -1;
    }

    if (chipno >= snddata.sound_chip_channels) {
        return -1;
    }

    return sound_machine_read(snddata.psid[chipno], addr);
}

void sound_store(uint16_t addr, uint8_t val, int chipno)
{
    int i;

    if (sound_run_sound()) {
        return;
    }

    if (chipno >= snddata.sound_chip_channels) {
        return;
    }

    sound_machine_store(snddata.psid[chipno], addr, val);

    if (!snddata.playdev->dump) {
        return;
    }

    i = snddata.playdev->dump(addr, val, maincpu_clk - snddata.wclk);

    snddata.wclk = maincpu_clk;

    if (i) {
        sound_error("store to sounddevice failed.");
    }
}


void sound_set_relative_speed(int value)
{
    if (value != speed_percent) {
        sid_state_changed = TRUE;
    }

    speed_percent = value;
}

void sound_set_warp_mode(int value)
{
    warp_mode_enabled = value;

    if (value) {
        sound_suspend();
    } else {
        sound_resume();
    }
}

void sound_snapshot_prepare(void)
{
    /* Update lastclk.  */
    sound_run_sound();
}

void sound_snapshot_finish(void)
{
    snddata.lastclk = maincpu_clk;
}

void sound_dac_init(sound_dac_t *dac, int speed)
{
    /* 20 dB/Decade high pass filter, cutoff at 5 Hz. For DC offset filtering. */
    dac->alpha = (float)(0.0318309886 / (0.0318309886 + 1.0 / (float)speed));
    dac->value = 0;
    dac->output = 0.0;
}

/* FIXME: this should use bandlimited step synthesis. Sadly, VICE does not
 * have an easy-to-use infrastructure for blep generation. We should write
 * this code. */
int sound_dac_calculate_samples(sound_dac_t *dac, int16_t *pbuf, int value, int nr, int soc, int cs)
{
    int i, sample;
    int off = 0;
    /* A simple high pass digital filter is employed here to get rid of the DC offset,
       which would cause distortion when mixed with other signal. This filter is formed
       on the actual hardware by the combination of output decoupling capacitor and load
       resistance.
    */
    if (nr) {
        dac->output = dac->alpha * (dac->output + (float)(value - dac->value));
        dac->value = value;
        sample = (int)dac->output;
        if (!sample) {
            return nr;
        }
        if (cs & 1) {
            pbuf[off] = sound_audio_mix(pbuf[off], sample);
        }
        if (cs & 2) {
            pbuf[off + 1] = sound_audio_mix(pbuf[off + 1], sample);
        }
        off += soc;
    }

    for (i = 1; i < nr; i++) {
        dac->output *= dac->alpha;
        sample = (int)dac->output;
        if (cs & 1) {
            pbuf[off] = sound_audio_mix(pbuf[off], sample);
        }
        if (cs & 2) {
            pbuf[off + 1] = sound_audio_mix(pbuf[off + 1], sample);
        }
        off += soc;
    }
    return nr;
}

/* recording related functions, equivalent to screenshot_... */
void sound_stop_recording(void)
{
    resources_set_string("SoundRecordDeviceName", "");
}

int sound_is_recording(void)
{
    return (strlen(recorddevice_name) > 0);
}
