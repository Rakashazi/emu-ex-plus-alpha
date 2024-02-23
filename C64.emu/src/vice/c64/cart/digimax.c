/*
 * digimax.c - Digimax DAC cartridge emulation.
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
#include "digimax.h"
#include "export.h"
#include "lib.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "sid.h"
#include "snapshot.h"
#include "sound.h"
#include "uiapi.h"
#include "util.h"

#include "digimaxcore.c"

/*
    Digimax Cartridge

    This cartridge is an 8bit 4-channel digital sound output
    interface.

    When inserted into the cart port the cart uses 4 registers,
    one for each channel. The base address can be relocated
    through the entire I/O-1 and I/O-2 range in 0x20 increments.
*/

/* DIGIMAX address */
int digimax_address;

static char *digimax_address_list = NULL;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static void digimax_sound_store(uint16_t addr, uint8_t value);
static uint8_t digimax_sound_read(uint16_t addr);

static io_source_t digimax_device = {
    CARTRIDGE_NAME_DIGIMAX, /* name of the device */
    IO_DETACH_RESOURCE,     /* use resource to detach the device when involved in a read-collision */
    "DIGIMAX",              /* resource to set to '0' */
    0xde00, 0xde03, 0x03,   /* range for the device, regs:$de00-$de03, range for vic20 will be different */
    1,                      /* read is always valid */
    digimax_sound_store,    /* store function */
    NULL,                   /* NO poke function */
    digimax_sound_read,     /* read function */
    digimax_sound_read,     /* peek function */
    NULL,                   /* nothing to dump */
    CARTRIDGE_DIGIMAX,      /* cartridge ID */
    IO_PRIO_NORMAL,         /* normal priority, device read needs to be checked for collisions */
    0,                      /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE          /* NO mirroring */
};

static io_source_list_t * digimax_list_item = NULL;

static export_resource_t export_res = {
    CARTRIDGE_NAME_DIGIMAX, 0, 0, &digimax_device, NULL, CARTRIDGE_DIGIMAX
};

/* ---------------------------------------------------------------------*/

void digimax_sound_chip_init(void)
{
    digimax_sound_chip_offset = sound_chip_register(&digimax_sound_chip);
}

int digimax_cart_enabled(void)
{
    return digimax_sound_chip.chip_enabled;
}

static void digimax_sound_store(uint16_t addr, uint8_t value)
{
    digimax_sound_data[addr] = value;
    sound_store((uint16_t)(digimax_sound_chip_offset | addr), value, 0);
}

static uint8_t digimax_sound_read(uint16_t addr)
{
    uint8_t value = sound_read((uint16_t)(digimax_sound_chip_offset | addr), 0);

    return value;
}

/* ---------------------------------------------------------------------*/

static int set_digimax_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!digimax_sound_chip.chip_enabled && val) {
        if (export_add(&export_res) < 0) {
            return -1;
        }
        digimax_list_item = io_source_register(&digimax_device);
        digimax_sound_chip.chip_enabled = 1;
    } else if (digimax_sound_chip.chip_enabled && !val) {
        if (digimax_list_item != NULL) {
            export_remove(&export_res);
            io_source_unregister(digimax_list_item);
            digimax_list_item = NULL;
        }
        digimax_sound_chip.chip_enabled = 0;
    }
    return 0;
}

static int set_digimax_base(int val, void *param)
{
    int addr = val;
    int old = digimax_sound_chip.chip_enabled;

    if (val == digimax_address) {
        return 0;
    }

    if (old) {
        set_digimax_enabled(0, NULL);
    }

    switch (addr) {
        case 0xde00:
        case 0xde20:
        case 0xde40:
        case 0xde60:
        case 0xde80:
        case 0xdea0:
        case 0xdec0:
        case 0xdee0:
            if (machine_class != VICE_MACHINE_VIC20) {
                digimax_device.start_address = (uint16_t)addr;
                digimax_device.end_address = (uint16_t)(addr + 3);
                export_res.io1 = &digimax_device;
                export_res.io2 = NULL;
            } else {
                return -1;
            }
            break;
        case 0xdf00:
        case 0xdf20:
        case 0xdf40:
        case 0xdf60:
        case 0xdf80:
        case 0xdfa0:
        case 0xdfc0:
        case 0xdfe0:
            if (machine_class != VICE_MACHINE_VIC20) {
                digimax_device.start_address = (uint16_t)addr;
                digimax_device.end_address = (uint16_t)(addr + 3);
                export_res.io1 = NULL;
                export_res.io2 = &digimax_device;
            } else {
                return -1;
            }
            break;
        case 0x9800:
        case 0x9820:
        case 0x9840:
        case 0x9860:
        case 0x9880:
        case 0x98a0:
        case 0x98c0:
        case 0x98e0:
        case 0x9c00:
        case 0x9c20:
        case 0x9c40:
        case 0x9c60:
        case 0x9c80:
        case 0x9ca0:
        case 0x9cc0:
        case 0x9ce0:
            if (machine_class == VICE_MACHINE_VIC20) {
                digimax_device.start_address = (uint16_t)addr;
                digimax_device.end_address = (uint16_t)(addr + 3);
            } else {
                return -1;
            }
            break;
        default:
            return -1;
    }

    digimax_address = val;

    if (old) {
        set_digimax_enabled(1, NULL);
    }
    return 0;
}

void digimax_reset(void)
{
}

int digimax_enable(void)
{
    return resources_set_int("DIGIMAX", 1);
}

int digimax_disable(void)
{
    return resources_set_int("DIGIMAX", 0);
}

void digimax_detach(void)
{
    resources_set_int("DIGIMAX", 0);
}

/* ---------------------------------------------------------------------*/

static resource_int_t resources_int[] = {
    { "DIGIMAX", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &digimax_sound_chip.chip_enabled, set_digimax_enabled, NULL },
    /*
     * The 'factory_value' gets set a proper default value for the current
     * emu in digimix_resource_init()
     */
    { "DIGIMAXbase", 0xffff, RES_EVENT_NO, NULL,
      &digimax_address, set_digimax_base, NULL },
    RESOURCE_INT_LIST_END
};

int digimax_resources_init(void)
{
    if (machine_class == VICE_MACHINE_VIC20) {
        resources_int[1].factory_value = 0x9800;
    } else {
        resources_int[1].factory_value = 0xde00;
    }
    return resources_register_int(resources_int);
}

void digimax_resources_shutdown(void)
{
    if (digimax_address_list) {
        lib_free(digimax_address_list);
    }
}

/* ---------------------------------------------------------------------*/

static const cmdline_option_t cmdline_options[] =
{
    { "-digimax", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DIGIMAX", (resource_value_t)1,
      NULL, "Enable the DigiMAX cartridge" },
    { "+digimax", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DIGIMAX", (resource_value_t)0,
      NULL, "Disable the DigiMAX cartridge" },
    CMDLINE_LIST_END
};

static cmdline_option_t base_cmdline_options[] =
{
    { "-digimaxbase", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DIGIMAXbase", NULL,
      "<Base address>", NULL },
    CMDLINE_LIST_END
};

int digimax_cmdline_options_init(void)
{
    char *temp1, *temp2;

    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    if (machine_class == VICE_MACHINE_VIC20) {
        temp1 = util_gen_hex_address_list(0x9800, 0x9900, 0x20);
        temp2 = util_gen_hex_address_list(0x9c00, 0x9d00, 0x20);
        digimax_address_list = util_concat("Base address of the DigiMAX cartridge. (", temp1, "/", temp2, ")", NULL);
        lib_free(temp2);
    } else {
        temp1 = util_gen_hex_address_list(0xde00, 0xe000, 0x20);
        digimax_address_list = util_concat("Base address of the DigiMAX cartridge. (", temp1, ")", NULL);
    }
    lib_free(temp1);

    base_cmdline_options[0].description = digimax_address_list;

    return cmdline_register_options(base_cmdline_options);
}

/* ---------------------------------------------------------------------*/

/* CARTDIGIMAX snapshot module format:

   type  | name       | description
   --------------------------------
   DWORD | base       | base address of the control registers
   ARRAY | sound data | 4 BYTES of sound data
   BYTE  | voice 0    | voice 0 data
   BYTE  | voice 1    | voice 1 data
   BYTE  | voice 2    | voice 2 data
   BYTE  | voice 3    | voice 3 data
 */

static const char snap_module_name[] = "CARTDIGIMAX";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int digimax_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_DW(m, (uint32_t)digimax_address) < 0)
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

int digimax_snapshot_read_module(snapshot_t *s)
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
        || (SMR_DW_INT(m, &temp_digimax_address) < 0)
        || (SMR_BA(m, digimax_sound_data, 4) < 0)
        || (SMR_B(m, &snd.voice[0]) < 0)
        || (SMR_B(m, &snd.voice[1]) < 0)
        || (SMR_B(m, &snd.voice[2]) < 0)
        || (SMR_B(m, &snd.voice[3]) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    /* HACK set address to an invalid value, then use the function */
    digimax_address = -1;
    set_digimax_base(temp_digimax_address, NULL);

    return digimax_enable();

fail:
    snapshot_module_close(m);
    return -1;
}
