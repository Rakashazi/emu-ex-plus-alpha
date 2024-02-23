/*
 * blackbox3.c - Cartridge handling, Blackbox V3 cart.
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
#include "blackbox3.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
   Blackbox V3

    - 8k ROM (2764)
    - IO1 accesses disable the cartridge ROM (writes only?)
    - IO2 accesses enable the cartridge ROM (writes only?)
*/

static uint8_t bb3_rom_enabled = 0;

/* some prototypes are needed */
static void blackbox3_io1_store(uint16_t addr, uint8_t value);
static void blackbox3_io2_store(uint16_t addr, uint8_t value);
static int blackbox3_dump(void);

static io_source_t blackbox3_io1_device = {
    CARTRIDGE_NAME_BLACKBOX3, /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,     /* range for the device, regs:$de00-$deff */
    0,                        /* read is never valid */
    blackbox3_io1_store,      /* store function */
    NULL,                     /* NO poke function */
    NULL,                     /* NO read function */
    NULL,                     /* NO peek function */
    blackbox3_dump,           /* device state information dump function */
    CARTRIDGE_BLACKBOX3,      /* cartridge ID */
    IO_PRIO_NORMAL,           /* normal priority, device read needs to be checked for collisions */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

static io_source_t blackbox3_io2_device = {
    CARTRIDGE_NAME_BLACKBOX3, /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,     /* range for the device, regs:$df00-$dfff */
    0,                        /* read is always valid */
    blackbox3_io2_store,      /* store function */
    NULL,                     /* NO poke function */
    NULL,                     /* NO read function */
    NULL,                     /* NO peek function */
    blackbox3_dump,           /* device state information dump function */
    CARTRIDGE_BLACKBOX3,      /* cartridge ID */
    IO_PRIO_NORMAL,           /* normal priority, device read needs to be checked for collisions */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

static io_source_list_t *blackbox3_io1_list_item = NULL;
static io_source_list_t *blackbox3_io2_list_item = NULL;

static const export_resource_t export_res_v3 = {
    CARTRIDGE_NAME_BLACKBOX3, 0, 1, &blackbox3_io1_device, &blackbox3_io2_device, CARTRIDGE_BLACKBOX3
};

/* ---------------------------------------------------------------------*/

void blackbox3_io1_store(uint16_t addr, uint8_t value)
{
    /* printf("io1 write %04x %02x\n", addr, value); */
    bb3_rom_enabled = 0;
    cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM, CMODE_READ);
}

void blackbox3_io2_store(uint16_t addr, uint8_t value)
{
    /* printf("io2 write %04x %02x\n", addr, value); */
    bb3_rom_enabled = 1;
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);

}

/* FIXME: Add EXROM, GAME lines to the dump */
static int blackbox3_dump(void)
{
    mon_out("Status: %s\n", bb3_rom_enabled ? "Enabled" : "Disabled");
    return 0;
}

/* ---------------------------------------------------------------------*/

void blackbox3_config_init(void)
{
    bb3_rom_enabled = 1;
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

void blackbox3_config_setup(uint8_t *rawcart)
{
    memcpy(&roml_banks[0], &rawcart[0x0000], 0x2000);
    bb3_rom_enabled = 1;
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int blackbox3_common_attach(void)
{
    if (export_add(&export_res_v3) < 0) {
        return -1;
    }

    blackbox3_io1_list_item = io_source_register(&blackbox3_io1_device);
    blackbox3_io2_list_item = io_source_register(&blackbox3_io2_device);

    return 0;
}

int blackbox3_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x2000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return blackbox3_common_attach();
}

int blackbox3_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.bank > 0 || chip.size != 0x2000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return blackbox3_common_attach();
}

void blackbox3_detach(void)
{
    export_remove(&export_res_v3);
    io_source_unregister(blackbox3_io1_list_item);
    io_source_unregister(blackbox3_io2_list_item);
    blackbox3_io1_list_item = NULL;
    blackbox3_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTBB3 snapshot module format:

   type  | name               | version | description
   -------------------------------------------
   BYTE  | bb3_rom_enabled    |   1.0   | 1 if ROM is enabled (informal)
   ARRAY | ROML               |   1.0   | 8192 of ROML data

 */

static const char snap_module_name[] = "CARTBB3";
#define SNAP_MAJOR   1
#define SNAP_MINOR   0

int blackbox3_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, bb3_rom_enabled) < 0
        || SMW_BA(m, roml_banks, 0x2000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int blackbox3_snapshot_read_module(snapshot_t *s)
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
        || SMR_B(m, &bb3_rom_enabled) < 0
        || SMR_BA(m, roml_banks, 0x2000) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return blackbox3_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
