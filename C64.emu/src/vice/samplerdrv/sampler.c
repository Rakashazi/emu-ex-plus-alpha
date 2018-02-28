/*
 * sampler.c - audio input driver manager.
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

#include <string.h>

#include "cmdline.h"
#include "file_drv.h"
#include "lib.h"
#include "resources.h"
#include "sampler.h"
#include "translate.h"
#include "uiapi.h"
#include "util.h"

#ifdef USE_PORTAUDIO
#include "portaudio_drv.h"
#endif

#ifdef USE_PORTAUDIO
#define DEFAULT_DEVICE SAMPLER_DEVICE_PORTAUDIO
#else
#define DEFAULT_DEVICE SAMPLER_DEVICE_FILE
#endif

/* used to build a resource string via lib_stralloc() and util_concat() calls,
 * gets free'd in sampler_resources_shutdown() */
static char *cmdline_devices = NULL;

static int current_sampler = DEFAULT_DEVICE;
static int sampler_status = SAMPLER_CLOSED;

/* sampler gain in % */
static int sampler_gain = 100;

static sampler_device_t devices[SAMPLER_MAX_DEVICES];

sampler_device_t *sampler_get_devices(void)
{
    return devices;
}

static void sampler_init(void)
{
    memset(devices, 0, sizeof(devices));

    fileaudio_init();

#ifdef USE_PORTAUDIO
    portaudio_init();
#endif
}

/* ------------------------------------------------------------------------- */

static inline BYTE calc_gain(BYTE val)
{
    int tmp = (val - 0x80);

    tmp = tmp * sampler_gain / 100;

    if (tmp > 127) {
        tmp = 127;
    }

    if (tmp < -128) {
        tmp = -128;
    }

    return (BYTE)(tmp + 0x80);
}

/* ------------------------------------------------------------------------- */

static char *current_sampler_device = NULL;

void sampler_reset(void)
{
    if (devices[current_sampler].reset) {
        devices[current_sampler].reset();
    }
}

void sampler_start(int channels, char *devname)
{
    if (current_sampler_device) {
        ui_error(translate_text(IDGS_SAMPLER_USED_BY), current_sampler_device);
    } else {
        if (devices[current_sampler].open) {
            devices[current_sampler].open(channels);
            sampler_status = SAMPLER_STARTED | (channels << 1);
            current_sampler_device = devname;
        }
    }
}

void sampler_stop(void)
{
    if (devices[current_sampler].close) {
        devices[current_sampler].close();
        sampler_status = SAMPLER_CLOSED;
        current_sampler_device = NULL;
    }
}

BYTE sampler_get_sample(int channel)
{
    if (devices[current_sampler].get_sample) {
        if (sampler_gain == 100) {
            return devices[current_sampler].get_sample(channel);
        }
        return calc_gain(devices[current_sampler].get_sample(channel));
    }
    return 0x80;
}

/* ------------------------------------------------------------------------- */

void sampler_device_register(sampler_device_t *device, int id)
{
    if (id >= SAMPLER_MAX_DEVICES || id < 0) {
        return;
    }

    devices[id].name = device->name;
    devices[id].open = device->open;
    devices[id].close = device->close;
    devices[id].get_sample = device->get_sample;
    devices[id].shutdown = device->shutdown;
    devices[id].resources_init = device->resources_init;
    devices[id].cmdline_options_init = device->cmdline_options_init;
    devices[id].reset = device->reset;
}

/* ------------------------------------------------------------------------- */

static int set_sampler_device(int id, void *param)
{
    int channels;

    /* 1st some sanity checks */
    if (id < 0 || id >= SAMPLER_MAX_DEVICES) {
        return -1;
    }

    /* Nothing changes */
    if (id == current_sampler) {
        return 0;
    }

    /* check if id is registered */
    if (!devices[id].name) {
        return -1;
    }

    if (sampler_status & SAMPLER_STARTED) {
        channels = sampler_status >> 1;
        sampler_stop();
        current_sampler = id;
        sampler_start(channels, current_sampler_device);
    } else {
        current_sampler = id;
    }

    return 0;
}

static int set_sampler_gain(int gain, void *param)
{
    if (gain < 1 || gain > 200) {
        return -1;
    }

    sampler_gain = gain;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "SamplerDevice", DEFAULT_DEVICE, RES_EVENT_NO, NULL,
      &current_sampler, set_sampler_device, NULL },
    { "SamplerGain", 100, RES_EVENT_NO, NULL,
      &sampler_gain, set_sampler_gain, NULL },
    RESOURCE_INT_LIST_END
};

int sampler_resources_init(void)
{
    int i;
    sampler_init();

    for (i = 0; i < SAMPLER_MAX_DEVICES; ++i) {
        if (devices[i].resources_init) {
            if (devices[i].resources_init() < 0) {
                return -1;
            }
        }
    }

    return resources_register_int(resources_int);
}

void sampler_resources_shutdown(void)
{
    int i;

    for (i = 0; i < SAMPLER_MAX_DEVICES; ++i) {
        if (devices[i].shutdown) {
            devices[i].shutdown();
        }
    }
    if (cmdline_devices != NULL) {
        lib_free(cmdline_devices);
        cmdline_devices = NULL;
    }
    fileaudio_shutdown();
}

/* ------------------------------------------------------------------------- */


static cmdline_option_t cmdline_options[] =
{
    { "-samplerdev", SET_RESOURCE, 1,
      NULL, NULL, "SamplerDevice", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDGS_DEVICE, IDCLS_SPECIFY_SAMPLER_DEVICE,
      NULL, NULL },
    { "-samplergain", SET_RESOURCE, 1,
      NULL, NULL, "SamplerGain", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_GAIN, IDCLS_SAMPLER_GAIN_IN_PERCENT,
      NULL, NULL },
    CMDLINE_LIST_END
};

int sampler_cmdline_options_init(void)
{
    int i;
    int started = 0;
    char *temp = NULL;
    char number[4];

    cmdline_devices = lib_stralloc(". (");

    for (i = 0; i < SAMPLER_MAX_DEVICES; ++i) {
        if (devices[i].name) {
            sprintf(number, "%d", i);
            if (!started) {
                temp = util_concat(cmdline_devices, number, ": ", devices[i].name, NULL);
                started = 1;
            } else {
                temp = util_concat(cmdline_devices, ", ", number, ": ", devices[i].name, NULL);
            }
            lib_free(cmdline_devices);
            cmdline_devices = temp;
        }
    }
    temp = util_concat(cmdline_devices, ")", NULL);
    lib_free(cmdline_devices);
    cmdline_devices = temp;

    cmdline_options[0].description = cmdline_devices;

    for (i = 0; i < SAMPLER_MAX_DEVICES; ++i) {
        if (devices[i].cmdline_options_init) {
            if (devices[i].cmdline_options_init() < 0) {
                return -1;
            }
        }
    }

    return cmdline_register_options(cmdline_options);
}
