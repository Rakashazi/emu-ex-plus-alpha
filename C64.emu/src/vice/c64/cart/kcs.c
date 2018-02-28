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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "cartio.h"
#include "cartridge.h"
#include "crt.h"
#include "export.h"
#include "kcs.h"
#include "log.h"
#include "maincpu.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "vicii-phi1.h"

/*
    KCS Power Cartridge

    - 16kb ROM, 128 bytes RAM

    It's rather simple:

    io1:
    - the second last page of the first 8k ROM bank is visible
    - bit 1 of the address sets EXROM and R/W sets GAME

    io2:
    - 00-7f cartridge RAM (128 bytes), writable
    - 80-ff open area where the GAME/EXROM lines can be read (pull resistor hack)

    - the cartridge starts in 16k game mode

    the original software uses:

    bit $de00   -> ROMH off

    sta $de00
    sta $de02
    sta $de80   -> ROMH on (a000)

    bit $df80 at beginning of freezer NMI to find GAME/EXROM status

    ... and also code is running in deXX area
*/

/*
 * ROM is selected if:                 OE = (!IO1 = 0) | !(!ROMH & !ROML)
 * RAM is selected if:                 CS = PHI2 & (!A7) & (!IO2)
 *                                      (not A4, that's a mistake on the schematic)
 */

/*      74LS275 (4 flip flops)
 *
 *      !reset <- !reset
 *      C1     <- 74LS00 pin8
 *
 *      Q (out)      D (in)
 *
 *      D6     Q0 <- D0  !GAME
 *      !GAME  Q1 <- D1  R/!W
 *      D7     Q2 <- D2  !EXROM
 *      !EXROM Q3 <- D3  74LS02 pin13
 *
 */

static int config;

static BYTE kcs_io1_read(WORD addr)
{
    config = (addr & 2) ? CMODE_RAM : CMODE_8KGAME;

    cart_config_changed_slotmain((BYTE)config, (BYTE)config, CMODE_READ);
    return roml_banks[0x1e00 + (addr & 0xff)];
}

static BYTE kcs_io1_peek(WORD addr)
{
    return roml_banks[0x1e00 + (addr & 0xff)];
}

static void kcs_io1_store(WORD addr, BYTE value)
{
    config = (addr & 2) ? CMODE_ULTIMAX : CMODE_16KGAME;

    cart_config_changed_slotmain((BYTE)config, (BYTE)config, CMODE_WRITE);
}

static BYTE kcs_io2_read(WORD addr)
{
    /* the software reads from df80 at beginning of nmi handler */
    /* to determine the status of GAME and EXROM lines */
    if (addr & 0x80) {
        return ((config & 2) ? 0x80 : 0) | ((config & 1) ? 0 : 0x40) | (vicii_read_phi1() & 0x3f); /* DF80-DFFF actual config */
    }
    return export_ram0[addr & 0x7f];
}

static BYTE kcs_io2_peek(WORD addr)
{
    if (addr & 0x80) {
        return ((config & 2) ? 0x80 : 0) | ((config & 1) ? 0 : 0x40); /* DF80-DFFF actual config */
    }
    return export_ram0[addr & 0x7f];
}

static void kcs_io2_store(WORD addr, BYTE value)
{
    if (addr & 0x80) {
        return; /* open area for GAME/EXROM status */
    }
    export_ram0[addr & 0x7f] = value;
}

static int kcs_io1_dump(void)
{
    mon_out("EXROM: %d GAME: %d (%s)\n", ((config >> 1) & 1), (config & 1) ^ 1, cart_config_string((BYTE)(config & 3)));
    return 0;
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
    kcs_io1_dump,
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
    NULL, /* TODO: dump */
    CARTRIDGE_KCS_POWER,
    0,
    0
};

static io_source_list_t *kcs_io1_list_item = NULL;
static io_source_list_t *kcs_io2_list_item = NULL;

static const export_resource_t export_res_kcs = {
    CARTRIDGE_NAME_KCS_POWER, 1, 1, &kcs_io1_device, &kcs_io2_device, CARTRIDGE_KCS_POWER
};

/* ---------------------------------------------------------------------*/

void kcs_freeze(void)
{
    config = CMODE_ULTIMAX;
    cart_config_changed_slotmain((BYTE)config, (BYTE)config, CMODE_READ | CMODE_RELEASE_FREEZE);
}

void kcs_config_init(void)
{
    config = CMODE_16KGAME;
    cart_config_changed_slotmain((BYTE)config, (BYTE)config, CMODE_READ);
}

void kcs_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000);
    memcpy(romh_banks, &rawcart[0x2000], 0x2000);
    config = CMODE_16KGAME;
    cart_config_changed_slotmain((BYTE)config, (BYTE)config, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int kcs_common_attach(void)
{
    if (export_add(&export_res_kcs) < 0) {
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
    export_remove(&export_res_kcs);
    io_source_unregister(kcs_io1_list_item);
    io_source_unregister(kcs_io2_list_item);
    kcs_io1_list_item = NULL;
    kcs_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTKCS snapshot module format:

   type  | name   | version | description
   --------------------------------------
   BYTE  | config |   0.2   | current configuration
   ARRAY | ROML   |   0.0+  | 8192 BYTES of ROML data
   ARRAY | ROMH   |   0.0+  | 8192 BYTES of ROMH data
   ARRAY | RAM    |   0.0+  | 128 BYTES of RAM data

Note: in snapshots before 0.3 the RAM size was 8192.
 */

static char snap_module_name[] = "CARTKCS";
#define SNAP_MAJOR   0
#define SNAP_MINOR   3

int kcs_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)config) < 0
        || SMW_BA(m, roml_banks, 0x2000) < 0
        || SMW_BA(m, romh_banks, 0x2000) < 0
        || SMW_BA(m, export_ram0, 128) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int kcs_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    BYTE dummy;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* added in 0.1, removed in 0.3 */
    if (SNAPVAL(vmajor, vminor, 0, 1) && !SNAPVAL(vmajor, vminor, 0, 3)) {
        if (SMR_B(m, &dummy) < 0) {
            goto fail;
        }
    }

    /* new in 0.2 */
    if (SNAPVAL(vmajor, vminor, 0, 2)) {
        if (SMR_B_INT(m, &config) < 0) {
            goto fail;
        }
    } else {
        config = 0;
    }

    if (0
        || SMR_BA(m, roml_banks, 0x2000) < 0
        || SMR_BA(m, romh_banks, 0x2000) < 0) {
        goto fail;
    }

    /* 0x80 in 0.3, 0x2000 before that */
    if (SNAPVAL(vmajor, vminor, 0, 3)) {
        if (SMR_BA(m, export_ram0, 128) < 0) {
            goto fail;
        }
    } else {
        if (SMR_BA(m, export_ram0, 0x2000) < 0) {
            goto fail;
        }
    }

    snapshot_module_close(m);

    return kcs_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
