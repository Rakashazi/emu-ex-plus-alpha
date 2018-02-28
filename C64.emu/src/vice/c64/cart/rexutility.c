/*
 * rexutility.c - Cartridge handling, REX Utility cart.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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
#include "rexutility.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    REX Utility Cartridge

    - 8kb ROM

    - reading df00-dfbf disables ROM
    - reading dfc0-dfff enables ROM (8k game config)
*/

static int rex_active = 0;

static BYTE rex_io2_read(WORD addr)
{
    if ((addr & 0xff) < 0xc0) {
        /* disable cartridge rom */
        cart_config_changed_slotmain(2, 2, CMODE_READ);
        rex_active = 0;
    } else {
        /* enable cartridge rom */
        cart_config_changed_slotmain(0, 0, CMODE_READ);
        rex_active = 1;
    }
    return 0;
}

static BYTE rex_io2_peek(WORD addr)
{
    return 0;
}

static int rex_dump(void)
{
    mon_out("$8000-$9FFF ROM: %s\n", (rex_active) ? "enabled" : "disabled");

    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t rex_device = {
    CARTRIDGE_NAME_REX,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    0, /* read is never valid */
    NULL,
    rex_io2_read,
    rex_io2_peek,
    rex_dump,
    CARTRIDGE_REX,
    0,
    0
};

static io_source_list_t *rex_list_item = NULL;

static const export_resource_t export_res_rex = {
    CARTRIDGE_NAME_REX, 0, 1, NULL, &rex_device, CARTRIDGE_REX
};

/* ---------------------------------------------------------------------*/

void rex_config_init(void)
{
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    rex_active = 1;
}

void rex_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000);
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    rex_active = 1;
}

static int rex_common_attach(void)
{
    if (export_add(&export_res_rex) < 0) {
        return -1;
    }
    rex_list_item = io_source_register(&rex_device);
    return 0;
}

int rex_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x2000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return rex_common_attach();
}

int rex_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.size != 0x2000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return rex_common_attach();
}

void rex_detach(void)
{
    export_remove(&export_res_rex);
    io_source_unregister(rex_list_item);
    rex_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTREXUTIL snapshot module format:

   type  | name   | version | description
   --------------------------------------
   BYTE  | active |   0.1   | cartridge active flag
   ARRAY | ROML   |   0.0+  | 524288 BYTES of ROML data
 */

static char snap_module_name[] = "CARTREXUTIL";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int rex_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)rex_active) < 0
        || SMW_BA(m, roml_banks, 0x2000 * 64) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int rex_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if ((vmajor != SNAP_MAJOR) || (vminor != SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* new in 0.1 */
    if (SNAPVAL(vmajor, vminor, 0, 1)) {
        if (SMR_B_INT(m, &rex_active) < 0) {
            goto fail;
        }
    } else {
        rex_active = 0;
    }

    if (SMR_BA(m, roml_banks, 0x2000 * 64) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return rex_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
