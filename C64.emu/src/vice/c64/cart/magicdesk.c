/*
 * magicdesk.c - Cartridge handling, Magic Desk cart.
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

/* #define MAGICDESK_DEBUG */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "magicdesk.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

#ifdef MAGICDESK_DEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    "Magic Desk" Cartridge

    - this cart comes in 3 sizes, 32Kb (4 banks), 64Kb (8 banks) and 128Kb (16 banks).
    - ROM is always mapped in at $8000-$9FFF (8k game).

    - 1 register at io1 / de00:

    bit 0-3   bank number
    bit 7     exrom (1 = cart disabled)
*/

#define MAXBANKS 16

static BYTE regval = 0;

static void magicdesk_io1_store(WORD addr, BYTE value)
{
    regval = value & 0x8f;
    cart_romlbank_set_slotmain(value & 0x0f);
    cart_set_port_game_slotmain(0);
    if (value & 0x80) {
        /* turn off cart ROM */
        cart_set_port_exrom_slotmain(0);
    } else {
        cart_set_port_exrom_slotmain(1);
    }
    cart_port_config_changed_slotmain();
    DBG(("MAGICDESK: Reg: %02x (Bank: %d, %s)\n", regval, (regval & 0x0f), (regval & 0x80) ? "disabled" : "enabled"));
}

static BYTE magicdesk_io1_peek(WORD addr)
{
    return regval;
}

static int magicdesk_dump(void)
{
    mon_out("Reg: %02x (Bank: %d, %s)\n", regval, (regval & 0x0f), (regval & 0x80) ? "disabled" : "enabled");
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t magicdesk_device = {
    CARTRIDGE_NAME_MAGIC_DESK,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    magicdesk_io1_store,
    NULL,
    magicdesk_io1_peek,
    magicdesk_dump,
    CARTRIDGE_MAGIC_DESK,
    0,
    0
};

static io_source_list_t *magicdesk_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_MAGIC_DESK, 0, 1, &magicdesk_device, NULL, CARTRIDGE_MAGIC_DESK
};

/* ---------------------------------------------------------------------*/

void magicdesk_config_init(void)
{
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    magicdesk_io1_store((WORD)0xde00, 0);
}

void magicdesk_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000 * MAXBANKS);
    cart_config_changed_slotmain(0, 0, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int magicdesk_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    magicdesk_list_item = io_source_register(&magicdesk_device);
    return 0;
}

int magicdesk_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x20000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            if (util_file_load(filename, rawcart, 0x8000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                return -1;
            }
        }
    }
    return magicdesk_common_attach();
}

int magicdesk_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }
        if ((chip.bank >= MAXBANKS) || ((chip.start != 0x8000) && (chip.start != 0xa000)) || (chip.size != 0x2000)) {
            return -1;
        }
        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }
    return magicdesk_common_attach();
}

void magicdesk_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(magicdesk_list_item);
    magicdesk_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   1
#define SNAP_MODULE_NAME  "CARTMAGICD"

int magicdesk_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)regval) < 0)
        || (SMW_BA(m, roml_banks, 0x2000 * MAXBANKS) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int magicdesk_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B(m, &regval) < 0)
        || (SMR_BA(m, roml_banks, 0x2000 * MAXBANKS) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (magicdesk_common_attach() == -1) {
        return -1;
    }
    magicdesk_io1_store(0xde00, regval);
    return 0;
}
