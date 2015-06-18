/*
 * pagefox.c - Cartridge handling, Pagefox cart.
 *
 * Written by
 *  Groepaz/Hitmen <groepaz@gmx.net>
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
#include "comal80.h"
#include "lib.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    Pagefox Cartridge

    - 64K ROM (2 32K Eproms, mapped to $8000 and $A000 in 16K Game Mode)
    - 32K RAM (mapped to $8000 and $A000 in 16K Game Mode)

    The cart has 1 (write-only) bank control register which
    is located at $DE80 and mirrored throughout the $DE80-$DEFF
    range.

    Bit 0: unused/don't care
    Bit 1: Bank select: 0=upper, 1=lower (not correct ?!)
    Bit 2: chip select 0
    Bit 3: chip select 1
    Bit 4: cartridge enable/disable: 0=enable, 1=disable
    Bits 5-7: unused/don't care

    Chip select combinations of 0/1 are:
    00: Eprom "79"
    01: Eprom "ZS3"
    10: Ram
    11: empty space (reading returns VIC data)

    note: on the original hardware "disabling" the cartridge by setting bit 4 of
          the control register does NOT prevent write accesses to the cartridge
          ram!. so to actually disable the ram, it is suggested to write $ff to
          the register (see http://www.c64-wiki.de/index.php/Pagefox#Pagefox-Modul_als_RAM-Erweiterung)

    FIXME: this property is not emulated yet
*/

/* #define DEBUGPF */

#ifdef DEBUGPF
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#define PAGEFOX_RAMSIZE (32 * 1024)

static BYTE *pagefox_ram = NULL;
static int pagefox_enabled = 0;

static int currbank = 0;
static int chipselect = 0;
static int bankselect = 0;

static void pagefox_io1_store(WORD addr, BYTE value)
{
    bankselect = ((value >> 1) & 1);
    chipselect = (value >> 2) & 3;
    pagefox_enabled = ((value >> 4) & 1) ^ 1;
    currbank = (chipselect << 1) | bankselect;

    DBG(("PAGEFOX: bankselect: %d chipselect:%d enabled: %d bank: %d\n", bankselect, chipselect, pagefox_enabled, currbank));

    if (pagefox_enabled) {
        cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME, CMODE_READ);
    } else {
        cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM, CMODE_READ);
    }

    cart_romhbank_set_slotmain(currbank & 3);
    cart_romlbank_set_slotmain(currbank & 3);
}

static BYTE pagefox_io1_peek(WORD addr)
{
    return (bankselect << 1) | (chipselect << 2) | ((pagefox_enabled ^ 1) << 4);
}

static int pagefox_dump(void)
{
    mon_out("enabled:        %s\n", pagefox_enabled ? "yes" : "no");
    mon_out("bank select:    %d\n", bankselect);
    mon_out("chip select:    %d\n", chipselect);
    mon_out("effective bank: %d\n", currbank);
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t pagefox_device = {
    CARTRIDGE_NAME_PAGEFOX,
    IO_DETACH_CART,
    NULL,
    0xde80, 0xdeff, 0xff,
    0,
    pagefox_io1_store,
    NULL,
    pagefox_io1_peek,
    pagefox_dump,
    CARTRIDGE_PAGEFOX,
    0,
    0
};

static io_source_list_t *pagefox_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_PAGEFOX, 1, 1, &pagefox_device, NULL, CARTRIDGE_PAGEFOX
};

/* ---------------------------------------------------------------------*/

/* ROML read - mapped to 8000 in 8k,16k,ultimax */
BYTE pagefox_roml_read(WORD addr)
{
    if (chipselect == 2) {
        return pagefox_ram[0x0000 + (addr & 0x1fff) + (bankselect << 14)];
    }
    return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
}

/* ROML store - mapped to 8000 in ultimax mode */
void pagefox_roml_store(WORD addr, BYTE value)
{
    if (chipselect == 2) {
        pagefox_ram[0x0000 + (addr & 0x1fff) + (bankselect << 14)] = value;
    }
}

/* ROMH read - mapped to A000 in 16k, to E000 in ultimax */
BYTE pagefox_romh_read(WORD addr)
{
    if (chipselect == 2) {
        return pagefox_ram[0x2000 + (addr & 0x1fff) + (bankselect << 14)];
    }
    return romh_banks[(addr & 0x1fff) + (romh_bank << 13)];
}

void pagefox_romh_store(WORD addr, BYTE value)
{
    if (chipselect == 2) {
        pagefox_ram[0x2000 + (addr & 0x1fff) + (bankselect << 14)] = value;
    }
}

/* ---------------------------------------------------------------------*/

void pagefox_config_init(void)
{
    pagefox_io1_store(0xde80, 0x00);
}

void pagefox_config_setup(BYTE *rawcart)
{
    memcpy(&roml_banks[0x0000], &rawcart[0x0000], 0x2000);
    memcpy(&romh_banks[0x0000], &rawcart[0x2000], 0x2000);
    memcpy(&roml_banks[0x2000], &rawcart[0x4000], 0x2000);
    memcpy(&romh_banks[0x2000], &rawcart[0x6000], 0x2000);
    memcpy(&roml_banks[0x4000], &rawcart[0x8000], 0x2000);
    memcpy(&romh_banks[0x4000], &rawcart[0xa000], 0x2000);
    memcpy(&roml_banks[0x6000], &rawcart[0xc000], 0x2000);
    memcpy(&romh_banks[0x6000], &rawcart[0xe000], 0x2000);
    pagefox_io1_store(0xde80, 0x00);
}

/* ---------------------------------------------------------------------*/
static int pagefox_common_attach(void)
{
    if (c64export_add(&export_res) < 0) {
        return -1;
    }
    pagefox_list_item = io_source_register(&pagefox_device);
    return 0;
}

int pagefox_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    pagefox_ram = lib_malloc(PAGEFOX_RAMSIZE);
    return pagefox_common_attach();
}

int pagefox_crt_attach(FILE *fd, BYTE *rawcart)
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
    pagefox_ram = lib_malloc(PAGEFOX_RAMSIZE);
    return pagefox_common_attach();
}

void pagefox_detach(void)
{
    c64export_remove(&export_res);
    io_source_unregister(pagefox_list_item);
    pagefox_list_item = NULL;
    lib_free(pagefox_ram);
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTPAGEFOX"

int pagefox_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)currbank) < 0)
        || (SMW_BA(m, pagefox_ram, PAGEFOX_RAMSIZE) < 0)
        || (SMW_BA(m, roml_banks, 0x8000) < 0)
        || (SMW_BA(m, romh_banks, 0x8000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int pagefox_snapshot_read_module(snapshot_t *s)
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

    pagefox_ram = lib_malloc(PAGEFOX_RAMSIZE);

    if (0
        || (SMR_B_INT(m, &currbank) < 0)
        || (SMR_BA(m, pagefox_ram, PAGEFOX_RAMSIZE) < 0)
        || (SMR_BA(m, roml_banks, 0x8000) < 0)
        || (SMR_BA(m, romh_banks, 0x8000) < 0)) {
        snapshot_module_close(m);
        lib_free(pagefox_ram);
        return -1;
    }

    snapshot_module_close(m);

    return pagefox_common_attach();
}
