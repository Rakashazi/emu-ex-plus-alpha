/*
 * supergames.c - Cartridge handling, Super Games cart.
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
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "monitor.h"
#include "snapshot.h"
#include "supergames.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    "Super Games" ("Commodore Arcade 3 in 1")

    - This cart uses 4 16Kb banks mapped in at $8000-$BFFF.
    - assuming the i/o register is reset to 0, the cartridge starts up in bank 0
      and in 16k configuration.

    The control registers is at $DF00, and has the following meaning:

    bit   meaning
    ---   -------
     0    bank bit 0
     1    bank bit 1
     2    mode (0 = EXROM/GAME (bridged on the same wire - 16k config) 1 = cartridge disabled)
     3    write-protect-latch  (1 = no more changes are possible until the next hardware-reset )
    4-7   unused

*/

static int currbank = 0;
static int currmode = 0;
static int reglatched = 0;
static BYTE regval = 0;

static void supergames_io2_store(WORD addr, BYTE value)
{
    if (reglatched == 0) {
        regval = value;
        currbank = value & 3;
        currmode = ((value >> 2) & 1) ^ 1;
        reglatched = ((value >> 3) & 1);

        cart_romhbank_set_slotmain(currbank);
        cart_romlbank_set_slotmain(currbank);

        /* printf("value: %02x bank: %d mode: %d\n", value, currbank, currmode); */
        cart_set_port_exrom_slotmain(currmode);
        cart_set_port_game_slotmain(currmode);

        cart_port_config_changed_slotmain();
    }
}

static BYTE supergames_io2_peek(WORD addr)
{
    return regval;
}

static int supergames_dump(void)
{
    mon_out("Bank: %d (%s, %s)\n", currbank, currmode ? "enabled" : "disabled",
            reglatched ? "latched" : "not latched");
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t supergames_device = {
    CARTRIDGE_NAME_SUPER_GAMES,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    0,
    supergames_io2_store,
    NULL,
    supergames_io2_peek,
    supergames_dump,
    CARTRIDGE_SUPER_GAMES,
    0,
    0
};

static io_source_list_t *supergames_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_SUPER_GAMES, 1, 1, NULL, &supergames_device, CARTRIDGE_SUPER_GAMES
};

/* ---------------------------------------------------------------------*/

void supergames_config_init(void)
{
    /* cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME, CMODE_READ); */
    reglatched = 0;
    supergames_io2_store(0xdf00, 0);
}

void supergames_config_setup(BYTE *rawcart)
{
    memcpy(&roml_banks[0x0000], &rawcart[0x0000], 0x2000);
    memcpy(&romh_banks[0x0000], &rawcart[0x2000], 0x2000);
    memcpy(&roml_banks[0x2000], &rawcart[0x4000], 0x2000);
    memcpy(&romh_banks[0x2000], &rawcart[0x6000], 0x2000);
    memcpy(&roml_banks[0x4000], &rawcart[0x8000], 0x2000);
    memcpy(&romh_banks[0x4000], &rawcart[0xa000], 0x2000);
    memcpy(&roml_banks[0x6000], &rawcart[0xc000], 0x2000);
    memcpy(&romh_banks[0x6000], &rawcart[0xe000], 0x2000);
    /* cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME, CMODE_READ); */
    reglatched = 0;
    supergames_io2_store(0xdf00, 0);
}

/* ---------------------------------------------------------------------*/
static int supergames_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    supergames_list_item = io_source_register(&supergames_device);
    return 0;
}

int supergames_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return supergames_common_attach();
}

int supergames_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.start != 0x8000 || chip.size != 0x4000 || chip.bank > 3) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 14, &chip, fd)) {
            return -1;
        }
    }
    return supergames_common_attach();
}

void supergames_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(supergames_list_item);
    supergames_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTSUPERGAMES snapshot module format:

   type  | name        | version | description
   -------------------------------------------
   BYTE  | mode        |   0.2   | current mode
   BYTE  | regval      |   0.2   | register
   BYTE  | bank        |   0.0+  | current bank   
   BYTE  | reg latched |   0.1   | register latched flag
   ARRAY | ROML        |   0.0+  | 32768 BYTES of ROML data
   ARRAY | ROMH        |   0.0+  | 32768 BYTES of ROMH data
 */

static char snap_module_name[] = "CARTSUPERGAMES";
#define SNAP_MAJOR   0
#define SNAP_MINOR   2

int supergames_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)currmode) < 0
        || SMW_B(m, regval) < 0
        || SMW_B(m, (BYTE)currbank) < 0
        || SMW_B(m, (BYTE)reglatched) < 0
        || SMW_BA(m, roml_banks, 0x8000) < 0
        || SMW_BA(m, romh_banks, 0x8000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int supergames_snapshot_read_module(snapshot_t *s)
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

    /* new in 0.2 */
    if (SNAPVAL(vmajor, vminor, 0, 2)) {
        if (0
            || SMR_B_INT(m, &currmode) < 0
            || SMR_B(m, &regval) < 0) {
            goto fail;
        }
    } else {
        currmode = 0;
        regval = 0;
    }

    if (SMR_B_INT(m, &currbank) < 0) {
        goto fail;
    }

    /* new in 0.1 */
    if (SNAPVAL(vmajor, vminor, 0, 2)) {
        if (SMR_B_INT(m, &reglatched) < 0) {
            goto fail;
        }
    } else {
        reglatched = 0;
    }

    if (0
        || SMR_BA(m, roml_banks, 0x8000) < 0
        || SMR_BA(m, romh_banks, 0x8000) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return supergames_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
