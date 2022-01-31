/*
 * drean.c - Cartridge handling, Drean cart. (H.E.R.O., Le Mans...)
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

/* #define DREAN_DEBUG */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "drean.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

#ifdef DREAN_DEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    Argentinian (Drean) "H.E.R.O." Cartridge

    - ROM is always mapped in at $8000-$9FFF (8k game).

    - 1 register at io2 / dfff:

    bit 0-1   bank number
    bit 5     exrom (1 = cart disabled)

    note: most of the above was guessed from the only existing ROM dump. maybe
          more ROMs that used the same hardware show up, also the code is prepared
          to easily add cartridges of the same type that use more banks.
*/

#define MAXBANKS 4

static uint8_t regval = 0;
static uint8_t bankmask = 0x03;

static void drean_io2_store(uint16_t addr, uint8_t value)
{
    regval = value & (0x20 | bankmask);
    cart_romlbank_set_slotmain(value & bankmask);
    cart_set_port_game_slotmain(0);
    if (value & 0x20) {
        /* turn off cart ROM */
        cart_set_port_exrom_slotmain(0);
    } else {
        cart_set_port_exrom_slotmain(1);
    }
    cart_port_config_changed_slotmain();
    DBG(("DREAN: Addr: %04x Value: %02x Reg: %02x (Bank: %d of %d, %s)\n", addr, value,
         regval, (regval & bankmask), bankmask + 1, (regval & 0x20) ? "disabled" : "enabled"));
}

static uint8_t drean_io2_peek(uint16_t addr)
{
    return regval;
}

static int drean_dump(void)
{
    mon_out("Reg: %02x (Bank: %d of %d, %s)\n", regval, (regval & bankmask), bankmask + 1, (regval & 0x80) ? "disabled" : "enabled");
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t drean_device = {
    CARTRIDGE_NAME_DREAN,       /* name of the device */
    IO_DETACH_CART,            /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,     /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,      /* range for the device, address is ignored, reg:$df00, mirrors:$df01-$dfff */
    0,                         /* read is never valid, reg is write only */
    drean_io2_store,            /* store function */
    NULL,                      /* NO poke function */
    NULL,                      /* read function */
    drean_io2_peek,             /* peek function */
    drean_dump,                 /* device state information dump function */
    CARTRIDGE_DREAN,            /* cartridge ID */
    IO_PRIO_NORMAL,            /* normal priority, device read needs to be checked for collisions */
    0                          /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *drean_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_DREAN, 0, 1, NULL, &drean_device, CARTRIDGE_DREAN
};

/* ---------------------------------------------------------------------*/

void drean_config_init(void)
{
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    drean_io2_store((uint16_t)0xdfff, 0);
}

void drean_config_setup(uint8_t *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000 * MAXBANKS);
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int drean_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    drean_list_item = io_source_register(&drean_device);
    return 0;
}

int drean_bin_attach(const char *filename, uint8_t *rawcart)
{
    bankmask = 0x03;
    if (util_file_load(filename, rawcart, 0x8000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return drean_common_attach();
}

int drean_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int lastbank = 0;

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }
        if ((chip.bank >= MAXBANKS) || ((chip.start != 0x8000) && (chip.start != 0xa000)) || (chip.size != 0x2000)) {
            return -1;
        }
        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
        if (chip.bank > lastbank) {
            lastbank = chip.bank;
        }
    }
    if (lastbank >= 4) {
        /* more than 4 banks does not work */
        return -1;
    } else {
        /* max 4 banks */
        bankmask = 0x03;
    }
    return drean_common_attach();
}

void drean_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(drean_list_item);
    drean_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   1
#define SNAP_MODULE_NAME  "CARTDREAN"

int drean_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (uint8_t)regval) < 0)
        || (SMW_B(m, (uint8_t)bankmask) < 0)
        || (SMW_BA(m, roml_banks, 0x2000 * MAXBANKS) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int drean_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
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
        || (SMR_B(m, &regval) < 0)
        || (SMR_B(m, &bankmask) < 0)
        || (SMR_BA(m, roml_banks, 0x2000 * MAXBANKS) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (drean_common_attach() == -1) {
        return -1;
    }
    drean_io2_store(0xdfff, regval);
    return 0;
}
