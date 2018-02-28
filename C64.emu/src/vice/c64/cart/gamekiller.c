/*
 * gamekiller.c - Cartridge handling, Game Killer cart.
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
#include "gamekiller.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/* #define GKDEBUG */

#ifdef GKDEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    ROBTEK "Game Killer"

    - 1 8k ROM
    - when cartridge is active, ultimax is enabled when addr>=e000, so the
      rom is visible at e000, below is normal c64 ram
    - the code writes 0 to both de00 and df00 to disable the cartridge. we
      assume the cart uses the full io1 and io2 range
    - when the freezer button is pressed the cartridge will be enabled and
      an NMI will be triggered
*/

#define GAME_KILLER_CART_SIZE (8 * 0x400)

/* ---------------------------------------------------------------------*/

static int cartridge_disable_flag;

static void gamekiller_io1_store(WORD addr, BYTE value)
{
    DBG(("io1 %04x %02x\n", addr, value));
    cartridge_disable_flag++;
    if (cartridge_disable_flag > 1) {
        cart_config_changed_slotmain(2, 2, CMODE_READ);
        DBG(("Game Killer disabled\n"));
    }
}

static void gamekiller_io2_store(WORD addr, BYTE value)
{
    DBG(("io2 %04x %02x\n", addr, value));
    cartridge_disable_flag++;
    if (cartridge_disable_flag > 1) {
        cart_config_changed_slotmain(2, 2, CMODE_READ);
        DBG(("Game Killer disabled\n"));
    }
}

static BYTE gamekiller_peek(WORD addr)
{
    return 0;
}

static io_source_t gamekiller_io1_device = {
    CARTRIDGE_NAME_GAME_KILLER,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0, /* read is never valid */
    gamekiller_io1_store,
    NULL,
    gamekiller_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_GAME_KILLER,
    0,
    0
};

static io_source_t gamekiller_io2_device = {
    CARTRIDGE_NAME_GAME_KILLER,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    0, /* read is never valid */
    gamekiller_io2_store,
    NULL,
    gamekiller_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_GAME_KILLER,
    0,
    0
};

static io_source_list_t *gamekiller_io1_list_item = NULL;
static io_source_list_t *gamekiller_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_GAME_KILLER, 1, 0, &gamekiller_io1_device, &gamekiller_io2_device, CARTRIDGE_GAME_KILLER
};

/* ---------------------------------------------------------------------*/

int gamekiller_peek_mem(export_t *export, WORD addr, BYTE *value)
{
    if (cartridge_disable_flag <= 1) {
        if (addr >= 0xe000) {
            *value = romh_banks[addr & 0x1fff];
            return CART_READ_VALID;
        }
    }
    return CART_READ_THROUGH;
}

void gamekiller_freeze(void)
{
    DBG(("Game Killer freeze\n"));
    cart_config_changed_slotmain(3, 3, CMODE_READ | CMODE_RELEASE_FREEZE);
    cartridge_disable_flag = 0;
}

void gamekiller_config_init(void)
{
    cart_config_changed_slotmain(3, 3, CMODE_READ);
    cartridge_disable_flag = 0;
}

void gamekiller_config_setup(BYTE *rawcart)
{
    memcpy(romh_banks, rawcart, GAME_KILLER_CART_SIZE);
    cart_config_changed_slotmain(3, 3, CMODE_READ);
    cartridge_disable_flag = 0;
}

/* ---------------------------------------------------------------------*/

static int gamekiller_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    gamekiller_io1_list_item = io_source_register(&gamekiller_io1_device);
    gamekiller_io2_list_item = io_source_register(&gamekiller_io2_device);

    return 0;
}

int gamekiller_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, GAME_KILLER_CART_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return gamekiller_common_attach();
}

int gamekiller_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.bank > 0 || chip.size != GAME_KILLER_CART_SIZE) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return gamekiller_common_attach();
}

void gamekiller_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(gamekiller_io1_list_item);
    io_source_unregister(gamekiller_io2_list_item);
    gamekiller_io1_list_item = NULL;
    gamekiller_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTGK snapshot module format:

   type  | name    | description
   -----------------------------
   BYTE  | disable | cartridge disable flag
   ARRAY | ROMH    | 8192 BYTES of ROMH data
 */

static char snap_module_name[] = "CARTGK";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int gamekiller_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)cartridge_disable_flag) < 0)
        || (SMW_BA(m, romh_banks, GAME_KILLER_CART_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int gamekiller_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B_INT(m, &cartridge_disable_flag) < 0)
        || (SMR_BA(m, romh_banks, GAME_KILLER_CART_SIZE) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    return gamekiller_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
