/*
 * kcs.c - Cartridge handling, KCS cart.
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

/* #define DEBUG_KCS */
#ifdef DEBUG_KCS
#define DBG(_x_)        log_debug _x_
#else
#define DBG(_x_)
#endif

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
#include "kcs.h"
#include "log.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    KCS Power Cartridge

    - 16kb ROM, 128 bytes RAM

    io1:
    - the second last page of the first 8k ROM bank is visible
    - when reading, bit 1 of the address selects mapping mode:
      0 : 8k game
      1 : cartridge disabled
    - when writing, 16k game mode is selected

    io2:
    - cartridge RAM (128 bytes)
    - when reading, if bit 7 of the address is set, freeze mode (nmi)
      is released.
    - when writing, and ultimax (freeze) mode is NOT active, then
      16k game mode is selected.
    - writes go to cartridge RAM

    FIXME: the above is still not 100% correct
     - BLOADing a frozen program does not work
*/

static int freeze_flag = 0;
static int config;

static BYTE kcs_io1_read(WORD addr)
{
    DBG(("io1 r %04x (%s)", addr, (addr & 2) ? "cart off" : "to 8k"));

    /* A1 switches off roml/romh banks */
    config = (addr & 2) ? CMODE_RAM : CMODE_8KGAME;

    cart_config_changed_slotmain((BYTE)config, (BYTE)config, CMODE_READ);
    freeze_flag = 0;
    return roml_banks[0x1e00 + (addr & 0xff)];
}

static BYTE kcs_io1_peek(WORD addr)
{
    return roml_banks[0x1e00 + (addr & 0xff)];
}

static void kcs_io1_store(WORD addr, BYTE value)
{
    DBG(("io1 w %04x %02x (to 16k)", addr, value));
    config = CMODE_16KGAME;
    cart_config_changed_slotmain((BYTE)config, (BYTE)config, CMODE_WRITE);
    freeze_flag = 0;
}

static BYTE kcs_io2_read(WORD addr)
{
    DBG(("io2 r %04x (%s)", addr, (addr & 0x80) ? "release NMI" : "-"));
    if (addr & 0x80) {
        cart_config_changed_slotmain((BYTE)config, (BYTE)config, CMODE_READ | CMODE_RELEASE_FREEZE);
        freeze_flag = 1;
    }
    return export_ram0[0x1f00 + (addr & 0x7f)];
}

static BYTE kcs_io2_peek(WORD addr)
{
    return export_ram0[0x1f00 + (addr & 0x7f)];
}

static void kcs_io2_store(WORD addr, BYTE value)
{
    DBG(("io2 w %04x %02x (%s)", addr, value, (freeze_flag == 0) ? "to 16k" : "-"));
    if (freeze_flag == 0) {
        config = CMODE_16KGAME;
        cart_config_changed_slotmain((BYTE)config, (BYTE)config, CMODE_WRITE);
    }
    export_ram0[0x1f00 + (addr & 0x7f)] = value;
}

/* ---------------------------------------------------------------------*/

static io_source_t kcs_io1_device = {
    CARTRIDGE_NAME_KCS_POWER,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    1, /* read is always valid */
    kcs_io1_store,
    kcs_io1_read,
    kcs_io1_peek,
    NULL,
    CARTRIDGE_KCS_POWER,
    0,
    0
};

static io_source_t kcs_io2_device = {
    CARTRIDGE_NAME_KCS_POWER,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    1, /* read is always valid */
    kcs_io2_store,
    kcs_io2_read,
    kcs_io2_peek,
    NULL,
    CARTRIDGE_KCS_POWER,
    0,
    0
};

static io_source_list_t *kcs_io1_list_item = NULL;
static io_source_list_t *kcs_io2_list_item = NULL;

static const c64export_resource_t export_res_kcs = {
    CARTRIDGE_NAME_KCS_POWER, 1, 1, &kcs_io1_device, &kcs_io2_device, CARTRIDGE_KCS_POWER
};

/* ---------------------------------------------------------------------*/

void kcs_freeze(void)
{
    config = CMODE_ULTIMAX;
    cart_config_changed_slotmain(CMODE_ULTIMAX, CMODE_ULTIMAX, CMODE_READ);
    freeze_flag = 1;
}

void kcs_config_init(void)
{
    config = CMODE_8KGAME;
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    freeze_flag = 0;
}

void kcs_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000);
    memcpy(romh_banks, &rawcart[0x2000], 0x2000);
    config = CMODE_8KGAME;
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    freeze_flag = 0;
}

/* ---------------------------------------------------------------------*/

static int kcs_common_attach(void)
{
    if (c64export_add(&export_res_kcs) < 0) {
        return -1;
    }

    kcs_io1_list_item = io_source_register(&kcs_io1_device);
    kcs_io2_list_item = io_source_register(&kcs_io2_device);
    return 0;
}

int kcs_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x4000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return kcs_common_attach();
}

int kcs_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i <= 1; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }

        if ((chip.start != 0x8000 && chip.start != 0xa000) || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.start - 0x8000, &chip, fd)) {
            return -1;
        }
    }

    return kcs_common_attach();
}

void kcs_detach(void)
{
    c64export_remove(&export_res_kcs);
    io_source_unregister(kcs_io1_list_item);
    io_source_unregister(kcs_io2_list_item);
    kcs_io1_list_item = NULL;
    kcs_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   2
#define SNAP_MODULE_NAME  "CARTKCS"

int kcs_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)freeze_flag) < 0)
        || (SMW_B(m, (BYTE)config) < 0)
        || (SMW_BA(m, roml_banks, 0x2000) < 0)
        || (SMW_BA(m, romh_banks, 0x2000) < 0)
        || (SMW_BA(m, export_ram0, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int kcs_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B_INT(m, &freeze_flag) < 0)
        || (SMR_B_INT(m, &config) < 0)
        || (SMR_BA(m, roml_banks, 0x2000) < 0)
        || (SMR_BA(m, romh_banks, 0x2000) < 0)
        || (SMR_BA(m, export_ram0, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return kcs_common_attach();
}
