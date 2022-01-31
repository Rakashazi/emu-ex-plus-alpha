/*
 * multimax.c - Cartridge handling, MultiMAX cart.
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
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "c64memrom.h"
#include "c64rom.h"
#include "cartio.h"
#include "cartridge.h"
#include "monitor.h"
#include "multimax.h"
#include "export.h"
#include "resources.h"
#include "ram.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    MultiMAX

    16MB ROM, 64*16K
    - ROM is mapped to $8000 and $e000 using ultimax mode

    2K RAM
    - RAM is mapped to $0800
    
    one register in the entire I/O1 space:
    
    bit 7       when set, the register is disabled and can only be 
                reenabled by reset
    bit 0-5     select ROM bank 0-63
*/

#define CART_RAM_SIZE (2 * 1024)

static uint8_t currbank = 0;
static uint8_t reg_enabled = 1;

static void multimax_io1_store(uint16_t addr, uint8_t value)
{
    /* printf("io1 %04x %02x\n", addr, value); */
    if (reg_enabled) {
        currbank = value & 0x3f;
        reg_enabled = ((value >> 7) & 1) ^ 1;
    }
}

static int multimax_dump(void)
{
    mon_out("Register is %s.\n", reg_enabled ? "enabled" : "disabled");
    mon_out("ROM Bank: %d\n", currbank);
    return 0;
}

static io_source_t multimax_device = {
    CARTRIDGE_NAME_MULTIMAX,  /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,     /* range for the device, address is ignored, reg:$de00, mirrors:$de01-$deff */
    0,                        /* read is never valid */
    multimax_io1_store,       /* store function */
    NULL,                     /* NO poke function */
    NULL,                     /* NO read function */
    NULL,                     /* NO peek function */
    multimax_dump,            /* device state information dump function */
    CARTRIDGE_MULTIMAX,       /* cartridge ID */
    IO_PRIO_NORMAL,           /* normal priority, device read needs to be checked for collisions */
    0                         /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *multimax_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_MULTIMAX, 1, 1, &multimax_device, NULL, CARTRIDGE_MULTIMAX
};

/* ---------------------------------------------------------------------*/

uint8_t multimax_0800_0fff_read(uint16_t addr)
{
    return export_ram0[addr & 0x07ff];
}

void multimax_0800_0fff_store(uint16_t addr, uint8_t value)
{
    if (reg_enabled) {
        currbank = value & 0x7f;
        reg_enabled = ((value >> 7) & 1) ^ 1;
    }
    export_ram0[addr & 0x07ff] = value;
}

uint8_t multimax_roml_read(uint16_t addr)
{
    return roml_banks[(addr & 0x1fff) + (currbank * 0x2000)];
}

uint8_t multimax_romh_read(uint16_t addr)
{
    return romh_banks[(addr & 0x1fff) + (currbank * 0x2000)];
}

int multimax_peek_mem(export_t *ex, uint16_t addr, uint8_t *value)
{
    if (addr >= 0xe000) {
        *value = romh_banks[(addr & 0x1fff) + (currbank * 0x2000)];
        return CART_READ_VALID;
    } else if ((addr >= 0xa000) && (addr <= 0xbfff)) {
        *value = roml_banks[(addr & 0x1fff) + (currbank * 0x2000)];
        return CART_READ_VALID;
    } else if ((addr >= 0x0800) && (addr <= 0x0fff)) {
        *value = export_ram0[addr & 0x07ff];
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH;
}

void multimax_config_init(void)
{
    currbank = 0;
    reg_enabled = 1;
    cart_config_changed_slotmain(CMODE_ULTIMAX, CMODE_ULTIMAX, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

void multimax_config_setup(uint8_t *rawcart)
{
    int i;
    for (i = 0; i < 64; i++) {
        memcpy(&roml_banks[0x0000 + (i * 0x2000)], &rawcart[0x0000 + (i * 0x4000)], 0x2000);
        memcpy(&romh_banks[0x0000 + (i * 0x2000)], &rawcart[0x2000 + (i * 0x4000)], 0x2000);
    }
    cart_config_changed_slotmain(CMODE_ULTIMAX, CMODE_ULTIMAX, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

/* FIXME: this still needs to be tweaked to match the hardware */
static RAMINITPARAM ramparam = {
    .start_value = 255,
    .value_invert = 2,
    .value_offset = 1,

    .pattern_invert = 0x100,
    .pattern_invert_value = 255,

    .random_start = 0,
    .random_repeat = 0,
    .random_chance = 0,
};

void multimax_powerup(void)
{
    ram_init_with_pattern(export_ram0, CART_RAM_SIZE, &ramparam);
}

static int multimax_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    multimax_list_item = io_source_register(&multimax_device);    
    return 0;
}

int multimax_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 1024 * 1024, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return multimax_common_attach();
}

int multimax_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i < 64; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }

        if (chip.size != 0x4000) {
            return -1;
        }

        if (crt_read_chip(rawcart + (i * 0x4000), 0, &chip, fd)) {
            return -1;
        }
    }

    return multimax_common_attach();
}

void multimax_detach(void)
{
    export_remove(&export_res);
}

/* ---------------------------------------------------------------------*/

/* CARTMULTIMAX snapshot module format:

   type  | name         | description
   ------------------------------------------------------
   BYTE  | currbank     | current ROM bank
   BYTE  | reg_enabled  | flag, is the register enabled?
   ARRAY | ROML         | 512K of ROML data
   ARRAY | ROMH         | 512K of ROMH data
   ARRAY | RAM          | 2048 BYTES of RAM data
 */

static const char snap_module_name[] = "CARTMULTIMAX";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int multimax_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (uint8_t)currbank) < 0)
        || (SMW_B(m, (uint8_t)reg_enabled) < 0)
        || (SMW_BA(m, roml_banks, 0x2000 * 64) < 0)
        || (SMW_BA(m, romh_banks, 0x2000 * 64) < 0)
        || (SMW_BA(m, export_ram0, 0x0800) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int multimax_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B(m, &currbank) < 0)
        || (SMR_B(m, &reg_enabled) < 0)
        || (SMR_BA(m, roml_banks, 0x2000 * 64) < 0)
        || (SMR_BA(m, romh_banks, 0x2000 * 64) < 0)
        || (SMR_BA(m, export_ram0, 0x0800) < 0)) {
        goto fail;
    }
    
    snapshot_module_close(m);

    return multimax_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
