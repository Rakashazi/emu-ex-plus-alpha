/*
 * ross.c - Cartridge handling, Ross cart.
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
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "monitor.h"
#include "ross.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    "Ross" Cartridge

    - 16kb or 32kb ROM

    - 16Kb ROM mapped in at $8000-$BFFF in 16k game config

    - Any read access to $DE00 will switch in bank 1 (if cart is 32Kb).

    - Any read access to $DF00 will switch off EXROM and GAME.
*/

static int currbank = 0;

static int ross_is_32k = 0;

static BYTE ross_io1_read(WORD addr)
{
    if (ross_is_32k) {
        cart_romhbank_set_slotmain(1);
        cart_romlbank_set_slotmain(1);
        currbank = 1;
    }
    return 0;
}

static BYTE ross_io_peek(WORD addr)
{
    return 0;
}

static BYTE ross_io2_read(WORD addr)
{
    cart_set_port_exrom_slotmain(0);
    cart_set_port_game_slotmain(0);
    cart_port_config_changed_slotmain();
    return 0;
}

static int ross_dump(void)
{
    mon_out("Size: %s, bank: %d\n",
            (ross_is_32k) ? "32Kb" : "16Kb",
            currbank);
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t ross_io1_device = {
    CARTRIDGE_NAME_ROSS,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0, /* read is never valid */
    NULL,
    ross_io1_read,
    ross_io_peek,
    ross_dump,
    CARTRIDGE_ROSS,
    0,
    0
};

static io_source_t ross_io2_device = {
    CARTRIDGE_NAME_ROSS,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    0, /* read is never valid */
    NULL,
    ross_io2_read,
    ross_io_peek,
    ross_dump,
    CARTRIDGE_ROSS,
    0,
    0
};

static io_source_list_t *ross_io1_list_item = NULL;
static io_source_list_t *ross_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_ROSS, 1, 1, &ross_io1_device, &ross_io2_device, CARTRIDGE_ROSS
};

/* ---------------------------------------------------------------------*/

void ross_config_init(void)
{
    cart_config_changed_slotmain(1, 1, CMODE_READ);
}

void ross_config_setup(BYTE *rawcart)
{
    memcpy(&roml_banks[0x0000], &rawcart[0x0000], 0x2000);
    memcpy(&romh_banks[0x0000], &rawcart[0x2000], 0x2000);
    memcpy(&roml_banks[0x2000], &rawcart[0x4000], 0x2000);
    memcpy(&romh_banks[0x2000], &rawcart[0x6000], 0x2000);
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    currbank = 0;
}

/* ---------------------------------------------------------------------*/

static int ross_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    ross_io1_list_item = io_source_register(&ross_io1_device);
    ross_io2_list_item = io_source_register(&ross_io2_device);
    return 0;
}

int ross_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x8000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        if (util_file_load(filename, rawcart, 0x4000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            return -1;
        }
        ross_is_32k = 0;
    } else {
        ross_is_32k = 1;
    }
    return ross_common_attach();
}

int ross_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int amount = 0;

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        amount++;

        if (chip.start != 0x8000 || chip.size != 0x4000 || chip.bank > 1) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 14, &chip, fd)) {
            return -1;
        }
    }

    if (amount == 1) {
        ross_is_32k = 0;
    } else {
        ross_is_32k = 1;
    }
    return ross_common_attach();
}

void ross_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(ross_io1_list_item);
    io_source_unregister(ross_io2_list_item);
    ross_io1_list_item = NULL;
    ross_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTROSS snapshot module format:

   type  | name  | version | description
   -------------------------------------
   BYTE  | is32k |   0.1   | cart is 32KB flag
   BYTE  | bank  |   0.0+  | current bank
   ARRAY | ROML  |   0.0+  | 16384 BYTES of ROML data
   ARRAY | ROMH  |   0.0+  | 16384 BYTES of ROMH data
 */

static char snap_module_name[] = "CARTROSS";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int ross_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)ross_is_32k) < 0
        || SMW_B(m, (BYTE)currbank) < 0
        || SMW_BA(m, roml_banks, 0x4000) < 0
        || SMW_BA(m, romh_banks, 0x4000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int ross_snapshot_read_module(snapshot_t *s)
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
        if (SMR_B_INT(m, &ross_is_32k) < 0) {
            goto fail;
        }
    } else {
        ross_is_32k = 0;
    }

    if (0
        || SMR_B_INT(m, &currbank) < 0
        || SMR_BA(m, roml_banks, 0x4000) < 0
        || SMR_BA(m, romh_banks, 0x4000) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return ross_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
