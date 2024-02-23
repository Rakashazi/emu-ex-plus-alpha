/*
 * sampler.h
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

#ifndef VICE_SAMPLER_H
#define VICE_SAMPLER_H

#include "types.h"

#define SAMPLER_CHANNEL_DEFAULT 0
#define SAMPLER_CHANNEL_1       1
#define SAMPLER_CHANNEL_2       2

#define SAMPLER_OPEN_MONO   1
#define SAMPLER_OPEN_STEREO 2

#define SAMPLER_CLOSED   0
#define SAMPLER_STARTED  1

#define SAMPLER_GAIN_ONE        100     /* 100% */
#define SAMPLER_GAIN_MIN          1
#define SAMPLER_GAIN_MAX        200     /* 200% */
#define SAMPLER_GAIN_DEFAULT    SAMPLER_GAIN_ONE

enum {
    SAMPLER_DEVICE_FILE = 0,
    SAMPLER_DEVICE_PORTAUDIO,

    /* This item always needs to be at the end */
    SAMPLER_MAX_DEVICES
};

typedef struct sampler_device_s {
    const char *name;
    void (*open)(int channels);
    void (*close)(void);
    uint8_t (*get_sample)(int channel);
    void (*shutdown)(void);
    int (*resources_init)(void);
    int (*cmdline_options_init)(void);
    void (*reset)(void);
} sampler_device_t;

void sampler_start(int channels, char *devname);
void sampler_stop(void);
uint8_t sampler_get_sample(int channel);
void sampler_reset(void);

void sampler_device_register(sampler_device_t *device, int id);

int sampler_resources_init(void);
void sampler_resources_shutdown(void);
int sampler_cmdline_options_init(void);

sampler_device_t *sampler_get_devices(void);

#endif
