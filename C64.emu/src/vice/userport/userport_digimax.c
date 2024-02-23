/*
 * userport_digimax.c - Digimax DAC userport device emulation.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "lib.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "snapshot.h"
#include "sound.h"
#include "uiapi.h"
#include "joyport.h"
#include "userport.h"
#include "userport_digimax.h"
#include "util.h"

#include "digimaxcore.c"

/*
    Digimax userport device

    This device is an 8bit 4-channel digital sound output
    interface.

C64/C128 | CBM2 | TLC7226 DAC | NOTES
-------------------------------------
    8    |  5   |      15     | /PC2 -> /Write
    9    |  4   |      16     | /PA3 -> A1
    C    | 14   |      14     | PB0 -> DB0
    D    | 13   |      13     | PB1 -> DB1
    E    | 12   |      12     | PB2 -> DB2
    F    | 11   |      11     | PB3 -> DB3
    H    | 10   |      10     | PB4 -> DB4
    J    |  9   |       9     | PB5 -> DB5
    K    |  8   |       8     | PB6 -> DB6
    L    |  7   |       7     | PB7 -> DB7
    M    |  2   |      17     | PA2 -> A0
*/

/* Some prototypes are needed */
static void userport_digimax_store_pbx(uint8_t value, int pulse);
static void userport_digimax_store_pa2(uint8_t value);
static void userport_digimax_store_pa3(uint8_t value);
static int userport_digimax_write_snapshot_module(snapshot_t *s);
static int userport_digimax_read_snapshot_module(snapshot_t *s);
static int digimax_enable(int value);

static userport_device_t digimax_device = {
    "Userport DigiMAX",                     /* device name */
    JOYSTICK_ADAPTER_ID_NONE,               /* NOT a joystick adapter */
    USERPORT_DEVICE_TYPE_AUDIO_OUTPUT,      /* device is an audio output */
    digimax_enable,                         /* enable function */
    NULL,                                   /* NO read pb0-pb7 function */
    userport_digimax_store_pbx,             /* store pb0-pb7 function */
    NULL,                                   /* NO read pa2 pin function */
    userport_digimax_store_pa2,             /* store pa2 pin function */
    NULL,                                   /* NO read pa3 pin function */
    userport_digimax_store_pa3,             /* store pa3 pin function */
    1,                                      /* pc pin is needed */
    NULL,                                   /* NO store sp1 pin function */
    NULL,                                   /* NO read sp1 pin function */
    NULL,                                   /* NO store sp2 pin function */
    NULL,                                   /* NO read sp2 pin function */
    NULL,                                   /* NO reset function */
    NULL,                                   /* NO powerup function */
    userport_digimax_write_snapshot_module, /* snapshot write function */
    userport_digimax_read_snapshot_module   /* snapshot read function */
};

/* ------------------------------------------------------------------------- */

static uint8_t userport_digimax_address = 3;

void userport_digimax_sound_chip_init(void)
{
    digimax_sound_chip_offset = sound_chip_register(&digimax_sound_chip);
}

static void userport_digimax_store_pa2(uint8_t value)
{
    userport_digimax_address &= 2;
    userport_digimax_address |= (value & 1);
}

static void userport_digimax_store_pa3(uint8_t value)
{
    userport_digimax_address &= 1;
    userport_digimax_address |= ((value & 1) << 1);
}

static void userport_digimax_store_pbx(uint8_t value, int pulse)
{
    uint8_t addr = 0;

    switch (userport_digimax_address) {
        case 0x0:
            addr = 2;
            break;
        case 0x4:
            addr = 3;
            break;
        case 0x8:
            addr = 0;
            break;
        case 0xc:
            addr = 1;
            break;
    }

    digimax_sound_data[addr] = value;
    sound_store((uint16_t)(digimax_sound_chip_offset | addr), value, 0);
}

/* ---------------------------------------------------------------------*/

static int digimax_enable(int value)
{
    int val = value ? 1 : 0;

    if (!digimax_sound_chip.chip_enabled && val) {
        digimax_sound_chip.chip_enabled = 1;
    } else if (digimax_sound_chip.chip_enabled && !val) {
        digimax_sound_chip.chip_enabled = 0;
    }
    return 0;
}

/* ---------------------------------------------------------------------*/

int userport_digimax_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_DIGIMAX, &digimax_device);
}

/* ---------------------------------------------------------------------*/

/* USERPORT_DIGIMAX snapshot module format:

   type  | name    | description
   -----------------------------
   BYTE  | address | current register address
   ARRAY | sound   | 4 BYTES of sound data
   BYTE  | voice 0 | voice 0 data
   BYTE  | voice 1 | voice 1 data
   BYTE  | voice 2 | voice 2 data
   BYTE  | voice 3 | voice 3 data
 */

static const char snap_module_name[] = "UPDIGIMAX";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int userport_digimax_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, userport_digimax_address) < 0)
        || (SMW_BA(m, digimax_sound_data, 4) < 0)
        || (SMW_B(m, snd.voice[0]) < 0)
        || (SMW_B(m, snd.voice[1]) < 0)
        || (SMW_B(m, snd.voice[2]) < 0)
        || (SMW_B(m, snd.voice[3]) < 0)) {
        snapshot_module_close(m);
        return -1;
    }
    return snapshot_module_close(m);
}

static int userport_digimax_read_snapshot_module(snapshot_t *s)
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
        || (SMR_B(m, &userport_digimax_address) < 0)
        || (SMR_BA(m, digimax_sound_data, 4) < 0)
        || (SMR_B(m, &snd.voice[0]) < 0)
        || (SMR_B(m, &snd.voice[1]) < 0)
        || (SMR_B(m, &snd.voice[2]) < 0)
        || (SMR_B(m, &snd.voice[3]) < 0)) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
