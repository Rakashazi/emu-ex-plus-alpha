/*
 * freezeframe.c - Cartridge handling, Freeze Frame cart.
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
#include "freezeframe.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/* #define FFDEBUG */

#ifdef FFDEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    Evesham Micros "Freeze Frame"

    - 8k ROM

    - reading io1 (the software uses de00 only it seems)
      - switches to 8k game mode
      - ROM is mapped to 8000

    - reading io2 (the software uses df00 only it seems)
      - disables cartridge ROM

    - reset
      - enables 8K game mode
      - ROM bank is mapped to 8000

    - freeze
      - enables ultimax mode
      - ROM is mapped to 8000
      - ROM is mapped to E000
*/

#define FREEZE_FRAME_CART_SIZE (8 * 0x400)

/* ---------------------------------------------------------------------*/

static BYTE freezeframe_io1_read(WORD addr)
{
    DBG(("io1 r %04x\n", addr));
    if (addr == 0) {
        cart_config_changed_slotmain(2, 1, CMODE_READ);
        DBG(("Freeze Frame: switching to 8k game mode\n"));
    }
    return 0; /* invalid */
}

static BYTE freezeframe_io1_peek(WORD addr)
{
    return 0; /* invalid */
}

static void freezeframe_io1_store(WORD addr, BYTE value)
{
    DBG(("io1 %04x %02x\n", addr, value));
}

static BYTE freezeframe_io2_read(WORD addr)
{
    DBG(("io2 r %04x\n", addr));
    if (addr == 0) {
        cart_config_changed_slotmain(2, 2, CMODE_READ);
        DBG(("Freeze Frame disabled\n"));
    }
    return 0; /* invalid */
}

static BYTE freezeframe_io2_peek(WORD addr)
{
    return 0; /* invalid */
}

static void freezeframe_io2_store(WORD addr, BYTE value)
{
    DBG(("io2 %04x %02x\n", addr, value));
}

static io_source_t freezeframe_io1_device = {
    CARTRIDGE_NAME_FREEZE_FRAME,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0, /* read is never valid */
    freezeframe_io1_store,
    freezeframe_io1_read,
    freezeframe_io1_peek,
    NULL,
    CARTRIDGE_FREEZE_FRAME,
    0,
    0
};

static io_source_t freezeframe_io2_device = {
    CARTRIDGE_NAME_FREEZE_FRAME,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    0, /* read is never valid */
    freezeframe_io2_store,
    freezeframe_io2_read,
    freezeframe_io2_peek,
    NULL,
    CARTRIDGE_FREEZE_FRAME,
    0,
    0
};

static io_source_list_t *freezeframe_io1_list_item = NULL;
static io_source_list_t *freezeframe_io2_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_FREEZE_FRAME, 1, 1, &freezeframe_io1_device, &freezeframe_io2_device, CARTRIDGE_FREEZE_FRAME
};

/* ---------------------------------------------------------------------*/

void freezeframe_freeze(void)
{
    DBG(("Freeze Frame: freeze\n"));
    cart_config_changed_slotmain(2, 3, CMODE_READ | CMODE_RELEASE_FREEZE);
}

void freezeframe_config_init(void)
{
    cart_config_changed_slotmain(2, 0, CMODE_READ);
}

void freezeframe_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, FREEZE_FRAME_CART_SIZE);
    memcpy(romh_banks, rawcart, FREEZE_FRAME_CART_SIZE);
    cart_config_changed_slotmain(2, 0, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int freezeframe_common_attach(void)
{
    if (c64export_add(&export_res) < 0) {
        return -1;
    }

    freezeframe_io1_list_item = io_source_register(&freezeframe_io1_device);
    freezeframe_io2_list_item = io_source_register(&freezeframe_io2_device);

    return 0;
}

int freezeframe_bin_attach(const char *filename, BYTE *rawcart)
{
    DBG(("Freeze Frame: bin attach '%s'\n", filename));
    if (util_file_load(filename, rawcart, FREEZE_FRAME_CART_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return freezeframe_common_attach();
}

int freezeframe_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.bank > 0 || chip.size != FREEZE_FRAME_CART_SIZE) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return freezeframe_common_attach();
}

void freezeframe_detach(void)
{
    c64export_remove(&export_res);
    io_source_unregister(freezeframe_io1_list_item);
    io_source_unregister(freezeframe_io2_list_item);
    freezeframe_io1_list_item = NULL;
    freezeframe_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTFREEZEF"

int freezeframe_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_BA(m, roml_banks, FREEZE_FRAME_CART_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int freezeframe_snapshot_read_module(snapshot_t *s)
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
        || (SMR_BA(m, roml_banks, FREEZE_FRAME_CART_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    memcpy(romh_banks, roml_banks, FREEZE_FRAME_CART_SIZE);

    return freezeframe_common_attach();
}
