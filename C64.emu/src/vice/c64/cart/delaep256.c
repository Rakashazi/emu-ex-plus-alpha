/*
 * delaep256.c - Cartridge handling, Dela EP256 cart.
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
#include "cartio.h"
#include "cartridge.h"
#include "delaep256.h"
#include "export.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/* This eprom system by DELA is similair to the EP64. It can handle
   what the EP64 can handle, plus the following features :

   - Alternate rom at $8000

   The system uses 33 8kb eproms, of which the first is used for
   the main menu. 32 8kb banks are used for above named features.

   Because of the fact that this system supports switching in a
   different eprom at $8000 (followed by a reset) it is possible
   to place other 8kb carts in the eproms and use them.

   The bank selecting is done by writing to $DE00.

   The values for the (extra) eprom banks are:

   eprom banks  1- 8 : $38-$3F
   eprom banks  9-16 : $28-$2F
   eprom banks 17-24 : $18-$1F
   eprom banks 25-32 : $08-$0F

   Setting bit 7 high will switch off EXROM.
*/

/* ---------------------------------------------------------------------*/

static int currbank = 0;
static BYTE regval = 0;

static void delaep256_io1_store(WORD addr, BYTE value)
{
    BYTE bank, config;

    regval = value;

    /* D7 switches off EXROM */
    config = (value & 0x80) ? 2 : 0;

    cart_config_changed_slotmain(config, config, CMODE_WRITE);

    bank = ((0x30 - (value & 0x30)) >> 1) + (value & 7) + 1;

    if (bank < 1 || bank > 32) {
        bank = 0;
    }

    cart_romlbank_set_slotmain(bank);
    currbank = bank;
}

static BYTE delaep256_io1_peek(WORD addr)
{
    return regval;
}

static int delaep256_dump(void)
{
    mon_out("Currently selected EPROM bank: %d, cart status: %s\n",
            currbank,
            (regval & 0x80) ? "Disabled" : "Enabled");
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t delaep256_device = {
    CARTRIDGE_NAME_DELA_EP256,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    delaep256_io1_store,
    NULL,
    delaep256_io1_peek,
    delaep256_dump,
    CARTRIDGE_DELA_EP256,
    0,
    0
};

static io_source_list_t *delaep256_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_DELA_EP256, 0, 1, &delaep256_device, NULL, CARTRIDGE_DELA_EP256
};

/* ---------------------------------------------------------------------*/

void delaep256_config_init(void)
{
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    cart_romlbank_set_slotmain(0);
}

void delaep256_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000 * 33);
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    cart_romlbank_set_slotmain(0);
}

/* ---------------------------------------------------------------------*/
static int delaep256_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    delaep256_list_item = io_source_register(&delaep256_device);
    return 0;
}

int delaep256_bin_attach(const char *filename, BYTE *rawcart)
{
    int size = 0x42000;

    memset(rawcart, 0xff, 0x42000);
    while (size != 0) {
        if (util_file_load(filename, rawcart, size, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            size -= 0x2000;
        } else {
            return delaep256_common_attach();
        }
    }
    return -1;
}

int delaep256_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    memset(rawcart, 0xff, 0x42000);

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > 32 || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }
    return delaep256_common_attach();
}

void delaep256_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(delaep256_list_item);
    delaep256_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTDELAEP256 snapshot module format:

   type  | name   | version | description
   --------------------------------------
   BYTE  | regval |   0.1   | register
   BYTE  | bank   |   0.0+  | current bank
   ARRAY | ROML   |   0.0+  | 262144 BYTES of ROML data
 */

static char snap_module_name[] = "CARTDELAEP256";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int delaep256_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, regval) < 0)
        || (SMW_B(m, (BYTE)currbank) < 0)
        || (SMW_BA(m, roml_banks, 0x2000 * 32) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int delaep256_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept higher versions than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* new in 0.1 */
    if (SNAPVAL(vmajor, vminor, 0, 1)) {
        if (SMR_B(m, &regval) < 0) {
            goto fail;
        }
    } else {
        regval = 0;
    }

    if (0
        || (SMR_B_INT(m, &currbank) < 0)
        || (SMR_BA(m, roml_banks, 0x2000 * 32) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    return delaep256_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
