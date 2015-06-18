/*
 * ocean.c - Cartridge handling, Ocean cart.
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
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64export.h"
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "monitor.h"
#include "ocean.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    "Ocean" Game Cartridges

    "Type A" (up to 256k):

    128k        Batman
    128k        Battle Command
    128k        Double Dragon
    128k        Navy Seals
    128k        Pang
    128k        Robocop 3
    128k        Toki

    256k        Chase H.Q. II
    256k        Robocop 2
    256k        Shadow of the Beast
    256k        Space Gun

    "Type B" (512k):

    512k        Terminator 2

    Note: other sizes (such as 32k) exist in the wild as crt files, but these
          do not seem to be official ocean cartridges

    "Type A" cartridges work like this:

    - the hardware uses 16k game config, the same 8k ROM bank is mapped
      to both ROML and ROMH.
    - the software accesses banks 0-15 at ROML ($8000-$9FFF) and banks 16-31 at
      ROMH ($A000-$BFFF) (only 256k type)

    "Type B" cartridges work like this:

    - the hardware uses 8k game config, 8k ROM bank is mapped to ROML.
    - the software accesses banks 0-63 at ROML ($8000-$9FFF).

    Note: a generic implementation may always use 16k game config and mirror the
          ROM banks (the terminator2 cart will work with it)

    Bank switching is done by writing to $DE00. The lower six bits give the
    bank number (ranging from 0-63). Bit 7 in this selection word is always
    set (but has no effect).
*/

/* #define ALWAYS16K */

/* ---------------------------------------------------------------------*/

static BYTE currbank = 0;
static BYTE regval = 0;
static BYTE io1_mask = 0x3f;
static unsigned int cart_size = 0;

/* ---------------------------------------------------------------------*/
static void ocean_io1_store(WORD addr, BYTE value)
{
    addr &= 0xff;
    regval = value;
    currbank = value & io1_mask & 0x3f;

    cart_romlbank_set_slotmain(currbank);
    cart_port_config_changed_slotmain();
}

static BYTE ocean_io1_peek(WORD addr)
{
    return regval;
}

static int ocean_dump(void)
{
    mon_out("Bank: %d\n", currbank);
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t ocean_device = {
    CARTRIDGE_NAME_OCEAN,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    ocean_io1_store,
    NULL,
    ocean_io1_peek,
    ocean_dump,
    CARTRIDGE_OCEAN,
    0,
    0
};

static io_source_list_t *ocean_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_OCEAN, 1, 1, &ocean_device, NULL, CARTRIDGE_OCEAN
};

/* ---------------------------------------------------------------------*/

BYTE ocean_romh_read(WORD addr)
{
    /* 256 kB OCEAN carts may access memory either at $8000 or $a000 */
    return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
}

void ocean_config_init(void)
{
    ocean_io1_store((WORD)0xde00, 0);
#ifdef ALWAYS16K
    /* Hack: using 16kB configuration, but some carts are 8kB only */
    cart_config_changed_slotmain(1, 1, CMODE_READ);
#else
    if (cart_size == 0x80000) {
        /* 8k configuration */
        cart_config_changed_slotmain(0, 0, CMODE_READ);
    } else {
        /* 16kB configuration */
        cart_config_changed_slotmain(1, 1, CMODE_READ);
    }
#endif
}

void ocean_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000 * 64);
#ifdef ALWAYS16K
    /* Hack: using 16kB configuration, but some carts are 8kB only */
    cart_config_changed_slotmain(1, 1, CMODE_READ);
#else
    if (cart_size == 0x80000) {
        /* 8k configuration */
        cart_config_changed_slotmain(0, 0, CMODE_READ);
    } else {
        /* 16kB configuration */
        cart_config_changed_slotmain(1, 1, CMODE_READ);
    }
#endif
}

/* ---------------------------------------------------------------------*/

static int ocean_common_attach(void)
{
    if (c64export_add(&export_res) < 0) {
        return -1;
    }
    ocean_list_item = io_source_register(&ocean_device);
    return 0;
}
/* ---------------------------------------------------------------------*/

/* HACK: 32k isnt really a valid size, see above */
int ocean_cart_sizes[] = { 0x80000, 0x40000, 0x20000, 0x08000, 0 };

int ocean_bin_attach(const char *filename, BYTE *rawcart)
{
    int rc = -1;
    int i;
    size_t size;
    for (i = 0; (size = ocean_cart_sizes[i]) != 0; i++) {
        rc = util_file_load(filename, rawcart, size, UTIL_FILE_LOAD_SKIP_ADDRESS);
        if (rc == 0) {
            io1_mask = (size >> 13) - 1;
            /* printf("rc=%d i=%d sz=0x%x mask=0x%02x\n", rc, i, size, io1_mask); */
            break;
        }
    }

    if (rc == 0) {
        cart_size = size;
        rc = ocean_common_attach();
    }
    return rc;
}

int ocean_crt_attach(FILE *fd, BYTE *rawcart)
{
    size_t rom_size;
    crt_chip_header_t chip;

    rom_size = 0;
    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }
        if ((chip.bank > 63) || ((chip.start != 0x8000) && (chip.start != 0xa000)) || (chip.size != 0x2000)) {
            return -1;
        }
        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
        rom_size += chip.size;
    }

    io1_mask = (rom_size >> 13) - 1;
    cart_size = rom_size;

    return ocean_common_attach();
}

void ocean_detach(void)
{
    c64export_remove(&export_res);
    io_source_unregister(ocean_list_item);
    ocean_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   1
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTOCEAN"

int ocean_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)currbank) < 0)
        || (SMW_B(m, (BYTE)io1_mask) < 0)
        || (SMW_B(m, (BYTE)regval) < 0)
        || (SMW_DW(m, (DWORD)cart_size) < 0)
        || (SMW_BA(m, roml_banks, 0x2000 * 64) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int ocean_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B(m, &currbank) < 0)
        || (SMR_B(m, &io1_mask) < 0)
        || (SMR_B(m, &regval) < 0)
        || (SMR_DW_UINT(m, &cart_size) < 0)
        || (SMR_BA(m, roml_banks, 0x2000 * 64) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return ocean_common_attach();
}
