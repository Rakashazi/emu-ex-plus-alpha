/*
 * bisplus.c - Cartridge handling, BIS-Plus cart.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "bisplus.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
   "BIS-Plus" by Micro-Luc

    - 2/4/8K ROM Variants
    - IO1 writes disable the cartridge ROM
*/

static uint8_t bis_rom_enabled = 0;
static uint8_t bis_rom_kb = 0;

/* some prototypes are needed */
static void bisplus_io1_store(uint16_t addr, uint8_t value);
static int bisplus_dump(void);

static io_source_t bisplus_io1_device = {
    CARTRIDGE_NAME_BISPLUS,   /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,     /* range for the device, regs:$de00-$deff */
    0,                        /* read is never valid */
    bisplus_io1_store,        /* store function */
    NULL,                     /* NO poke function */
    NULL,                     /* NO read function */
    NULL,                     /* NO peek function */
    bisplus_dump,             /* device state information dump function */
    CARTRIDGE_BISPLUS,        /* cartridge ID */
    IO_PRIO_NORMAL,           /* normal priority, device read needs to be checked for collisions */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

static io_source_list_t *bisplus_io1_list_item = NULL;

static const export_resource_t export_res_bis = {
    CARTRIDGE_NAME_BISPLUS, 0, 1, &bisplus_io1_device, NULL, CARTRIDGE_BISPLUS
};

/* ---------------------------------------------------------------------*/

void bisplus_io1_store(uint16_t addr, uint8_t value)
{
    /* printf("io1 write %04x %02x\n", addr, value); */
    bis_rom_enabled = 0;
    cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM, CMODE_READ);
}

/* FIXME: Add EXROM, GAME lines to the dump */
static int bisplus_dump(void)
{
    mon_out("ROM Size: %dKiB\n", bis_rom_kb);
    mon_out("Status: %s\n", bis_rom_enabled ? "Enabled" : "Disabled");
    return 0;
}

/* ---------------------------------------------------------------------*/

void bisplus_config_init(void)
{
    bis_rom_enabled = 1;
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

void bisplus_config_setup(uint8_t *rawcart)
{
    memcpy(&roml_banks[0], &rawcart[0], 0x2000);
    if (bis_rom_kb == 4) {
        memcpy(&roml_banks[0x1000], &rawcart[0], 0x1000);
    }
    if (bis_rom_kb == 2) {
        memcpy(&roml_banks[0x0800], &rawcart[0], 0x0800);
        memcpy(&roml_banks[0x1000], &rawcart[0], 0x0800);
        memcpy(&roml_banks[0x1800], &rawcart[0], 0x0800);
    }
    bis_rom_enabled = 1;
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int bisplus_common_attach(void)
{
    if (export_add(&export_res_bis) < 0) {
        return -1;
    }

    bisplus_io1_list_item = io_source_register(&bisplus_io1_device);

    return 0;
}

int bisplus_bin_attach(const char *filename, uint8_t *rawcart)
{
    bis_rom_kb = 8;
    if (util_file_load(filename, rawcart, 0x2000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        bis_rom_kb = 4;
        if (util_file_load(filename, rawcart, 0x1000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            bis_rom_kb = 2;
            if (util_file_load(filename, rawcart, 0x0800, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                return -1;
            }
        }
    }

    return bisplus_common_attach();
}

int bisplus_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.bank > 0) {
        return -1;
    }

    switch (chip.size) {
        case 0x2000:
            bis_rom_kb = 8;
            break;
        case 0x1000:
            bis_rom_kb = 4;
            break;
        case 0x0800:
            bis_rom_kb = 2;
            break;
        default:
            return -1;
            break;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return bisplus_common_attach();
}

void bisplus_detach(void)
{
    export_remove(&export_res_bis);
    io_source_unregister(bisplus_io1_list_item);
    bisplus_io1_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTBB3 snapshot module format:

   type  | name               | version | description
   -------------------------------------------
   BYTE  | bis_rom_enabled    |   1.0   | 1 if ROM is enabled (informal)
   BYTE  | bis_rom_kb         |   1.0   | 2/4/8 kb ROM
   ARRAY | ROML               |   1.0   | 8192 of ROML data

 */

static const char snap_module_name[] = "CARTBB3";
#define SNAP_MAJOR   1
#define SNAP_MINOR   0

int bisplus_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, bis_rom_enabled) < 0
        || SMW_B(m, bis_rom_kb) < 0
        || SMW_BA(m, roml_banks, 0x2000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int bisplus_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

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
        || SMR_B(m, &bis_rom_enabled) < 0
        || SMR_B(m, &bis_rom_kb) < 0
        || SMR_BA(m, roml_banks, 0x2000) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return bisplus_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
