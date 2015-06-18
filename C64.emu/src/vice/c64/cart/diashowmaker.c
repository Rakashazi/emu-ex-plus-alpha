/*
 * diashowmaker.c - Cartridge handling, Diashow Maker cart.
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
#include "c64export.h"
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "diashowmaker.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/* #define DSMDEBUG */

#ifdef DSMDEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    "Diashow Maker" (c) M.Grieb & B.Trenkel

    - 8k ROM

    - accessing io1 (the software uses de00 only it seems)
      - disables cartridge ROM

    - reset
      - enables 8K game mode
      - ROM bank is mapped to 8000

    - freeze
      - ROM is mapped to 8000

    freeze, then:
        return:     main menu
        space:      dos-kit/fastload
        arrow left: normal reset
        f7:         cycle video banks (?)

*/

#define DSM_CART_SIZE (8 * 0x400)

/* ---------------------------------------------------------------------*/

static BYTE dsm_io1_read(WORD addr)
{
    DBG(("io1 r %04x\n", addr));
    if (addr == 0) {
        cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM, CMODE_READ);
        DBG(("Diashow Maker disabled\n"));
    }
    return 0; /* invalid */
}

static BYTE dsm_io1_peek(WORD addr)
{
    return 0;
}

static void dsm_io1_store(WORD addr, BYTE value)
{
    DBG(("io1 w %04x %02x\n", addr, value));
    if (addr == 0) {
        cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM, CMODE_READ);
        DBG(("Diashow Maker disabled\n"));
    }
}

static io_source_t dsm_io1_device = {
    CARTRIDGE_NAME_DIASHOW_MAKER,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0, /* read is never valid */
    dsm_io1_store,
    dsm_io1_read,
    dsm_io1_peek,
    NULL,
    CARTRIDGE_DIASHOW_MAKER,
    0,
    0
};

static io_source_list_t *dsm_io1_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_DIASHOW_MAKER, 1, 1, &dsm_io1_device, NULL, CARTRIDGE_DIASHOW_MAKER
};

/* ---------------------------------------------------------------------*/

void dsm_freeze(void)
{
    DBG(("Diashow Maker: freeze\n"));
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ | CMODE_RELEASE_FREEZE);
}


void dsm_config_init(void)
{
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

void dsm_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, DSM_CART_SIZE);
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int dsm_common_attach(void)
{
    if (c64export_add(&export_res) < 0) {
        return -1;
    }

    dsm_io1_list_item = io_source_register(&dsm_io1_device);
    return 0;
}

int dsm_bin_attach(const char *filename, BYTE *rawcart)
{
    DBG(("Diashow Maker: bin attach '%s'\n", filename));
    if (util_file_load(filename, rawcart, DSM_CART_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return dsm_common_attach();
}

int dsm_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.bank > 0 || chip.size != DSM_CART_SIZE) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return dsm_common_attach();
}

void dsm_detach(void)
{
    c64export_remove(&export_res);
    io_source_unregister(dsm_io1_list_item);
    dsm_io1_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTDSM"

int dsm_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_BA(m, roml_banks, DSM_CART_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int dsm_snapshot_read_module(snapshot_t *s)
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
        || (SMR_BA(m, roml_banks, DSM_CART_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return dsm_common_attach();
}
