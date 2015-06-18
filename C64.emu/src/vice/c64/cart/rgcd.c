/*
 * rgcd.c - Cartridge handling, RGCD cart.
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

/* #define RGCD_DEBUG */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64export.h"
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "rgcd.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

#ifdef RGCD_DEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    "RGCD" 64k Cartridge

    - 64Kb (8 banks)
    - ROM is always mapped in at $8000-$9FFF (8k game).

    - 1 register at io1 / de00:

    bit 0-2   bank number
    bit 3     exrom (1 = cart rom and I/O disabled until reset/powercycle)
*/

#define MAXBANKS 8

static BYTE regval = 0;
static BYTE disabled = 0;

static void rgcd_io1_store(WORD addr, BYTE value)
{
    regval = value & 0x0f;
    cart_set_port_game_slotmain(0);
    disabled |= (value & 0x08) ? 1 : 0;
    if (disabled) {
        /* turn off cart ROM */
        cart_set_port_exrom_slotmain(0);
    } else {
        cart_romlbank_set_slotmain(value & 0x07);
        cart_set_port_exrom_slotmain(1);
    }
    cart_port_config_changed_slotmain();
    DBG(("RGCD: Reg: %02x (Bank: %d, %s)\n", regval, (regval & 0x07), disabled ? "disabled" : "enabled"));
}

static BYTE rgcd_io1_peek(WORD addr)
{
    return regval;
}

static int rgcd_dump(void)
{
    mon_out("Reg: %02x (Bank: %d, %s)\n", regval, (regval & 0x07), disabled ? "disabled" : "enabled");
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t rgcd_device = {
    CARTRIDGE_NAME_RGCD,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    rgcd_io1_store,
    NULL,
    rgcd_io1_peek,
    rgcd_dump,
    CARTRIDGE_RGCD,
    0,
    0
};

static io_source_list_t *rgcd_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_RGCD, 1, 1, &rgcd_device, NULL, CARTRIDGE_RGCD
};

/* ---------------------------------------------------------------------*/

void rgcd_reset(void)
{
    disabled = 0;
    cart_set_port_exrom_slotmain(1);
}

void rgcd_config_init(void)
{
    disabled = 0;
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    rgcd_io1_store((WORD)0xde00, 0);
}

void rgcd_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000 * MAXBANKS);
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int rgcd_common_attach(void)
{
    if (c64export_add(&export_res) < 0) {
        return -1;
    }
    rgcd_list_item = io_source_register(&rgcd_device);
    return 0;
}

int rgcd_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return rgcd_common_attach();
}

int rgcd_crt_attach(FILE *fd, BYTE *rawcart)
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
    return rgcd_common_attach();
}

void rgcd_detach(void)
{
    c64export_remove(&export_res);
    io_source_unregister(rgcd_list_item);
    rgcd_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   2
#define SNAP_MODULE_NAME  "CARTRGCD"

int rgcd_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)regval) < 0)
        || (SMW_B(m, (BYTE)disabled) < 0)
        || (SMW_BA(m, roml_banks, 0x2000 * MAXBANKS) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int rgcd_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B(m, &disabled) < 0)
        || (SMR_BA(m, roml_banks, 0x2000 * MAXBANKS) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (rgcd_common_attach() == -1) {
        return -1;
    }
    rgcd_io1_store(0xde00, regval);
    return 0;
}
