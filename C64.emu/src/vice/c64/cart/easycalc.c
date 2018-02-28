/*
 * easycalc.c - Cartridge handling, Easy Calc Result cart.
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
#include "cartridge.h"
#include "crt.h"
#include "easycalc.h"
#include "export.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"

/*
    this cart uses 8Kb mapped in at $8000-$9FFF
    and 2 banks of 8Kb mapped in at $A000-$BFFF.

    the bank at $A000-$BFFF is selected by bit 0 of the address of a write
    access to I/O-1.

    A0 | bank
    ---------
     0 |  0
     1 |  1
 */

/* some prototypes are needed */
static void easycalc_io1_store(WORD addr, BYTE val);
static int easycalc_dump(void);

static io_source_t easycalc_device = {
    CARTRIDGE_NAME_EASYCALC,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0, /* read is never valid */
    easycalc_io1_store,
    NULL, /* no read */
    NULL, /* no peek */
    easycalc_dump,
    CARTRIDGE_EASYCALC,
    0,
    0
};

static io_source_list_t *easycalc_list_item = NULL;

static const export_resource_t export_res_easycalc = {
    CARTRIDGE_NAME_EASYCALC, 1, 1, NULL, &easycalc_device, CARTRIDGE_EASYCALC
};

/* ---------------------------------------------------------------------*/

void easycalc_config_init(void)
{
    cart_config_changed_slotmain(1, 1, CMODE_READ);
}

void easycalc_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000);
    memcpy(romh_banks, &rawcart[0x2000], 0x4000);
    cart_config_changed_slotmain(1, 1, CMODE_READ);
}

static int easycalc_common_attach(void)
{
    if (export_add(&export_res_easycalc) < 0) {
        return -1;
    }
    easycalc_list_item = io_source_register(&easycalc_device);

    return 0;
}

int easycalc_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x6000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return easycalc_common_attach();
}

int easycalc_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int i;

    /* first CHIP header holds $8000-$a000 data */
    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.start != 0x8000 || chip.size != 0x2000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    /* second/third CHIP headers hold $a000-$c000 banked data */
    for (i = 0; i <= 1; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }

        if (chip.start != 0xa000 || chip.size != 0x2000 || chip.bank > 1) {
            return -1;
        }

        if (crt_read_chip(rawcart, 0x2000 + (chip.bank << 13), &chip, fd)) {
            return -1;
        }
    }

    return easycalc_common_attach();
}

void easycalc_detach(void)
{
    export_remove(&export_res_easycalc);
    io_source_unregister(easycalc_list_item);
    easycalc_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

static int curbank = 0;

static void easycalc_io1_store(WORD addr, BYTE val)
{
    curbank = (addr & 1) ? 1 : 0;

    cart_romhbank_set_slotmain(curbank);
}

static int easycalc_dump(void)
{
    mon_out("Bank: %d", curbank);
    return 0;
}

/* ---------------------------------------------------------------------*/

/* CARTEASYCALC snapshot module format:

   type  | name | description
   --------------------------
   ARRAY | ROML | 8192 BYTES of ROML data
   ARRAY | ROMH | 16384 BYTES of ROMH data
 */

static char snap_module_name[] = "CARTEASYCALC";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int easycalc_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_BA(m, roml_banks, 0x2000) < 0)
        || (SMW_BA(m, romh_banks, 0x4000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int easycalc_snapshot_read_module(snapshot_t *s)
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
        || (SMR_BA(m, roml_banks, 0x2000) < 0)
        || (SMR_BA(m, romh_banks, 0x4000) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    return easycalc_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
