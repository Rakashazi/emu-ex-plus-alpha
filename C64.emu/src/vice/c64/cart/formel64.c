/*
 * formel64.c - Cartridge handling, Formel64 cart.
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

/*
the following is a quick overview of how the cartridge works, as its a bit
unusual and different from most other cartridges:

- 27256 EPROM (32k)
- 7430 TTL
- MC6821
- 1 button (reset)

rom bank 0x00 - 0x03 (0x04) 8192* 4 32k mapped to e000
MC6821 registers mapped to io2 at dfc0..dfc4

- press reset and hold delete to get the main menu
- press reset and hold control to skip cbm80 check (dont start additional cartridge)

- press RESTORE, then...
  - left arrow, return  show disk directory
  - delete, 1           load"*" from disk, run
  - f1/f2               ? (disk stuff?)
  - f3/f4               ? (disk stuff?)
  - f5/f6, q            enter monitor
  - f7/f8, 2            show drive error channel
  - control, cursor     back to basic

  - type "help" in basic to get a list of available commands

*** MC6821 Port usage

Port A (parallel cable to floppy drive):
dfc0 port a ddr
dfc1 port a (in/out)

Port B (controls banking)
dfc2 port b ddr ($7f)
dfc3 port b (out)

bit3    1 = rom at $e000 enabled, 0 = cartridge disabled
bit1-2  rom bank number
bit0    ?

*/

/* define for debug messages */
/* #define FORMEL64_DEBUG */

/* #define LOG_PORTA */
/* #define LOG_PORTB */

#ifdef FORMEL64_DEBUG
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#include "vice.h"

#include <stdio.h>
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "c64parallel.h"
#include "cartio.h"
#include "cartridge.h"
#include "drive.h"
#include "export.h"
#include "maincpu.h"
#include "machine.h"
#include "mc6821core.h"
#include "formel64.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static void formel64_io2_store(WORD addr, BYTE value);
static BYTE formel64_io2_read(WORD addr);
static BYTE formel64_io2_peek(WORD addr);

static io_source_t formel64_io2_device = {
    CARTRIDGE_NAME_FORMEL64,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    1, /* read is always valid */
    formel64_io2_store,
    formel64_io2_read,
    formel64_io2_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_FORMEL64,
    0,
    0
};

static io_source_list_t *formel64_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_FORMEL64, 1, 1, NULL, &formel64_io2_device, CARTRIDGE_FORMEL64
};

/* ---------------------------------------------------------------------*/

static int f64_enabled = 1;

/***************************************************************************
    connection to the mc6821 emulation
 ***************************************************************************/

static mc6821_state my6821;

#ifdef LOG_PORTA
static void f64_print_pa(BYTE data)
{
    DBG(("6821 PA %02x ", data));
    DBG(("[??? %02x] ", data & 0xff));
}
#endif

#ifdef LOG_PORTB
static void f64_print_pb(BYTE data)
{
    BYTE page;

    page = ((data >> 1) & 3);

    DBG(("6821 PB %02x", data));
    DBG(("[ROM BANK %02x] ", page));
}
#endif

/* parallel cable data */
static void f64_set_pa(mc6821_state *ctx)
{
#ifdef LOG_PORTA
    DBG(("Formel64: to drive     "));
    f64_print_pa(ctx->dataA);
    DBG(("\n"));
#endif
    parallel_cable_cpu_pulse(DRIVE_PC_FORMEL64);
    parallel_cable_cpu_write(DRIVE_PC_FORMEL64, (BYTE)ctx->dataA);
}

static BYTE f64_get_pa(mc6821_state *ctx)
{
    BYTE data = 0xff;

    parallel_cable_cpu_write(DRIVE_PC_FORMEL64, 0xff);
    data = parallel_cable_cpu_read(DRIVE_PC_FORMEL64, data);

#ifdef LOG_PORTA
    DBG(("Formel64: from drive   "));
    f64_print_pa(data);
    DBG(("\n"));
#endif
    return data;
}

/* banking */
static void f64_set_pb(mc6821_state *ctx)
{
    romh_bank = (ctx->dataB >> 1) & 3;
    f64_enabled = (ctx->dataB >> 3);

#ifdef LOG_PORTB
    DBG(("Formel64: banking      "));
    f64_print_pb(ctx->dataB);
    DBG(("\n"));
#endif
}

static BYTE f64_get_pb(mc6821_state *ctx)
{
    BYTE data = 0;
    return data;
}

/****************************************************************************
*
****************************************************************************/

static BYTE formel64_io2_read(WORD addr)
{
    int port, reg;

    port = (addr >> 1) & 1; /* rs1 */
    reg = (addr >> 0) & 1;  /* rs0 */

    /* DBG(("Formel64: read from io2 %04x port %02x reg %02x\n",addr,port,reg)); */

    return mc6821core_read(&my6821, port /* rs1 */, reg /* rs0 */);
}

static BYTE formel64_io2_peek(WORD addr)
{
    int port, reg;

    port = (addr >> 1) & 1; /* rs1 */
    reg = (addr >> 0) & 1;  /* rs0 */

    return mc6821core_peek(&my6821, port /* rs1 */, reg /* rs0 */);
}

static void formel64_io2_store(WORD addr, BYTE value)
{
    int port, reg;

    port = (addr >> 1) & 1; /* rs1 */
    reg = (addr >> 0) & 1;  /* rs0 */

    mc6821core_store(&my6821, port /* rs1 */, reg /* rs0 */, value);
}

/****************************************************************************
*
****************************************************************************/
/* ---------------------------------------------------------------------*/

BYTE formel64_romh_read(WORD addr)
{
    if (f64_enabled) {
        return romh_banks[(addr & 0x1fff) + (romh_bank << 13)];
    }
    return mem_read_without_ultimax(addr);
}

BYTE formel64_romh_read_hirom(WORD addr)
{
    if (f64_enabled) {
        return romh_banks[(addr & 0x1fff) + (romh_bank << 13)];
    }
    return mem_read_without_ultimax(addr);
}

int formel64_romh_phi1_read(WORD addr, BYTE *value)
{
    return CART_READ_C64MEM;
}

int formel64_romh_phi2_read(WORD addr, BYTE *value)
{
    return formel64_romh_phi1_read(addr, value);
}

int formel64_peek_mem(export_t *export, WORD addr, BYTE *value)
{
    if (addr >= 0xe000) {
        *value = romh_banks[(addr & 0x1fff) + (romh_bank << 13)];
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH;
}

/****************************************************************************/

/* ultimax, rom bank 2 */

void formel64_config_init(void)
{
    DBG(("Formel64: init\n"));

    f64_enabled = 1;   /* PB3 */

    my6821.set_pa = f64_set_pa;
    my6821.set_pb = f64_set_pb;
    my6821.get_pa = f64_get_pa;
    my6821.get_pb = f64_get_pb;

    romh_bank = 2;
    cart_config_changed_slotmain(CMODE_RAM, (BYTE)(CMODE_ULTIMAX | (romh_bank << CMODE_BANK_SHIFT)), CMODE_READ);
}

void formel64_reset(void)
{
    DBG(("Formel64: reset\n"));
    mc6821core_reset(&my6821);
}

void formel64_config_setup(BYTE *rawcart)
{
    memcpy(romh_banks, rawcart, 0x8000);
}

static int formel64_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    formel64_io2_list_item = io_source_register(&formel64_io2_device);
    return 0;
}

int formel64_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x8000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return formel64_common_attach();
}

/*
    load CRT
*/

int formel64_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i <= 3; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }

        if (chip.bank > 15 || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    return formel64_common_attach();
}

void formel64_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(formel64_io2_list_item);
    formel64_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTF64 snapshot module format:

   type  | name      | description
   -------------------------------
   BYTE  | enabled   | cartridge enabled flag
   ARRAY | ROMH      | 32768 BYTES of ROMH data
   BYTE  | CTRL A    | control register A
   BYTE  | CTRL B    | control register B
   BYTE  | DATA A    | data register A
   BYTE  | DATA B    | data register B
   BYTE  | DIR A     | direction register A
   BYTE  | DIR B     | direction register B
   BYTE  | CA2       | CA2 line flag
   BYTE  | CA2 state | CA2 line state
   BYTE  | CB2       | CB2 line flag
   BYTE  | CB2 state | CB2 line state
 */

static char snap_module_name[] = "CARTF64";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int formel64_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)f64_enabled) < 0)
        || (SMW_BA(m, romh_banks, 0x8000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    if (mc6821core_snapshot_write_data(&my6821, m) < 0) {
        return -1;
    }

    return snapshot_module_close(m);
}

int formel64_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B_INT(m, &f64_enabled) < 0)
        || (SMR_BA(m, romh_banks, 0x8000) < 0)) {
        goto fail;
    }

    if (mc6821core_snapshot_read_data(&my6821, m) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    parallel_cable_cpu_undump(DRIVE_PC_FORMEL64, (BYTE)my6821.dataA);

    return formel64_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
