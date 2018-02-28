/*
 * magicformel.c - Cartridge handling, Magic Formel cart.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

/*
the following is a quick overview of how the cartridge works, as its a bit
unusual and different from most other cartridges:

Magic Formel
- 64K ROM
- 8K RAM

rom bank 0x00 - 0x07 (0x08) 8192* 8 64k
ram bank 0x00 - 0x1f (0x20)  256*32  8k

Magic Formel 2
- 64K ROM
- 32K extra ROM
- 8K RAM

rom bank 0x00 - 0x0f (0x10) 8192*16 128k
ram bank 0x00 - 0x1f (0x20)  256*32   8k

ram is mapped to $deXX (one page in io1 space)
rom is mapped to $e000

*** Register Details

Writing anywhere to the IO2 area adresses a MC6521 (2 8bit i/o ports). However
its registers are not simply memory mapped, instead A0-A5 go to D0-D5, D1 to D7
and A6-A7 to RS1/RS0.

addr    dfXX    RRDD DDDD

  R -rs1/rs0 of MC6821
  D -d0-d5 of MC6821

data      .X    .... ..D.

  D - d7 of MC6821 (Controls Mapping)

*** MC6821 Registers

RS1  RS0 (CRA2 CRB2 = control register bit 2)

0    0    1    .     Data A
0    0    0    .     Data Direction A
0    1    .    .     Control Register A
1    0    .    1     Data B
1    0    .    0     Data Direction B
1    1    .    .     Control Register B

*** MC6821 Port usage

PA (Output Data)

A0,A1,A2       - ROM Bank
A3             - extra ROM enable (0) / disable (1)
A4             - RAM enable (0) / disable (1)
A5-A7 unused

PB (Output Data)

B3,B2,B0,B1,B4 - RAM Bank
B5-B6 (unused)
B7             - enable ROM at $E000 (?)

CB2            - enable Cartridge (?)

*/

/* permanently enable RAM in io1 */
/* #define DEBUG_IO1_NO_DISABLE  */

/* define for debug messages */
/* #define MAGICFORMEL_DEBUG */

/* #define LOG_BANKS */
/* #define LOG_PORTS */

#ifdef MAGICFORMEL_DEBUG
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
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "maincpu.h"
#include "machine.h"
#include "magicformel.h"
#include "mc6821core.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static void magicformel_io1_store(WORD addr, BYTE value);
static BYTE magicformel_io1_read(WORD addr);
static BYTE magicformel_io1_peek(WORD addr);
static void magicformel_io2_store(WORD addr, BYTE value);
static BYTE magicformel_io2_read(WORD addr);
static BYTE magicformel_io2_peek(WORD addr);

static io_source_t magicformel_io1_device = {
    CARTRIDGE_NAME_MAGIC_FORMEL,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    magicformel_io1_store,
    magicformel_io1_read,
    magicformel_io1_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_MAGIC_FORMEL,
    0,
    0
};

static io_source_t magicformel_io2_device = {
    CARTRIDGE_NAME_MAGIC_FORMEL,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    1, /* read is always valid */
    magicformel_io2_store,
    magicformel_io2_read,
    magicformel_io2_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_MAGIC_FORMEL,
    0,
    0
};

static io_source_list_t *magicformel_io1_list_item = NULL;
static io_source_list_t *magicformel_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_MAGIC_FORMEL, 1, 1, &magicformel_io1_device, &magicformel_io2_device, CARTRIDGE_MAGIC_FORMEL
};

/* ---------------------------------------------------------------------*/

static int ram_page = 0;
static int io1_enabled = 0;     /* PA4 */
static int kernal_enabled = 0;  /* PBZ */
static int freeze_enabled = 0;
static int export_game = 1;
static int hwversion = 0;

static mc6821_state my6821;

/****************************************************************************
*
****************************************************************************/
#ifdef LOG_BANKS
static int logbanks[0x10];

static void log_bank(int bank)
{
    int i;

    logbanks[bank] = 0xff;
    DBG(("["));

    for (i = 0; i < 0x10; i++) {
        if (logbanks[i] == 0xff) {
            DBG(("*"));
        } else {
            DBG(("."));
        }
    }
    DBG(("]\n"));
}
#endif

/****************************************************************************
*
****************************************************************************/

/*
    magic formel switches GAME depending on ADDR ($E000-$FFFF)
*/

static void change_config(void)
{
    if (kernal_enabled || freeze_enabled) {
        cart_config_changed_slotmain(2, (BYTE)(3 | (romh_bank << CMODE_BANK_SHIFT)), CMODE_READ | CMODE_PHI2_RAM);
    } else {
        cart_config_changed_slotmain(2, (BYTE)(2 | (romh_bank << CMODE_BANK_SHIFT)), CMODE_READ | CMODE_PHI2_RAM);
    }
}

static void freeze_flipflop(int reset, int freeze, int clear)
{
    if (reset) {
        freeze_enabled = 1;
    } else {
        if (clear) {
            freeze_enabled = 0;
        } else if (freeze) {
            freeze_enabled = freeze;
        }
    }
/* DBG(("freeze_flipflop reset %d freeze %d clear %d -> freeze_enabled %d kernal_enabled %d\n",reset,freeze,clear,freeze_enabled,kernal_enabled)); */
}

/***************************************************************************
    connection to the mc6821 emulation
 ***************************************************************************/

#ifdef LOG_PORTS
static void mf_print_pa(BYTE data)
{
    /*
        PA (Output Data)

        A0,A1,A2       - ROM Bank
        A3             - extra ROM enable/disable (?)
        A4             - RAM enable/disable (?)
        A5-A7 unused
    */
    DBG(("6821 PA "));
    DBG(("[ROM BANK %02x] ", data & 0x07));
    DBG(("[EXTRA ROM %d] ", (data >> 3) & 1));
    DBG(("[RAM ENABLE %d] ", (data >> 4) & 1));
    DBG(("[UNUSED %02x] ", (data) & 0xe0));
}

static void mf_print_pb(BYTE data)
{
    BYTE page;

    /*
        PB (Output Data)

        B3,B2,B0,B1,B4 - RAM Bank
        B5-B6 (unused)
        B7             - rom enable (?)
    */
    /* ram_page = data & 0x1f; */
    page = (((data >> 3) & 1) << 0) |
           (((data >> 2) & 1) << 1) |
           (((data >> 0) & 1) << 2) |
           (((data >> 1) & 1) << 3) |
           (((data >> 4) & 1) << 4);

    DBG(("6821 PB "));
    DBG(("[RAM BANK %02x] ", page));
    DBG(("[ROM ENABLE %d] ", (data >> 7) & 1));
    DBG(("[UNUSED %02x] ", (data) & 0x60));
}
#endif

static void mf_set_pa(mc6821_state *ctx)
{
    BYTE data = ctx->dataA;
#ifdef LOG_PORTS
    mf_print_pa(ctx->dataA);
    mf_print_pb(ctx->dataB);
    DBG(("\n"));
#endif
    /*
    PA (Output Data)

    A0,A1,A2       - ROM Bank
    A3             - extra ROM enable/disable (?)
    A4             - RAM enable/disable (?)
    A5-A7 unused
    */
    if (hwversion == 0) {
        romh_bank = data & 0x07;
    } else {
        romh_bank = data & 0x0f;
    }
    freeze_flipflop(0 /* reset */, 0 /* freeze */, ctx->CB2);
    change_config();

#ifdef LOG_BANKS
    log_bank(romh_bank);
#endif

    /* DBG(("ROM Bank %02x\n",data&0x0f)); */
    if (data & 0x10) {
        /* DBG(("  %04x PA DATA %02x [IO1 RAM DISABLE?]\n",addr,data)); */
        io1_enabled = 0;
    } else {
        /* DBG(("  %04x PA DATA %02x [IO1 RAM ENABLE?]\n",addr,data)); */
        io1_enabled = 1;
    }
}

static void mf_set_pb(mc6821_state *ctx)
{
    BYTE data = ctx->dataB;
#ifdef LOG_PORTS
    mf_print_pa(ctx->dataA);
    mf_print_pb(ctx->dataB);
    DBG(("\n"));
#endif
    /*
    PB (Output Data)

    B3,B2,B0,B1,B4 - RAM Bank
    B5-B6 (unused)
    B7             - rom enable (?)
    */
    /* ram_page = data & 0x1f; */
    ram_page =
        (((data >> 3) & 1) << 0) |
        (((data >> 2) & 1) << 1) |
        (((data >> 0) & 1) << 2) |
        (((data >> 1) & 1) << 3) |
        (((data >> 4) & 1) << 4);

    /* DBG(("RAM Bank %02x\n",data&0x0f)); */

    if (data & 0x80) {
        /* DBG(("  %04x PB DATA %02x [ROM ENABLE?] [RAM BANK %02x]\n",0xdf00,data,ram_page)); */
        kernal_enabled = 1;
    } else {
        /* DBG(("  %04x PB DATA %02x [ROM DISABLE?] [RAM BANK %02x]\n",0xdf00,data,ram_page)); */
        kernal_enabled = 0;
    }

    freeze_flipflop(0 /* reset */, 0 /* freeze */, ctx->CB2);
    change_config();
}

/*
static int mf_get_pa(mc6821_state *ctx)
{
}

static int mf_get_pb(mc6821_state *ctx)
{
}

static void mf_set_ca2(mc6821_state *ctx)
{
}
*/

static void mf_set_cb2(mc6821_state *ctx)
{
    freeze_flipflop(0 /* reset */, 0 /* freeze */, ctx->CB2);
    change_config();
}

/****************************************************************************
*
****************************************************************************/

static BYTE magicformel_io1_read(WORD addr)
{
#ifdef DEBUG_IO1_NO_DISABLE
    if (1) {
#else
    if (io1_enabled) {
#endif
        magicformel_io1_device.io_source_valid = 1;
        return export_ram0[(ram_page << 8) + (addr & 0xff)];
    } else {
        magicformel_io1_device.io_source_valid = 0;
        DBG(("MF: read from disabled io1\n"));
    }
    return 0;
}

static BYTE magicformel_io1_peek(WORD addr)
{
    return export_ram0[(ram_page << 8) + (addr & 0xff)];
}

static void magicformel_io1_store(WORD addr, BYTE value)
{
#ifdef DEBUG_IO1_NO_DISABLE
    if (1) {
#else
    if (io1_enabled) {
#endif
        export_ram0[(ram_page << 8) + (addr & 0xff)] = value;
    } else {
        DBG(("MF: write to disabled io1\n"));
    }
}

/*
    a0..a5 go to d0..d5
    a6..a7 go to rs1..rs0
    d1 goes to d7
*/

static BYTE magicformel_io2_read(WORD addr)
{
    int port, reg;

    port = (addr >> 7) & 1; /* rs1 */
    reg = (addr >> 6) & 1;  /* rs0 */

    DBG(("MF: read from io2 %04x data %02x port %02x reg %02x\n", addr, addr & 0x3f, port, reg));

    return mc6821core_read(&my6821, port /* rs1 */, reg /* rs0 */);
}

static BYTE magicformel_io2_peek(WORD addr)
{
    int port, reg;

    port = (addr >> 7) & 1; /* rs1 */
    reg = (addr >> 6) & 1;  /* rs0 */

    return mc6821core_peek(&my6821, port /* rs1 */, reg /* rs0 */);
}

static void magicformel_io2_store(WORD addr, BYTE value)
{
    int port;
    WORD reg;
    BYTE data;

    data = (addr & 0x3f) | ((value & 2) << 6); /* d0..d5 d7 */
    port = (addr >> 7) & 1; /* rs1 */
    reg = (addr >> 6) & 1;  /* rs0 */

    mc6821core_store(&my6821, port /* rs1 */, reg /* rs0 */, data);
}


/****************************************************************************
*
****************************************************************************/
/* ---------------------------------------------------------------------*/

/*
    the "mf-windows" stuff only works if it reads RAM here, the freezer must
    however always read ROM
*/
BYTE magicformel_romh_read(WORD addr)
{
    if (freeze_enabled && addr >= 0xe000) {
        return romh_banks[(addr & 0x1fff) + (romh_bank << 13)];
    }
    return mem_read_without_ultimax(addr);
}

BYTE magicformel_romh_read_hirom(WORD addr)
{
    if (addr >= 0xe000) {
        return romh_banks[(addr & 0x1fff) + (romh_bank << 13)];
    }
    return mem_read_without_ultimax(addr);
}

int magicformel_romh_phi1_read(WORD addr, BYTE *value)
{
    return CART_READ_C64MEM;
}

int magicformel_romh_phi2_read(WORD addr, BYTE *value)
{
    return magicformel_romh_phi1_read(addr, value);
}

int magicformel_peek_mem(export_t *export, WORD addr, BYTE *value)
{
    if (addr >= 0xe000) {
        *value = romh_banks[(addr & 0x1fff) + (romh_bank << 13)];
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH;
}

/****************************************************************************/

/* ultimax, rom bank 1 */
void magicformel_freeze(void)
{
    DBG(("MF: freeze\n"));
    /* mc6821_reset(); */

    kernal_enabled = 1;   /* PB7 */
    romh_bank = 0x01;
    io1_enabled = 1;

    freeze_flipflop(0 /* reset */, 1 /* freeze */, my6821.CB2);

    cart_config_changed_slotmain(2, (BYTE)(3 | ((romh_bank & 0x0f) << CMODE_BANK_SHIFT)), CMODE_READ | CMODE_RELEASE_FREEZE);
}

void magicformel_config_init(void)
{
    DBG(("MF: init\n"));

    my6821.set_pa = mf_set_pa;
    my6821.set_pb = mf_set_pb;
    /* my6821.set_ca2 = mf_set_ca2; */
    my6821.set_cb2 = mf_set_cb2;

    kernal_enabled = 1;   /* PB7 */

    freeze_flipflop(1 /* reset */, 0 /* freeze */, my6821.CB2);

    cart_config_changed_slotmain(2, (BYTE)(3 | (romh_bank << CMODE_BANK_SHIFT)), CMODE_READ);
}

void magicformel_reset(void)
{
    DBG(("MF: reset\n"));

    romh_bank = 0;      /* PA0..PA3 */
    io1_enabled = 0;    /* PA4 */

    ram_page = 0;       /* PB0..PB4 */
    kernal_enabled = 0; /* PB7 */

    mc6821core_reset(&my6821);
}

void magicformel_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x20000);
    memcpy(romh_banks, rawcart, 0x20000);
}

static int magicformel_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    magicformel_io1_list_item = io_source_register(&magicformel_io1_device);
    magicformel_io2_list_item = io_source_register(&magicformel_io2_device);
    return 0;
}

int magicformel_bin_attach(const char *filename, BYTE *rawcart)
{
    hwversion = 2;
    if (util_file_load(filename, rawcart, 0x20000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        hwversion = 1;
        if (util_file_load(filename, rawcart, 0x18000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            hwversion = 0;
            if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                return -1;
            }
        }
        memcpy(&rawcart[0x18000], &rawcart[0x10000], 0x8000);
    }
    return magicformel_common_attach();
}

/*
    load CRT, handles 64k (v1.2), 64k+32k (v2.0), 64k+64k (v2.0)
*/

int magicformel_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int i, cnt = 0;

    for (i = 0; i <= 15; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > 15 || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
        cnt++;
    }

    if (cnt == 8) {
        DBG(("MF: 64k ROM loaded.\n"));
        hwversion = 0;
    } else if (cnt == 12) {
        DBG(("MF: 64k+32k ROM loaded.\n"));
        hwversion = 1;
        memcpy(&rawcart[0x18000], &rawcart[0x10000], 0x8000);
    } else if (cnt == 16) {
        DBG(("MF: 2*64k ROM loaded.\n"));
        hwversion = 2;
    } else {
        return -1;
    }
    return magicformel_common_attach();
}

void magicformel_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(magicformel_io1_list_item);
    io_source_unregister(magicformel_io2_list_item);
    magicformel_io1_list_item = NULL;
    magicformel_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTMF snapshot module format:

   type  | name          | description
   -----------------------------------
   BYTE  | RAM page      | current RAM page
   BYTE  | I/O-1 enable  | I/O-1 enable flag
   BYTE  | kernal enable | kernal enable flag
   BYTE  | freeze enable | freeze enable flag
   BYTE  | export game   | game line state
   BYTE  | hw version    | hardware version
   ARRAY | ROML          | 131072 BYTES of ROML data
   ARRAY | RAM           | 8192 BYTES of RAM data
   BYTE  | CTRL A        | A control register
   BYTE  | CTRL B        | B control register
   BYTE  | DATA A        | A data register
   BYTE  | DATA B        | B data register
   BYTE  | DIR A         | A direction register
   BYTE  | DIR B         | B direction register
   BYTE  | CA2           | CA2 line flag
   BYTE  | CA2 state     | CA2 line state
   BYTE  | CB2           | CB2 line flag
   BYTE  | CB2 state     | CB2 line state
 */

static char snap_module_name[] = "CARTMF";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int magicformel_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)ram_page) < 0)
        || (SMW_B(m, (BYTE)io1_enabled) < 0)
        || (SMW_B(m, (BYTE)kernal_enabled) < 0)
        || (SMW_B(m, (BYTE)freeze_enabled) < 0)
        || (SMW_B(m, (BYTE)export_game) < 0)
        || (SMW_B(m, (BYTE)hwversion) < 0)
        || (SMW_BA(m, roml_banks, 0x20000) < 0)
        || (SMW_BA(m, export_ram0, 0x2000) < 0)) {
        goto fail;
    }

    if (mc6821core_snapshot_write_data(&my6821, m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

int magicformel_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B_INT(m, &ram_page) < 0)
        || (SMR_B_INT(m, &io1_enabled) < 0)
        || (SMR_B_INT(m, &kernal_enabled) < 0)
        || (SMR_B_INT(m, &freeze_enabled) < 0)
        || (SMR_B_INT(m, &export_game) < 0)
        || (SMR_B_INT(m, &hwversion) < 0)
        || (SMR_BA(m, roml_banks, 0x20000) < 0)
        || (SMR_BA(m, export_ram0, 0x2000) < 0)) {
        goto fail;
    }

    if (mc6821core_snapshot_read_data(&my6821, m) < 0) {
        return -1;
    }

    snapshot_module_close(m);

    memcpy(romh_banks, roml_banks, 0x20000);

    return magicformel_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
