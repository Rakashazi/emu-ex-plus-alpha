/*
 * prophet64.c - Cartridge handling, Prophet 64 cart.
 *
 * Written by
 *  Groepaz <groepaz@gmx.net>
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
#include "prophet64.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    32 banks, 8k each == 256kb

    banks are always mapped to $8000

    controlregister is $df00

    lower 5 bits ($00..$1f) selects bank
    bit 6 ($2x) disables cartridge
*/

#define PROPHET64_CART_SIZE (256 * 0x400)

/* ---------------------------------------------------------------------*/

static int currbank = 0;
static BYTE regval = 0;

static void p64_io2_store(WORD addr, BYTE value)
{
    regval = value;

    /* confirmation needed: register mirrored in entire io2 ? */
    if ((value >> 5) & 1) {
        /* cartridge off */
        cart_config_changed_slotmain(2, 2, CMODE_READ);
    } else {
        /* cartridge on */
        cart_config_changed_slotmain(0, 0, CMODE_READ);
    }
    currbank = value & 0x1f;
    cart_romlbank_set_slotmain(value & 0x1f);
}

static BYTE p64_io2_peek(WORD addr)
{
    return regval;
}

static int p64_dump(void)
{
    mon_out("Bank: %d\n", currbank);
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t p64_device = {
    CARTRIDGE_NAME_P64,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    0, /* read is never valid */
    p64_io2_store,
    NULL,
    p64_io2_peek,
    p64_dump,
    CARTRIDGE_P64,
    0,
    0
};

static io_source_list_t *p64_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_P64, 0, 1, NULL, &p64_device, CARTRIDGE_P64
};

/* ---------------------------------------------------------------------*/

void p64_config_init(void)
{
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    cart_romlbank_set_slotmain(0);
}

void p64_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, PROPHET64_CART_SIZE);
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    cart_romlbank_set_slotmain(0);
}

/* ---------------------------------------------------------------------*/

static int p64_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    p64_list_item = io_source_register(&p64_device);

    return 0;
}

int p64_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, PROPHET64_CART_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return p64_common_attach();
}

int p64_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int i, cnt = 0;

    for (i = 0; i <= 0x1f; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > 0x1f || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
        cnt++;
    }

    return p64_common_attach();
}

void p64_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(p64_list_item);
    p64_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTP64 snapshot module format:

   type  | name     | version | description
   ----------------------------------------
   BYTE  | bank     |   0.1   | current bank
   BYTE  | register |   0.1   | register
   ARRAY | ROML     |   0.0+  | 262144 BYTES of ROML data
 */

static char snap_module_name[] = "CARTP64";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int p64_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)currbank) < 0
        || SMW_B(m, regval) < 0
        || SMW_BA(m, roml_banks, PROPHET64_CART_SIZE) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int p64_snapshot_read_module(snapshot_t *s)
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
            || SMR_B_INT(m, &currbank) < 0
            || SMR_B(m, &regval) < 0) {
            goto fail;
        }
    } else {
        currbank = 0;
        regval = 0;
    }

    if (SMR_BA(m, roml_banks, PROPHET64_CART_SIZE) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return p64_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
