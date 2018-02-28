/*
 * stb.c - Cartridge handling, Structured Basic cart.
 *
 * Written by
 *  Walter Zimmer, adapted from kcs.c
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
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "monitor.h"
#include "snapshot.h"
#include "stb.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/* Structured Basic IO1 logic for the roml range $8000-$9fff
*
*  Any read/write access for IO/1 latch the lower two address bits:
*  - bit 1 selects bank 1 of the stb rom
*  - bit 0 and bit 1 set deactivate EXROM and expose the RAM
*/

/* ---------------------------------------------------------------------*/

static BYTE stb_io1_read(WORD addr);
static BYTE stb_io1_peek(WORD addr);
static void stb_io1_store(WORD addr, BYTE value);
static int stb_dump(void);

static io_source_t stb_device = {
    CARTRIDGE_NAME_STRUCTURED_BASIC,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0, /* read is never valid */
    stb_io1_store,
    stb_io1_read,
    stb_io1_peek,
    stb_dump,
    CARTRIDGE_STRUCTURED_BASIC,
    0,
    0
};

static io_source_list_t *stb_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_STRUCTURED_BASIC, 1, 0, &stb_device, NULL, CARTRIDGE_STRUCTURED_BASIC
};

/* ---------------------------------------------------------------------*/

static int stb_bank = 0;
static int stb_active = 0;

static void stb_io(WORD addr)
{
    switch (addr & 3) {
        /* normal config: bank 0 visible */
        case 0:
        case 1:
            cart_config_changed_slotmain(0, 0, CMODE_READ);
            stb_bank = 0;
            stb_active = 1;
            break;

        /* bank 1 visible, gets copied to RAM during reset */
        case 2:
            cart_config_changed_slotmain(0 | (1 << CMODE_BANK_SHIFT), 0 | (1 << CMODE_BANK_SHIFT), CMODE_READ);
            stb_bank = 1;
            stb_active = 1;
            break;

        /* RAM visible, which contains bank 1 */
        case 3:
            cart_config_changed_slotmain(2, 2, CMODE_READ);
            stb_bank = 1;
            stb_active = 0;
            break;
    }
}

static BYTE stb_io1_read(WORD addr)
{
    stb_io(addr);
    return 0;
}

static BYTE stb_io1_peek(WORD addr)
{
    return 0;
}

static void stb_io1_store(WORD addr, BYTE value)
{
    stb_io(addr);
}

static int stb_dump(void)
{
    mon_out("$8000-$9FFF ROM: %s\n", (stb_active) ? "enabled" : "disabled");
    if (stb_active) {
        mon_out("bank: %d\n", stb_bank);
    }
    return 0;
}

/* ---------------------------------------------------------------------*/

void stb_config_init(void)
{
    /* turn on normal config: bank 0 */
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    stb_bank = 0;
    stb_active = 1;
}

void stb_config_setup(BYTE *rawcart)
{
    /* copy banks 0 and 1 */
    memcpy(roml_banks, rawcart, 0x4000);

    /* turn on normal config: bank 0 */
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    stb_bank = 0;
    stb_active = 1;
}

/* ---------------------------------------------------------------------*/

static int stb_common_attach(void)
{
    /* add export */
    if (export_add(&export_res) < 0) {
        return -1;
    }

    stb_list_item = io_source_register(&stb_device);

    return 0;
}

int stb_bin_attach(const char *filename, BYTE *rawcart)
{
    /* load file into cartridge address space */
    if (util_file_load(filename, rawcart, 0x4000, UTIL_FILE_LOAD_RAW) < 0) {
        return -1;
    }

    return stb_common_attach();
}

int stb_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.start != 0x8000 || chip.size != 0x2000 || chip.bank > 1) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    return stb_common_attach();
}

void stb_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(stb_list_item);
    stb_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTSTB snapshot module format:

   type  | name   | version | description
   --------------------------------------
   BYTE  | bank   |   0.1   | current bank
   BYTE  | active |   0.1   | cartridge active flag
   ARRAY | ROML   |   0.0+  | 16384 BYTES of ROML data
 */

static char snap_module_name[] = "CARTSTB";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int stb_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)stb_bank) < 0
        || SMW_B(m, (BYTE)stb_active) < 0
        || SMW_BA(m, roml_banks, 0x4000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int stb_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* new in 0.1 */
    if (SNAPVAL(vmajor, vminor, 0, 1)) {
        if (0
            || SMR_B_INT(m, &stb_bank) < 0
            || SMR_B_INT(m, &stb_active) < 0) {
            goto fail;
        }
    } else {
        stb_bank = 0;
        stb_active = 0;
    }

    if (SMR_BA(m, roml_banks, 0x4000) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return stb_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
