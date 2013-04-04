/*
 * rexep256.c - Cartridge handling, REX EP256 cart.
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

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64export.h"
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "monitor.h"
#include "rexep256.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/* This eprom system by REX is similair to the EP64. It can handle
   what the EP64 can handle, plus the following features :

   - Alternate rom at $8000
   - 8kb or 16kb or 32kb eprom support

   The system uses 9 eproms, of which the first (8kb eprom) is used
   for the main menu. 8 eprom banks, populated with either an 8kb,
   16kb or 32kb eprom, are used for above named features.

   When a 16kb or 32kb is used only 8kb blocks of it can be switched.

   Because of the fact that this system supports switching in a
   different eprom at $8000 (followed by a reset) it is possible
   to place other 8kb carts in the eproms and use them.

   The bank selecting is done by  writing  to  $DFA0:

   bit   meaning
   ---   -------
    0    socket selection bit 0
    1    socket selection bit 1
    2    socket selection bit 2
    3    unused
    4    8kb bank selection bit 0
    5    8kb bank selection bit 1
   6-7   unused

    Reading from $DFC0 switches off the EXROM.

    Reading from $DFE0 switches on the EXROM.
*/

/* ---------------------------------------------------------------------*/

static WORD rexep256_eprom[8];
static BYTE rexep256_eprom_roml_bank_offset[8];
static BYTE regval = 0;

/* ---------------------------------------------------------------------*/

static void rexep256_io2_store(WORD addr, BYTE value)
{
    BYTE eprom_bank, test_value, eprom_part = 0;

    if ((addr & 0xff) == 0xa0) {
        regval = value;
        eprom_bank = (value & 0xf);
        if (eprom_bank > 7) {
            return;
        }

        test_value = (value & 0xf0) >> 4;
        if (test_value > 3) {
            return;
        }

        if (rexep256_eprom[eprom_bank] == 0x2000) {
            eprom_part = 0;
        }
        if (rexep256_eprom[eprom_bank] == 0x4000) {
            eprom_part = test_value & 1;
        }
        if (rexep256_eprom[eprom_bank] == 0x8000) {
            eprom_part = test_value;
        }

        cart_romlbank_set_slotmain(rexep256_eprom_roml_bank_offset[eprom_bank] + eprom_part + 1);
    }
}

/* I'm unsure whether the register is write-only,
   but in this case it is assumed to be. */
static BYTE rexep256_io2_read(WORD addr)
{
    if ((addr & 0xff) == 0xc0) {
        cart_set_port_exrom_slotmain(0);
        cart_port_config_changed_slotmain();
    }
    if ((addr & 0xff) == 0xe0) {
        cart_set_port_exrom_slotmain(1);
        cart_port_config_changed_slotmain();
    }
    return 0;
}

static BYTE rexep256_io2_peek(WORD addr)
{
    return regval;
}

static int rexep256_dump(void)
{
    mon_out("Socket: %d, bank: %d\n",
            regval & 7,
            (regval & 0x30) >> 4);
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t rexep256_device = {
    CARTRIDGE_NAME_REX_EP256,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    0, /* read is never valid */
    rexep256_io2_store,
    rexep256_io2_read,
    rexep256_io2_peek,
    rexep256_dump,
    CARTRIDGE_REX_EP256,
    0,
    0
};

static io_source_list_t *rexep256_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_REX_EP256, 1, 0, NULL, &rexep256_device, CARTRIDGE_REX_EP256
};

/* ---------------------------------------------------------------------*/

void rexep256_config_init(void)
{
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    cart_romlbank_set_slotmain(0);
}

void rexep256_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x42000);
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    cart_romlbank_set_slotmain(0);
}

/* ---------------------------------------------------------------------*/
static int rexep256_common_attach(void)
{
    if (c64export_add(&export_res) < 0) {
        return -1;
    }
    rexep256_list_item = io_source_register(&rexep256_device);
    return 0;
}

/* FIXME: handle the various combinations / possible file lengths */
int rexep256_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x2000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return rexep256_common_attach();
}

int rexep256_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int rexep256_total_size = 0;
    int i;

    memset(rawcart, 0xff, 0x42000);

    for (i = 0; i < 8; i++) {
        rexep256_eprom[i] = 0x2000;
        rexep256_eprom_roml_bank_offset[i] = 0x1f;
    }

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.size != 0x2000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.size != 0x2000 && chip.size != 0x4000 && chip.size != 0x8000) {
            return -1;
        }

        if (chip.bank > 8) {
            return -1;
        }

        rexep256_eprom[chip.bank - 1] = chip.size;
        rexep256_eprom_roml_bank_offset[chip.bank - 1] = rexep256_total_size >> 13;

        if (crt_read_chip(rawcart, 0x2000 + rexep256_total_size, &chip, fd)) {
            return -1;
        }

        rexep256_total_size = rexep256_total_size + chip.size;
    }
    return rexep256_common_attach();
}

void rexep256_detach(void)
{
    c64export_remove(&export_res);
    io_source_unregister(rexep256_list_item);
    rexep256_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTREXEP256"

int rexep256_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_WA(m, rexep256_eprom, 8) < 0)
        || (SMW_BA(m, rexep256_eprom_roml_bank_offset, 8) < 0)
        || (SMW_BA(m, roml_banks, 0x42000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int rexep256_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0
        || (SMR_WA(m, rexep256_eprom, 8) < 0)
        || (SMR_BA(m, rexep256_eprom_roml_bank_offset, 8) < 0)
        || (SMR_BA(m, roml_banks, 0x42000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return rexep256_common_attach();
}
