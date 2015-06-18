/*
 * mach5.c - Cartridge handling, Mach 5 cart.
 *
 * Written by
 * Groepaz <groepaz@gmx.net>
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
#include "cartio.h"
#include "cartridge.h"
#include "mach5.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    This cart has 8Kb ROM mapped at $8000-$9FFF.

    The $9E00-$9EFF range is mirrored at $DE00-$DEFF.
    The $9F00-$9FFF range is mirrored at $DF00-$DFFF.
*/

/* #define DEBUGMACH5 */

#ifdef DEBUGMACH5
#define DBG(x) printf x
#else
#define DBG(x)
#endif

static BYTE mach5_io1_read(WORD addr)
{
/*    DBG(("io1 rd %04x\n", addr)); */
    return roml_banks[0x1e00 + (addr & 0xff)];
}

static void mach5_io1_store(WORD addr, BYTE value)
{
    DBG(("io1 st %04x %02x\n", addr, value));
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_WRITE);
}

static BYTE mach5_io2_read(WORD addr)
{
/*    DBG(("io2 rd %04x\n", addr)); */
    return roml_banks[0x1f00 + (addr & 0xff)];
}

static void mach5_io2_store(WORD addr, BYTE value)
{
    DBG(("%04x io2 st %04x %02x\n", reg_pc, addr, value));
    cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM, CMODE_WRITE);
}

/* ---------------------------------------------------------------------*/

static io_source_t mach5_io1_device = {
    CARTRIDGE_NAME_MACH5,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    1, /* read is always valid */
    mach5_io1_store,
    mach5_io1_read,
    mach5_io1_read,
    NULL,
    CARTRIDGE_MACH5,
    0,
    0
};

static io_source_t mach5_io2_device = {
    CARTRIDGE_NAME_MACH5,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    1, /* read is always valid */
    mach5_io2_store,
    mach5_io2_read,
    mach5_io2_read,
    NULL,
    CARTRIDGE_MACH5,
    0,
    0
};

static io_source_list_t *mach5_io1_list_item = NULL;
static io_source_list_t *mach5_io2_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_MACH5, 1, 0, &mach5_io1_device, &mach5_io2_device, CARTRIDGE_MACH5
};

/* ---------------------------------------------------------------------*/

void mach5_config_init(void)
{
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

void mach5_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000);
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int mach5_common_attach(void)
{
    if (c64export_add(&export_res) < 0) {
        return -1;
    }
    mach5_io1_list_item = io_source_register(&mach5_io1_device);
    mach5_io2_list_item = io_source_register(&mach5_io2_device);
    return 0;
}

int mach5_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x2000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        if (util_file_load(filename, rawcart, 0x1000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            return -1;
        }
        memcpy(&rawcart[0x1000], rawcart, 0x1000);
    }

    return mach5_common_attach();
}

int mach5_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.size == 0x1000) {
        if (crt_read_chip(rawcart, 0, &chip, fd)) {
            return -1;
        }
        memcpy(&rawcart[0x1000], rawcart, 0x1000);
    } else if (chip.size == 0x2000) {
        if (crt_read_chip(rawcart, 0, &chip, fd)) {
            return -1;
        }
    } else {
        return -1;
    }

    return mach5_common_attach();
}

void mach5_detach(void)
{
    c64export_remove(&export_res);
    io_source_unregister(mach5_io1_list_item);
    io_source_unregister(mach5_io2_list_item);
    mach5_io1_list_item = NULL;
    mach5_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTMACH5"

int mach5_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_BA(m, roml_banks, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int mach5_snapshot_read_module(snapshot_t *s)
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
        || (SMR_BA(m, roml_banks, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return mach5_common_attach();
}
