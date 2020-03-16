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
static void userport_digimax_store_pbx(uint8_t value);
static void userport_digimax_store_pa2(uint8_t value);
static void userport_digimax_store_pa3(uint8_t value);
static int userport_digimax_write_snapshot_module(snapshot_t *s);
static int userport_digimax_read_snapshot_module(snapshot_t *s);

static userport_device_t digimax_device = {
    USERPORT_DEVICE_DIGIMAX,    /* device id */
    "Userport DigiMAX",         /* device name */
    NULL,                       /* NO read pb0-pb7 function */
    userport_digimax_store_pbx, /* store pb0-pb7 function */
    NULL,                       /* NO read pa2 pin function */
    userport_digimax_store_pa2, /* store pa2 pin function */
    NULL,                       /* NO read pa3 pin function */
    userport_digimax_store_pa3, /* store pa3 pin function */
    1,                          /* pc pin is needed */
    NULL,                       /* NO store sp1 pin function */
    NULL,                       /* NO read sp1 pin function */
    NULL,                       /* NO store sp2 pin function */
    NULL,                       /* NO read sp2 pin function */
    "UserportDigiMax",          /* resource used by the device */
    0xff,                       /* return value from a read, not used since the device is write only */
    0x0f,                       /* validity mask of the device, doesn't change */
    0,                          /* device involved in a read collision, to be filled in by the collision detection system */
    0                           /* a tag to indicate the order of insertion */
};

static userport_snapshot_t digimax_snapshot = {
    USERPORT_DEVICE_DIGIMAX,
    userport_digimax_write_snapshot_module,
    userport_digimax_read_snapshot_module
};

static userport_device_list_t *userport_digimax_list_item = NULL;

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

static void userport_digimax_store_pbx(uint8_t value)
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

static int set_digimax_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!digimax_sound_chip.chip_enabled && val) {
        userport_digimax_list_item = userport_device_register(&digimax_device);
        digimax_sound_chip.chip_enabled = 1;
    } else if (digimax_sound_chip.chip_enabled && !val) {
        if (userport_digimax_list_item != NULL) {
            userport_device_unregister(userport_digimax_list_item);
            userport_digimax_list_item = NULL;
        }
        digimax_sound_chip.chip_enabled = 0;
    }
    return 0;
}

/* ---------------------------------------------------------------------*/

static const resource_int_t resources_int[] = {
    { "UserportDIGIMAX", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &digimax_sound_chip.chip_enabled, set_digimax_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int userport_digimax_resources_init(void)
{
    userport_snapshot_register(&digimax_snapshot);

    return resources_register_int(resources_int);
}

/* ---------------------------------------------------------------------*/

static const cmdline_option_t cmdline_options[] =
{
    { "-userportdigimax", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "UserportDIGIMAX", (resource_value_t)1,
      NULL, "Enable the userport DigiMAX device" },
    { "+userportdigimax", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "UserportDIGIMAX", (resource_value_t)0,
      NULL, "Disable the userport DigiMAX device" },
    CMDLINE_LIST_END
};

int userport_digimax_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
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

static char snap_module_name[] = "USERPORT_DIGIMAX";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

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
        || (SMW_B(m, snd.voice0) < 0)
        || (SMW_B(m, snd.voice1) < 0)
        || (SMW_B(m, snd.voice2) < 0)
        || (SMW_B(m, snd.voice3) < 0)) {
        snapshot_module_close(m);
        return -1;
    }
    return snapshot_module_close(m);
}

static int userport_digimax_read_snapshot_module(snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    /* enable device */
    set_digimax_enabled(1, NULL);

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
        || (SMR_B(m, &snd.voice0) < 0)
        || (SMR_B(m, &snd.voice1) < 0)
        || (SMR_B(m, &snd.voice2) < 0)
        || (SMR_B(m, &snd.voice3) < 0)) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
