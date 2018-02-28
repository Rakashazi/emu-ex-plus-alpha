/*
 * final3.c - Cartridge handling, Final cart.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "final3.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
   The Final Cartridge 3

   - 4 16K ROM Banks at $8000/$a000 (=64K)

        Bank 0:  BASIC, Monitor, Disk-Turbo
        Bank 1:  Notepad, BASIC (Menu Bar)
        Bank 2:  Desktop, Freezer/Print
        Bank 3:  Freezer, Compression

   - the cartridges uses the entire io1 and io2 range

   - one register at $DFFF:

    7      Hide this register (1 = hidden)
    6      NMI line   (0 = low = active) *1)
    5      GAME line  (0 = low = active) *2)
    4      EXROM line (0 = low = active)
    2-3    unassigned (usually set to 0)
    0-1    number of bank to show at $8000

    1) if either the freezer button is pressed, or bit 6 is 0, then
       an NMI is generated

    2) if the freezer button is pressed, GAME is also forced low

    - the rest of io1/io2 contain a mirror of the last 2 pages of the
      currently selected rom bank (also at $dfff, contrary to what some
      other documents say)

    This implementation also supports the community developed "Final Cartridge III+"
    which has 16 ROM banks instead of the usual 4.

*/

static int fc3_reg_enabled = 1;
static BYTE regval = 0;
static int fc3_rom_banks = 4;

/* some prototypes are needed */
static BYTE final_v3_io1_read(WORD addr);
static BYTE final_v3_io2_read(WORD addr);
static void final_v3_io2_store(WORD addr, BYTE value);
static int final_v3_dump(void);

static io_source_t final3_io1_device = {
    CARTRIDGE_NAME_FINAL_III,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    1, /* read is always valid */
    NULL,
    final_v3_io1_read,
    final_v3_io1_read, /* peek */
    final_v3_dump,
    CARTRIDGE_FINAL_III,
    0,
    0
};

static io_source_t final3_io2_device = {
    CARTRIDGE_NAME_FINAL_III,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    1, /* read is always valid */
    final_v3_io2_store,
    final_v3_io2_read,
    final_v3_io2_read, /* peek */
    final_v3_dump,
    CARTRIDGE_FINAL_III,
    0,
    0
};

static io_source_list_t *final3_io1_list_item = NULL;
static io_source_list_t *final3_io2_list_item = NULL;

static const export_resource_t export_res_v3 = {
    CARTRIDGE_NAME_FINAL_III, 1, 1, &final3_io1_device, &final3_io2_device, CARTRIDGE_FINAL_III
};

/* ---------------------------------------------------------------------*/

BYTE final_v3_io1_read(WORD addr)
{
    return roml_banks[0x1e00 + (roml_bank << 13) + (addr & 0xff)];
}

BYTE final_v3_io2_read(WORD addr)
{
    return roml_banks[0x1f00 + (roml_bank << 13) + (addr & 0xff)];
}

void final_v3_io2_store(WORD addr, BYTE value)
{
    unsigned int flags;
    BYTE mode;

    regval = value;

    if ((fc3_reg_enabled) && ((addr & 0xff) == 0xff)) {
        flags = CMODE_WRITE;

        fc3_reg_enabled = ((value >> 7) & 1) ^ 1;
        if (value & 0x40) {
            flags |= CMODE_RELEASE_FREEZE;
        } else {
            flags |= CMODE_TRIGGER_FREEZE_NMI_ONLY;
        }
        mode = ((value >> 3) & 2) | (((value >> 5) & 1) ^ 1) | ((value & (fc3_rom_banks - 1)) << CMODE_BANK_SHIFT);
        cart_config_changed_slotmain(mode, mode, flags);
    }
}

/* FIXME: Add EXROM, GAME and NMI lines to the dump */
static int final_v3_dump(void)
{
    mon_out("Bank: %d of %d, register status: %s\n",
            regval & (fc3_rom_banks - 1), fc3_rom_banks,
            (regval & 0x80) ? "Hidden" : "Visible");
    return 0;
}

/* ---------------------------------------------------------------------*/

void final_v3_freeze(void)
{
    fc3_reg_enabled = 1;

    /* note: freeze does NOT force a specific bank like some other carts do */
    cart_config_changed_slotmain(2, (BYTE)(3 | (roml_bank << CMODE_BANK_SHIFT)), CMODE_READ);
}

void final_v3_config_init(void)
{
    fc3_reg_enabled = 1;
    cart_config_changed_slotmain(1, 1, CMODE_READ);
}

void final_v3_config_setup(BYTE *rawcart)
{
    int i;
    for (i = 0; i <= fc3_rom_banks; i++) {
        memcpy(&roml_banks[0x2000 * i], &rawcart[0x0000 + (0x4000 * i)], 0x2000);
        memcpy(&romh_banks[0x2000 * i], &rawcart[0x2000 + (0x4000 * i)], 0x2000);
    }
    cart_config_changed_slotmain(1, 1, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int final_v3_common_attach(void)
{
    if (export_add(&export_res_v3) < 0) {
        return -1;
    }

    final3_io1_list_item = io_source_register(&final3_io1_device);
    final3_io2_list_item = io_source_register(&final3_io2_device);

    return 0;
}

int final_v3_bin_attach(const char *filename, BYTE *rawcart)
{
    fc3_rom_banks = 4;
    if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        if (util_file_load(filename, rawcart, 0x10000 * 4, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            return -1;
        }
        fc3_rom_banks = 16;
    }

    return final_v3_common_attach();
}

int final_v3_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int i, banks = 0;

    for (i = 0; i <= 16; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > 16 || chip.size != 0x4000) {
            break;
        }

        if (crt_read_chip(rawcart, chip.bank << 14, &chip, fd)) {
            break;
        }
        ++banks;
    }

    if ((banks != 4) && (banks != 16)) {
        return -1;
    }
    fc3_rom_banks = banks;

    return final_v3_common_attach();
}

void final_v3_detach(void)
{
    export_remove(&export_res_v3);
    io_source_unregister(final3_io1_list_item);
    io_source_unregister(final3_io2_list_item);
    final3_io1_list_item = NULL;
    final3_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTFC3 snapshot module format:

   type  | name        | version | description
   -------------------------------------------
   BYTE  | ROML banks  |   1.2   | amount of ROML banks
   BYTE  | register    |   1.2   | register
   BYTE  | reg enabled |   0.0+  | register enabled flag
   ARRAY | ROML        |   1.1+  | 32768 or 131072 BYTES of ROML data
   ARRAY | ROMH        |   1.1+  | 32768 or 131072 BYTES of ROML data   

   Note: in 0.0 ROML and ROMH data was always 32768 BYTES.
 */

static char snap_module_name[] = "CARTFC3";
#define SNAP_MAJOR   1
#define SNAP_MINOR   2

int final_v3_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)fc3_rom_banks) < 0
        || SMW_B(m, regval) < 0
        || SMW_B(m, (BYTE)fc3_reg_enabled) < 0
        || SMW_BA(m, roml_banks, 0x2000 * fc3_rom_banks) < 0
        || SMW_BA(m, romh_banks, 0x2000 * fc3_rom_banks) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int final_v3_snapshot_read_module(snapshot_t *s)
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

    /* new in 1.2 */
    if (SNAPVAL(vmajor, vminor, 1, 2)) {
        if (0
            || SMR_B_INT(m, &fc3_rom_banks) < 0
            || SMR_B(m, &regval) < 0) {
            goto fail;
        }
    } else {
        fc3_rom_banks = 4;
        regval = 0;
    }

    if (SMR_B_INT(m, &fc3_reg_enabled) < 0) {
        goto fail;
    }

    /* changed in 1.1 */
    if (SNAPVAL(vmajor, vminor, 1, 1)) {
        if (0
            || SMR_BA(m, roml_banks, 0x2000 * fc3_rom_banks) < 0
            || SMR_BA(m, romh_banks, 0x2000 * fc3_rom_banks) < 0) {
            goto fail;
        }
    } else {
        if (0
            || SMR_BA(m, roml_banks, 0x2000 * 4) < 0
            || SMR_BA(m, romh_banks, 0x2000 * 4) < 0) {
            goto fail;
        }
    }

    snapshot_module_close(m);

    return final_v3_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
