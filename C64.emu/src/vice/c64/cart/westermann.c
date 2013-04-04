/*
 * westermann.c - Cartridge handling, Westermann cart.
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
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "westermann.h"
#include "crt.h"

/*
    Westermann Utility Cartridge

    16kb ROM

    - starts in 16k game config
    - any read access to io2 switches to 8k game config
*/

/* some prototypes are needed */
static BYTE westermann_io2_read(WORD addr);
static BYTE westermann_io2_peek(WORD addr);

static io_source_t westermann_device = {
    CARTRIDGE_NAME_WESTERMANN,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    0, /* read is never valid */
    NULL,
    westermann_io2_read,
    westermann_io2_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_WESTERMANN,
    0,
    0
};

static io_source_list_t *westermann_list_item = NULL;

static const c64export_resource_t export_res_westermann = {
    CARTRIDGE_NAME_WESTERMANN, 1, 0, NULL, &westermann_device, CARTRIDGE_WESTERMANN
};

/* ---------------------------------------------------------------------*/

static BYTE westermann_io2_read(WORD addr)
{
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    return 0;
}

static BYTE westermann_io2_peek(WORD addr)
{
    return 0;
}

/* ---------------------------------------------------------------------*/

void westermann_config_init(void)
{
    cart_config_changed_slotmain(1, 1, CMODE_READ);
}

void westermann_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000);
    memcpy(romh_banks, &rawcart[0x2000], 0x2000);
    cart_config_changed_slotmain(1, 1, CMODE_READ);
}

static int westermann_common_attach(void)
{
    if (c64export_add(&export_res_westermann) < 0) {
        return -1;
    }
    westermann_list_item = io_source_register(&westermann_device);

    return 0;
}

int westermann_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x4000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return westermann_common_attach();
}

int westermann_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.start != 0x8000 || chip.size != 0x4000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return westermann_common_attach();
}

void westermann_detach(void)
{
    c64export_remove(&export_res_westermann);
    io_source_unregister(westermann_list_item);
    westermann_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTWEST"

int westermann_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_BA(m, roml_banks, 0x2000) < 0)
        || (SMW_BA(m, romh_banks, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int westermann_snapshot_read_module(snapshot_t *s)
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
        || (SMR_BA(m, roml_banks, 0x2000) < 0)
        || (SMR_BA(m, romh_banks, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return westermann_common_attach();
}
