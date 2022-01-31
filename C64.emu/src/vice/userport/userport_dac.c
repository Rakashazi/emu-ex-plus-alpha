/*
 * userport_dac.c - Generic userport 8bit DAC emulation.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 * Filtering by
 *  Kajtar Zsolt <soci@c64.rulez.org>
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

/* 8bit userport DAC (C64/C128/CBM2/PET/PLUS4/VIC20)

C64/C128 | CBM2 | PET | PLUS4 | VIC20 | NAME
--------------------------------------------
    C    |  14  |  C  |   B   |   C   | PB0 -> D0
    D    |  13  |  D  |   K   |   D   | PB1 -> D1
    E    |  12  |  E  |   4   |   E   | PB2 -> D2
    F    |  11  |  F  |   5   |   F   | PB3 -> D3
    H    |  10  |  H  |   6   |   H   | PB4 -> D4
    J    |   9  |  J  |   7   |   J   | PB5 -> D5
    K    |   8  |  K  |   J   |   K   | PB6 -> D6
    L    |   7  |  L  |   F   |   L   | PB7 -> D7
*/

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "lib.h"
#include "maincpu.h"
#include "resources.h"
#include "snapshot.h"
#include "sound.h"
#include "uiapi.h"
#include "joyport.h"
#include "userport.h"
#include "userport_dac.h"

/* ------------------------------------------------------------------------- */

static sound_dac_t userport_dac_dac;

/* Some prototypes are needed */
static int userport_dac_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
static int userport_dac_sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int sound_output_channels, int sound_chip_channels, CLOCK *delta_t);
static void userport_dac_sound_machine_store(sound_t *psid, uint16_t addr, uint8_t val);
static uint8_t userport_dac_sound_machine_read(sound_t *psid, uint16_t addr);
static void userport_dac_sound_reset(sound_t *psid, CLOCK cpu_clk);

static int userport_dac_sound_machine_cycle_based(void)
{
    return 0;
}

static int userport_dac_sound_machine_channels(void)
{
    return 1;
}

/* Userport DAC device sound chip */
static sound_chip_t userport_dac_sound_chip = {
    NULL,                                         /* NO sound chip open function */ 
    userport_dac_sound_machine_init,              /* sound chip init function */
    NULL,                                         /* NO sound chip close function */
    userport_dac_sound_machine_calculate_samples, /* sound chip calculate samples function */
    userport_dac_sound_machine_store,             /* sound chip store function */
    userport_dac_sound_machine_read,              /* sound chip read function */
    userport_dac_sound_reset,                     /* sound chip reset function */
    userport_dac_sound_machine_cycle_based,       /* sound chip 'is_cycle_based()' function, chip is NOT cycle based */
    userport_dac_sound_machine_channels,          /* sound chip 'get_amount_of_channels()' function, sound chip has 1 channel */
    0                                             /* sound chip enabled flag, toggled upon device (de-)activation */
};

static uint16_t userport_dac_sound_chip_offset = 0;

void userport_dac_sound_chip_init(void)
{
    userport_dac_sound_chip_offset = sound_chip_register(&userport_dac_sound_chip);
}

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void userport_dac_store_pbx(uint8_t value, int pulse);
static int userport_dac_write_snapshot_module(snapshot_t *s);
static int userport_dac_read_snapshot_module(snapshot_t *s);
static int userport_dac_enable(int val);

static userport_device_t dac_device = {
    "Userport DAC",                     /* device name */
    JOYSTICK_ADAPTER_ID_NONE,           /* NOT a joystick adapter */
    USERPORT_DEVICE_TYPE_AUDIO_OUTPUT,  /* device is an audio output */
    userport_dac_enable,                /* enable function */
    NULL,                               /* NO read pb0-pb7 function */
    userport_dac_store_pbx,             /* store pb0-pb7 function */
    NULL,                               /* NO read pa2 pin function */
    NULL,                               /* NO store pa2 pin function */
    NULL,                               /* NO read pa3 pin function */
    NULL,                               /* NO store pa3 pin function */
    0,                                  /* pc pin is NOT needed */
    NULL,                               /* NO store sp1 pin function */
    NULL,                               /* NO read sp1 pin function */
    NULL,                               /* NO store sp2 pin function */
    NULL,                               /* NO read sp2 pin function */
    NULL,                               /* NO reset function */
    NULL,                               /* NO powerup function */
    userport_dac_write_snapshot_module, /* snapshot write function */
    userport_dac_read_snapshot_module   /* snapshot read function */
};

/* ------------------------------------------------------------------------- */

static int userport_dac_enable(int value)
{
    int val = (value) ? 1 : 0;

    userport_dac_sound_chip.chip_enabled = val;

    return 0;
}

int userport_dac_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_DAC, &dac_device);
}

/* ---------------------------------------------------------------------*/

static uint8_t userport_dac_sound_data;

static void userport_dac_store_pbx(uint8_t value, int pulse)
{
    userport_dac_sound_data = value;
    sound_store(userport_dac_sound_chip_offset, value, 0);
}

struct userport_dac_sound_s {
    uint8_t voice0;
};

static struct userport_dac_sound_s snd;

static int userport_dac_sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int soc, int scc, CLOCK *delta_t)
{
    return sound_dac_calculate_samples(&userport_dac_dac, pbuf, (int)snd.voice0 * 128, nr, soc, (soc > 1) ? 3 : 1);
}

static int userport_dac_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    sound_dac_init(&userport_dac_dac, speed);
    snd.voice0 = 0;

    return 1;
}

static void userport_dac_sound_machine_store(sound_t *psid, uint16_t addr, uint8_t val)
{
    snd.voice0 = val;
}

static uint8_t userport_dac_sound_machine_read(sound_t *psid, uint16_t addr)
{
    return userport_dac_sound_data;
}

static void userport_dac_sound_reset(sound_t *psid, CLOCK cpu_clk)
{
    snd.voice0 = 0;
    userport_dac_sound_data = 0;
}

/* ---------------------------------------------------------------------*/

/* USERPORT_DAC snapshot module format:

   type  | name       | description
   --------------------------------
   BYTE  | sound data | sound data
   BYTE  | voice      | voice
 */

static const char snap_module_name[] = "UPDAC";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int userport_dac_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, userport_dac_sound_data) < 0
        || SMW_B(m, snd.voice0) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    return snapshot_module_close(m);
}

static int userport_dac_read_snapshot_module(snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || SMR_B(m, &userport_dac_sound_data) < 0
        || SMR_B(m, &snd.voice0) < 0) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
