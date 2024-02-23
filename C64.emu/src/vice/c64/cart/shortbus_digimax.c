/*
 * shortbus_digimax.c - IDE64 Digimax DAC expansion emulation.
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

#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "lib.h"
#include "resources.h"
#include "snapshot.h"
#include "sound.h"
#include "util.h"

#include "digimaxcore.c"
#include "shortbus_digimax.h"

/*
    Digimax Short Bus expansion

    This cartridge is an 8bit 4-channel digital sound output
    interface.

    When inserted into the short bus port the cart uses 4 registers,
    one for each channel. The base address can be relocated
    to be at either $DE40-$DE47 or $DE48-$DE4F.
*/

/* This flag indicates if the IDE64 cart is active */
static int shortbus_digimax_host_active = 0;

/* This flag indicates if the expansion is active,
   real activity depends on the 'host' active flag */
static int shortbus_digimax_expansion_active = 0;

/* DIGIMAX address */
static int shortbus_digimax_address;

static char *shortbus_digimax_address_list = NULL;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static void shortbus_digimax_sound_store(uint16_t addr, uint8_t value);
static uint8_t shortbus_digimax_sound_read(uint16_t addr);

static io_source_t digimax_device = {
    "ShortBus " CARTRIDGE_NAME_DIGIMAX, /* name of the device */
    IO_DETACH_RESOURCE,                 /* use resource to detach the device when involved in a read-collision */
    "SBDIGIMAX",                        /* resource to set to '0' */
    0xde40, 0xde47, 0x03,               /* range for the device, regs:$de40-$de43, mirrors:$de44-$de47 */
    1,                                  /* read is always valid */
    shortbus_digimax_sound_store,       /* store function */
    NULL,                               /* NO poke function */
    shortbus_digimax_sound_read,        /* read function */
    shortbus_digimax_sound_read,        /* peek function */
    NULL,                               /* nothing to dump */
    CARTRIDGE_IDE64,                    /* cartridge ID */
    IO_PRIO_NORMAL,                     /* normal priority, device read needs to be checked for collisions */
    0,                                  /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                      /* NO mirroring */
};

static io_source_list_t *shortbus_digimax_list_item = NULL;

/* ---------------------------------------------------------------------*/

void shortbus_digimax_sound_chip_init(void)
{
    digimax_sound_chip_offset = sound_chip_register(&digimax_sound_chip);
}

static void shortbus_digimax_sound_store(uint16_t addr, uint8_t value)
{
    digimax_sound_data[addr] = value;
    sound_store((uint16_t)(digimax_sound_chip_offset | addr), value, 0);
}

static uint8_t shortbus_digimax_sound_read(uint16_t addr)
{
    uint8_t value = sound_read((uint16_t)(digimax_sound_chip_offset | addr), 0);

    return value;
}

/* ---------------------------------------------------------------------*/

void shortbus_digimax_unregister(void)
{
    if (shortbus_digimax_list_item != NULL) {
        io_source_unregister(shortbus_digimax_list_item);
        shortbus_digimax_list_item = NULL;
        digimax_sound_chip.chip_enabled = 0;
    }
    shortbus_digimax_host_active = 0;
}

void shortbus_digimax_register(void)
{
    if (!digimax_sound_chip.chip_enabled && shortbus_digimax_expansion_active) {
        shortbus_digimax_list_item = io_source_register(&digimax_device);
        digimax_sound_chip.chip_enabled = 1;
    }
    shortbus_digimax_host_active = 1;
}

/* ---------------------------------------------------------------------*/

static int set_shortbus_digimax_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (shortbus_digimax_host_active) {
        if (!digimax_sound_chip.chip_enabled && val) {
            shortbus_digimax_list_item = io_source_register(&digimax_device);
            digimax_sound_chip.chip_enabled = 1;
        } else if (digimax_sound_chip.chip_enabled && !val) {
            if (shortbus_digimax_list_item != NULL) {
                io_source_unregister(shortbus_digimax_list_item);
                shortbus_digimax_list_item = NULL;
            }
            digimax_sound_chip.chip_enabled = 0;
        }
    }
    shortbus_digimax_expansion_active = val;

    return 0;
}

static int set_shortbus_digimax_base(int val, void *param)
{
    int addr = val;
    int old = digimax_sound_chip.chip_enabled;

    if (val == shortbus_digimax_address) {
        return 0;
    }

    if (old) {
        set_shortbus_digimax_enabled(0, NULL);
    }

    switch (addr) {
        case 0xde40:
        case 0xde48:
            digimax_device.start_address = (uint16_t)addr;
            digimax_device.end_address = (uint16_t)(addr + 3);
            break;
        default:
            return -1;
    }

    shortbus_digimax_address = val;

    if (old) {
        set_shortbus_digimax_enabled(1, NULL);
    }
    return 0;
}

void shortbus_digimax_reset(void)
{
}

/* ---------------------------------------------------------------------*/

static const resource_int_t resources_int[] = {
    { "SBDIGIMAX", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &shortbus_digimax_expansion_active, set_shortbus_digimax_enabled, NULL },
    { "SBDIGIMAXbase", 0xde40, RES_EVENT_NO, NULL,
      &shortbus_digimax_address, set_shortbus_digimax_base, NULL },
    RESOURCE_INT_LIST_END
};

int shortbus_digimax_resources_init(void)
{
    return resources_register_int(resources_int);
}

void shortbus_digimax_resources_shutdown(void)
{
    if (shortbus_digimax_address_list) {
        lib_free(shortbus_digimax_address_list);
    }
}

/* ---------------------------------------------------------------------*/

static const cmdline_option_t cmdline_options[] =
{
    { "-sbdigimax", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SBDIGIMAX", (resource_value_t)1,
      NULL, "Enable the Short Bus DigiMAX expansion" },
    { "+sbdigimax", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SBDIGIMAX", (resource_value_t)0,
      NULL, "Disable the Short Bus DigiMAX expansion" },
    CMDLINE_LIST_END
};

static cmdline_option_t base_cmdline_options[] =
{
    { "-sbdigimaxbase", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SBDIGIMAXbase", NULL,
      "<Base address>", NULL },
    CMDLINE_LIST_END
};

int shortbus_digimax_cmdline_options_init(void)
{
    char *temp1;

    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    temp1 = util_gen_hex_address_list(0xde40, 0xde50, 8);
    shortbus_digimax_address_list = util_concat("Base address of the Short Bus DigiMAX expansion. (", temp1, ")", NULL);
    lib_free(temp1);

    base_cmdline_options[0].description = shortbus_digimax_address_list;

    return cmdline_register_options(base_cmdline_options);
}

int shortbus_digimax_enabled(void)
{
    return shortbus_digimax_expansion_active;
}

/* ---------------------------------------------------------------------*/

/* SHORTBUSDIGIMAX snapshot module format:

   type  | name       | description
   --------------------------------
   DWORD | base       | base address
   ARRAY | sound data | 4 BYTES of sound data
   BYTE  | voice 0    | voice 0 state
   BYTE  | voice 1    | voice 1 state
   BYTE  | voice 2    | voice 2 state
   BYTE  | voice 3    | voice 3 state
 */

static const char snap_module_name[] = "SHORTBUSDIGIMAX";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int shortbus_digimax_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_DW(m, (uint32_t)shortbus_digimax_address) < 0
        || SMW_BA(m, digimax_sound_data, 4) < 0
        || SMW_B(m, snd.voice[0]) < 0
        || SMW_B(m, snd.voice[1]) < 0
        || SMW_B(m, snd.voice[2]) < 0
        || SMW_B(m, snd.voice[3]) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int shortbus_digimax_read_snapshot_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;
    int temp_digimax_address;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || SMR_DW_INT(m, &temp_digimax_address) < 0
        || SMR_BA(m, digimax_sound_data, 4) < 0
        || SMR_B(m, &snd.voice[0]) < 0
        || SMR_B(m, &snd.voice[1]) < 0
        || SMR_B(m, &snd.voice[2]) < 0
        || SMR_B(m, &snd.voice[3]) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    /* HACK set address to an invalid value, then use the function */
    shortbus_digimax_address = -1;
    set_shortbus_digimax_base(temp_digimax_address, NULL);

    return set_shortbus_digimax_enabled(1, NULL);

fail:
    snapshot_module_close(m);
    return -1;
}
