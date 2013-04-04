/*
 * atomicpower.c - Cartridge handling, Atomic Power cart.
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
#include <string.h>

#include "atomicpower.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64export.h"
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/* #define DEBUGAP */

#ifdef DEBUGAP
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    Atomic Power/Nordic Power

    - the hardware is very similar to Action Replay 5, with one exception

    32K rom, 4*8k pages
    8K ram

    io1 (writes)

    7    extra ROM bank selector (A15) (unused)
    6    1 = resets FREEZE-mode (turns back to normal mode)
    5    1 = enable RAM at ROML ($8000-$9FFF) &
            I/O2 ($DF00-$DFFF = $9F00-$9FFF)
    4    ROM bank selector high (A14)
    3    ROM bank selector low  (A13)
    2    1 = disable cartridge (turn off $DE00)
    1    1 = /EXROM high
    0    1 = /GAME low

    different to original AR:

    if bit 5 (RAM enable) is 1,
       bit 0,1 (exrom/game) is == 2 (cart off),
       bit 2,6,7 (cart disable, freeze clear) are 0,

    then Cart ROM (Bank 0..3) is mapped at 8000-9fff,
     and Cart RAM (Bank 0) is mapped at A000-bfff
     and Cart RAM (Bank 0) is is enabled in io2 area
     using 16K Game config

    io2 (r/w)
        cart RAM (if enabled) or cart ROM
*/

/* Atomic Power RAM hack. */
static int export_ram_at_a000 = 0;
static int ap_active;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static void atomicpower_io1_store(WORD addr, BYTE value);
static BYTE atomicpower_io2_read(WORD addr);
static void atomicpower_io2_store(WORD addr, BYTE value);

static io_source_t atomicpower_io1_device = {
    CARTRIDGE_NAME_ATOMIC_POWER,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    atomicpower_io1_store,
    NULL,
    NULL, /* TODO: peek */
    NULL, /* TODO: dump */
    CARTRIDGE_ATOMIC_POWER,
    0,
    0
};

static io_source_t atomicpower_io2_device = {
    CARTRIDGE_NAME_ATOMIC_POWER,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    0,
    atomicpower_io2_store,
    atomicpower_io2_read,
    NULL, /* TODO: peek */
    NULL, /* TODO: dump */
    CARTRIDGE_ATOMIC_POWER,
    0,
    0
};

static io_source_list_t *atomicpower_io1_list_item = NULL;
static io_source_list_t *atomicpower_io2_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_ATOMIC_POWER, 1, 1, &atomicpower_io1_device, &atomicpower_io2_device, CARTRIDGE_ATOMIC_POWER
};

/* ---------------------------------------------------------------------*/
static void atomicpower_io1_store(WORD addr, BYTE value)
{
    int flags = CMODE_WRITE, bank, mode;
    if (ap_active) {
        bank = ((value >> 3) & 3);
        mode = (value & 3);
        DBG(("io1 w %02x mode %d bank %d (np special: %s)\n", value, mode, bank, ((value & 0xe7) == 0x22) ? "yes" : "no"));

        if ((value & 0xe7) == 0x22) {
            mode = 1; /* 16k Game */
            export_ram_at_a000 = 1; /* RAM at a000 enabled */
        } else {
            /* Action Replay 5 compatible values */
            export_ram_at_a000 = 0;
            if (value & 0x40) {
                flags |= CMODE_RELEASE_FREEZE;
            }
            if (value & 0x20) {
                flags |= CMODE_EXPORT_RAM;
            }
        }
        if (value & 4) {
            ap_active = 0;
        }

        cart_config_changed_slotmain((BYTE) 2, (BYTE) (mode | (bank << CMODE_BANK_SHIFT)), flags);
    }
}

static BYTE atomicpower_io2_read(WORD addr)
{
    atomicpower_io2_device.io_source_valid = 0;

    if (!ap_active) {
        return 0;
    }

    atomicpower_io2_device.io_source_valid = 1;

    if (export_ram || export_ram_at_a000) {
        return export_ram0[0x1f00 + (addr & 0xff)];
    }

    addr |= 0xdf00;

    switch (roml_bank) {
        case 0:
            return roml_banks[addr & 0x1fff];
        case 1:
            return roml_banks[(addr & 0x1fff) + 0x2000];
        case 2:
            return roml_banks[(addr & 0x1fff) + 0x4000];
        case 3:
            return roml_banks[(addr & 0x1fff) + 0x6000];
    }
    atomicpower_io2_device.io_source_valid = 0;
    return 0;
}

static void atomicpower_io2_store(WORD addr, BYTE value)
{
    if (ap_active) {
        if (export_ram || export_ram_at_a000) {
            export_ram0[0x1f00 + (addr & 0xff)] = value;
        }
    }
}

/* ---------------------------------------------------------------------*/

BYTE atomicpower_roml_read(WORD addr)
{
    if (export_ram) {
        return export_ram0[addr & 0x1fff];
    }

    return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
}

void atomicpower_roml_store(WORD addr, BYTE value)
{
    if (export_ram) {
        export_ram0[addr & 0x1fff] = value;
    }
}

BYTE atomicpower_romh_read(WORD addr)
{
    if (export_ram_at_a000) {
        return export_ram0[addr & 0x1fff];
    }
    return romh_banks[(addr & 0x1fff) + (romh_bank << 13)];
}

void atomicpower_romh_store(WORD addr, BYTE value)
{
    if (export_ram_at_a000) {
        export_ram0[addr & 0x1fff] = value;
    }
}

void atomicpower_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
{
    switch (addr & 0xe000) {
        case 0x8000:
            if (export_ram) {
                *base = export_ram0;
            } else {
                *base = roml_banks + (roml_bank << 13) - 0x8000;
            }
            *start = 0x8000;
            *limit = 0x9ffd;
            return;
        case 0xa000:
            if (export_ram_at_a000) {
                *base = export_ram0;
            } else {
                *base = romh_banks + (romh_bank << 13) - 0xa000;
            }
            *start = 0xa000;
            *limit = 0xbffd;
            return;
        case 0xe000:
            if (export_ram_at_a000) {
                *base = export_ram0;
            } else {
                *base = romh_banks + (romh_bank << 13) - 0xe000;
            }
            *start = 0xe000;
            *limit = 0xfffd;
            return;
        default:
            break;
    }
    *base = NULL;
    *start = 0;
    *limit = 0;
}

/* ---------------------------------------------------------------------*/

void atomicpower_freeze(void)
{
    ap_active = 1;
    cart_config_changed_slotmain(3, 3, CMODE_READ | CMODE_EXPORT_RAM);
}

void atomicpower_config_init(void)
{
    ap_active = 1;
    export_ram_at_a000 = 0;
    cart_config_changed_slotmain(0, 0, CMODE_READ);
}

void atomicpower_reset(void)
{
    ap_active = 1;
}

void atomicpower_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x8000);
    memcpy(romh_banks, rawcart, 0x8000);
    cart_config_changed_slotmain(0, 0, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int atomicpower_common_attach(void)
{
    if (c64export_add(&export_res) < 0) {
        return -1;
    }

    atomicpower_io1_list_item = io_source_register(&atomicpower_io1_device);
    atomicpower_io2_list_item = io_source_register(&atomicpower_io2_device);

    return 0;
}

int atomicpower_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x8000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return atomicpower_common_attach();
}

int atomicpower_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i <= 3; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }

        if (chip.bank > 3 || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    return atomicpower_common_attach();
}

void atomicpower_detach(void)
{
    c64export_remove(&export_res);
    io_source_unregister(atomicpower_io1_list_item);
    io_source_unregister(atomicpower_io2_list_item);
    atomicpower_io1_list_item = NULL;
    atomicpower_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTAP"

int atomicpower_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)ap_active) < 0)
        || (SMW_B(m, (BYTE)export_ram_at_a000) < 0)
        || (SMW_BA(m, roml_banks, 0x8000) < 0)
        || (SMW_BA(m, export_ram0, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int atomicpower_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B_INT(m, &ap_active) < 0)
        || (SMR_B_INT(m, &export_ram_at_a000) < 0)
        || (SMR_BA(m, roml_banks, 0x8000) < 0)
        || (SMR_BA(m, export_ram0, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    memcpy(romh_banks, roml_banks, 0x8000);
    return atomicpower_common_attach();
}
