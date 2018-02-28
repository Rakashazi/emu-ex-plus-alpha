/*
 * comal80.c - Cartridge handling, Comal80 cart.
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

/* #define DEBUGCART */

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
#include "comal80.h"
#include "export.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

#ifdef DEBUGCART
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    Comal80 Cartridge

    - 64K ROM (32K mapped to $8000 and 32K mapped to $A000)

    The cart has 1 (write-only) bank control register which
    is located at $DE00 and mirrored throughout the $DE00-$DEFF
    range.

    bit 7   : exrom?
    bit 6   : game?
    bit 5   : unknown function (used by the software to disable the cartridge)
    bit 4   : unused?
    bit 3   : unused?
    bit 2   : unknown function (used by the software however)
    bit 0-1 : selects bank
*/

static int currregval = 0;

static void comal80_io1_store(WORD addr, BYTE value)
{
    int cmode, currbank;

    currregval = value & 0xc7;
    currbank = value & 3;

    switch (value & 0xe0) {
        case 0xe0:
            cmode = CMODE_RAM;
            break;
        default:
        case 0x80:
            cmode = CMODE_16KGAME;
            break;
        case 0x40:
            cmode = CMODE_8KGAME;
            break;
    }
#ifdef DEBUGCART
    if ((value != 0x82) && (value != 0x83)) {
        DBG(("COMAL80: IO1W %04x %02x mode: %d bank: %d\n", addr, value, cmode, currbank));
    }
#endif
    cart_config_changed_slotmain(0, (BYTE)(cmode | (currbank << CMODE_BANK_SHIFT)), CMODE_READ);
}

static BYTE comal80_io1_peek(WORD addr)
{
    return currregval;
}

static int comal80_dump(void)
{
    mon_out("register value: %d\n", currregval);
    mon_out(" bank: %d\n", currregval & 3);
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t comal80_device = {
    CARTRIDGE_NAME_COMAL80,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    comal80_io1_store,
    NULL,
    comal80_io1_peek,
    comal80_dump,
    CARTRIDGE_COMAL80,
    0,
    0
};

static io_source_list_t *comal80_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_COMAL80, 1, 1, &comal80_device, NULL, CARTRIDGE_COMAL80
};

/* ---------------------------------------------------------------------*/

void comal80_config_init(void)
{
    cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME, CMODE_READ);
    currregval = 0;
}

void comal80_config_setup(BYTE *rawcart)
{
    memcpy(&roml_banks[0x0000], &rawcart[0x0000], 0x2000);
    memcpy(&romh_banks[0x0000], &rawcart[0x2000], 0x2000);
    memcpy(&roml_banks[0x2000], &rawcart[0x4000], 0x2000);
    memcpy(&romh_banks[0x2000], &rawcart[0x6000], 0x2000);
    memcpy(&roml_banks[0x4000], &rawcart[0x8000], 0x2000);
    memcpy(&romh_banks[0x4000], &rawcart[0xa000], 0x2000);
    memcpy(&roml_banks[0x6000], &rawcart[0xc000], 0x2000);
    memcpy(&romh_banks[0x6000], &rawcart[0xe000], 0x2000);
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

/* ---------------------------------------------------------------------*/
static int comal80_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    comal80_list_item = io_source_register(&comal80_device);
    return 0;
}

int comal80_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return comal80_common_attach();
}

int comal80_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.start != 0x8000 || chip.size != 0x4000 || chip.bank > 3) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 14, &chip, fd)) {
            return -1;
        }
    }
    return comal80_common_attach();
}

void comal80_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(comal80_list_item);
    comal80_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTCOMAL snapshot module format:

   type  | name     | description
   ------------------------------
   BYTE  | register | control register
   ARRAY | ROML     | 32768 BYTES of ROML data
   ARRAY | ROMH     | 32768 BYTES of ROMH data
 */

static char snap_module_name[] = "CARTCOMAL";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int comal80_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)currregval) < 0)
        || (SMW_BA(m, roml_banks, 0x8000) < 0)
        || (SMW_BA(m, romh_banks, 0x8000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int comal80_snapshot_read_module(snapshot_t *s)
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

    if (0
        || (SMR_B_INT(m, &currregval) < 0)
        || (SMR_BA(m, roml_banks, 0x8000) < 0)
        || (SMR_BA(m, romh_banks, 0x8000) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    return comal80_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
