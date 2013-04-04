/*
 * dinamic.c - Cartridge handling, Dinamic cart.
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
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64export.h"
#include "cartio.h"
#include "cartridge.h"
#include "dinamic.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/* #define DBGDINAMIC */

#ifdef DBGDINAMIC
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    Dinamic Software Game Cartridge

    - Narco Police (128k, 8k*16)
    - Satan (128k, 8k*16)

    - 16 8k ROM Banks, mapped to $8000 in 8k Game Mode

    io1:
    - banks are switched by read accesses to deXX, where XX is the bank number
*/

static int currbank = 0;

static BYTE dinamic_io1_read(WORD addr)
{
    DBG(("@ $%04x io1 rd %04x (bank: %02x)\n", reg_pc, addr, addr & 0x0f));
    if ((addr & 0x0f) == addr) {
        cart_romlbank_set_slotmain(addr & 0x0f);
        cart_romhbank_set_slotmain(addr & 0x0f);
        currbank = addr & 0x0f;
    }
    return 0;
}

static BYTE dinamic_io1_peek(WORD addr)
{
    return 0;
}

static int dinamic_dump(void)
{
    mon_out("Bank: %d\n", currbank);
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t dinamic_io1_device = {
    CARTRIDGE_NAME_DINAMIC,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0, /* reads are never valid */
    NULL,
    dinamic_io1_read,
    dinamic_io1_peek,
    dinamic_dump,
    CARTRIDGE_DINAMIC,
    0,
    0
};

static io_source_list_t *dinamic_io1_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_DINAMIC, 1, 0, &dinamic_io1_device, NULL, CARTRIDGE_DINAMIC
};

/* ---------------------------------------------------------------------*/

void dinamic_config_init(void)
{
    cart_config_changed_slotmain(0, 0, CMODE_READ);
}

void dinamic_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000 * 16);
    cart_config_changed_slotmain(0, 0, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int dinamic_common_attach(void)
{
    if (c64export_add(&export_res) < 0) {
        return -1;
    }
    dinamic_io1_list_item = io_source_register(&dinamic_io1_device);
    return 0;
}

int dinamic_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x20000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return dinamic_common_attach();
}

int dinamic_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > 15 || chip.size != 0x2000 || chip.start != 0x8000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    return dinamic_common_attach();
}

void dinamic_detach(void)
{
    io_source_unregister(dinamic_io1_list_item);
    dinamic_io1_list_item = NULL;
    c64export_remove(&export_res);
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTDINAMIC"

int dinamic_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)currbank) < 0)
        || (SMW_BA(m, roml_banks, 0x2000 * 16) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int dinamic_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B_INT(m, &currbank) < 0)
        || (SMR_BA(m, roml_banks, 0x2000 * 16) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return dinamic_common_attach();
}
