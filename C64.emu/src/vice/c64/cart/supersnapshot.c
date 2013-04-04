/*
 * supersnapshot.c - Cartridge handling, Super Snapshot cart.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Nathan Huizinga <nathan.huizinga@chess.nl>
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
#include "cartio.h"
#include "cartridge.h"
#include "monitor.h"
#include "snapshot.h"
#include "supersnapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    Super Snapshot v5

    - 64K ROM,8*8K Banks (4*16k)
    - 32K RAM,4*8K Banks

    io1: (read)
        cart rom

    io1 (write)

    de00/de01:

    bit 4 - rom/ram bank bit1
    bit 3 - rom enable
    bit 2 - rom/ram bank bit0
    bit 1 - ram enable, EXROM
    bit 0 - release freeze, !GAME
*/

/* Super Snapshot configuration flags.  */
static BYTE romconfig = 9;
static int ram_bank = 0; /* Version 5 supports 4 - 8Kb RAM banks. */

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static BYTE supersnapshot_v5_io1_read(WORD addr);
static BYTE supersnapshot_v5_io1_peek(WORD addr);
static void supersnapshot_v5_io1_store(WORD addr, BYTE value);
static int supersnapshot_v5_dump(void);

static io_source_t ss5_device = {
    CARTRIDGE_NAME_SUPER_SNAPSHOT_V5,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    supersnapshot_v5_io1_store,
    supersnapshot_v5_io1_read,
    supersnapshot_v5_io1_peek,
    supersnapshot_v5_dump,
    CARTRIDGE_SUPER_SNAPSHOT_V5,
    0,
    0
};

static io_source_list_t *ss5_list_item = NULL;

static const c64export_resource_t export_res_v5 = {
    CARTRIDGE_NAME_SUPER_SNAPSHOT_V5, 1, 1, &ss5_device, NULL, CARTRIDGE_SUPER_SNAPSHOT_V5
};

/* ---------------------------------------------------------------------*/

static int currbank = 0;

static BYTE supersnapshot_v5_io1_read(WORD addr)
{
    ss5_device.io_source_valid = 1;

    switch (roml_bank) {
        case 0:
            return roml_banks[0x1e00 + (addr & 0xff)];
        case 1:
            return roml_banks[0x1e00 + (addr & 0xff) + 0x2000];
        case 2:
            return roml_banks[0x1e00 + (addr & 0xff) + 0x4000];
        case 3:
            return roml_banks[0x1e00 + (addr & 0xff) + 0x6000];
    }

    ss5_device.io_source_valid = 0;

    return 0;
}

static BYTE supersnapshot_v5_io1_peek(WORD addr)
{
    switch (roml_bank) {
        case 0:
            return roml_banks[0x1e00 + (addr & 0xff)];
        case 1:
            return roml_banks[0x1e00 + (addr & 0xff) + 0x2000];
        case 2:
            return roml_banks[0x1e00 + (addr & 0xff) + 0x4000];
        case 3:
            return roml_banks[0x1e00 + (addr & 0xff) + 0x6000];
    }
    return 0;
}

static void supersnapshot_v5_io1_store(WORD addr, BYTE value)
{
    if (((addr & 0xff) == 0) || ((addr & 0xff) == 1)) {
        int mode = CMODE_WRITE;

        if ((value & 1) == 1) {
            mode |= CMODE_RELEASE_FREEZE;
        }

        /* D0 = ~GAME */
        romconfig = ((value & 1) ^ 1);

        /* Calc RAM/ROM bank nr. */
        currbank = ((value >> 2) & 0x1) | ((value >> 3) & 0x2);

        /* ROM ~OE set? */
        if (((value >> 3) & 1) == 0) {
            romconfig |= (currbank << CMODE_BANK_SHIFT); /* Select ROM banknr. */
        }

        /* RAM ~OE set? */
        if (((value >> 1) & 1) == 0) {
            ram_bank = currbank;          /* Select RAM banknr. */
            mode |= CMODE_EXPORT_RAM;   /* export_ram */
            romconfig |= (1 << 1);      /* exrom */
        }
        cart_config_changed_slotmain(1, romconfig, mode);
    }
}

static int supersnapshot_v5_dump(void)
{
    mon_out("Bank: %d, ROM/RAM: %s\n",
            currbank,
            (export_ram) ? "RAM" : "ROM");
    return 0;
}

/* ---------------------------------------------------------------------*/

BYTE supersnapshot_v5_roml_read(WORD addr)
{
    if (export_ram) {
        return export_ram0[(addr & 0x1fff) + (ram_bank << 13)];
    }

    return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
}

void supersnapshot_v5_roml_store(WORD addr, BYTE value)
{
    if (export_ram) {
        export_ram0[(addr & 0x1fff) + (ram_bank << 13)] = value;
    }
}

void supersnapshot_v5_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
{
    switch (addr & 0xe000) {
        case 0x8000:
            if (export_ram) {
                *base = export_ram0 + (ram_bank << 13) - 0x8000;
            } else {
                *base = roml_banks + (roml_bank << 13) - 0x8000;
            }
            *start = 0x8000;
            *limit = 0x9ffd;
            return;
        default:
            break;
    }
    *base = NULL;
    *start = 0;
    *limit = 0;
}

/* ---------------------------------------------------------------------*/

void supersnapshot_v5_freeze(void)
{
    cart_config_changed_slotmain(3, 3, CMODE_READ | CMODE_EXPORT_RAM);
}

void supersnapshot_v5_config_init(void)
{
    supersnapshot_v5_io1_store((WORD)0xde00, 2);
}

void supersnapshot_v5_config_setup(BYTE *rawcart)
{
    memcpy(&roml_banks[0x0000], &rawcart[0x0000], 0x2000);
    memcpy(&romh_banks[0x0000], &rawcart[0x2000], 0x2000);
    memcpy(&roml_banks[0x2000], &rawcart[0x4000], 0x2000);
    memcpy(&romh_banks[0x2000], &rawcart[0x6000], 0x2000);
    memcpy(&roml_banks[0x4000], &rawcart[0x8000], 0x2000);
    memcpy(&romh_banks[0x4000], &rawcart[0xa000], 0x2000);
    memcpy(&roml_banks[0x6000], &rawcart[0xc000], 0x2000);
    memcpy(&romh_banks[0x6000], &rawcart[0xe000], 0x2000);
    supersnapshot_v5_io1_store((WORD)0xde00, 2);
}

/* ---------------------------------------------------------------------*/

static int supersnapshot_v5_common_attach(void)
{
    if (c64export_add(&export_res_v5) < 0) {
        return -1;
    }

    ss5_list_item = io_source_register(&ss5_device);

    return 0;
}

int supersnapshot_v5_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return supersnapshot_v5_common_attach();
}

int supersnapshot_v5_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i < 4; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }

        if (chip.start != 0x8000 || chip.size != 0x4000 || chip.bank > 3) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 14, &chip, fd)) {
            return -1;
        }
    }

    return supersnapshot_v5_common_attach();
}

void supersnapshot_v5_detach(void)
{
    c64export_remove(&export_res_v5);
    io_source_unregister(ss5_list_item);
    ss5_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTSS5"

int supersnapshot_v5_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, romconfig) < 0)
        || (SMW_B(m, (BYTE)ram_bank) < 0)
        || (SMW_BA(m, roml_banks, 0x8000) < 0)
        || (SMW_BA(m, romh_banks, 0x8000) < 0)
        || (SMW_BA(m, export_ram0, 0x8000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int supersnapshot_v5_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B(m, &romconfig) < 0)
        || (SMR_B_INT(m, &ram_bank) < 0)
        || (SMR_BA(m, roml_banks, 0x8000) < 0)
        || (SMR_BA(m, romh_banks, 0x8000) < 0)
        || (SMR_BA(m, export_ram0, 0x8000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return supersnapshot_v5_common_attach();
}
