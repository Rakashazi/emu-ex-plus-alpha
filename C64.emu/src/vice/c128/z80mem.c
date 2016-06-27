/*
 * z80mem.c
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
#include <string.h>

#include "c128mem.h"
#include "c128mmu.h"
#include "c64cia.h"
#include "cartio.h"
#include "cmdline.h"
#include "log.h"
#include "mem.h"
#include "resources.h"
#include "sid.h"
#include "sysfile.h"
#include "types.h"
#include "vdc-mem.h"
#include "vdc.h"
#include "vicii-mem.h"
#include "vicii.h"
#include "z80mem.h"
#include "z80.h"

/* Z80 boot BIOS.  */
BYTE z80bios_rom[0x1000];

/* Logging.  */
static log_t z80mem_log = LOG_ERR;

/* Pointers to the currently used memory read and write tables.  */
read_func_ptr_t *_z80mem_read_tab_ptr;
store_func_ptr_t *_z80mem_write_tab_ptr;
BYTE **_z80mem_read_base_tab_ptr;
int *z80mem_read_limit_tab_ptr;

#define NUM_CONFIGS 8

/* Memory read and write tables.  */
static store_func_ptr_t mem_write_tab[NUM_CONFIGS][0x101];
static read_func_ptr_t mem_read_tab[NUM_CONFIGS][0x101];
static BYTE *mem_read_base_tab[NUM_CONFIGS][0x101];
static int mem_read_limit_tab[NUM_CONFIGS][0x101];

store_func_ptr_t io_write_tab[0x101];
read_func_ptr_t io_read_tab[0x101];

/* ------------------------------------------------------------------------- */

/* Generic memory access.  */
#if 0
static void z80mem_store(WORD addr, BYTE value)
{
    _z80mem_write_tab_ptr[addr >> 8](addr, value);
}

static BYTE z80mem_read(WORD addr)
{
    return _z80mem_read_tab_ptr[addr >> 8](addr);
}
#endif

BYTE bios_read(WORD addr)
{
    return z80bios_rom[addr & 0x0fff];
}

void bios_store(WORD addr, BYTE value)
{
    z80bios_rom[addr] = value;
}

static BYTE z80_read_zero(WORD addr)
{
    return mem_page_zero[addr];
}

static void z80_store_zero(WORD addr, BYTE value)
{
    mem_page_zero[addr] = value;
}

static BYTE read_unconnected_io(WORD addr)
{
    log_message(z80mem_log, "Read from unconnected IO %04x", addr);
#ifdef Z80_4MHZ
    z80_clock_stretch();
#endif
    return 0;
}

static void store_unconnected_io(WORD addr, BYTE value)
{
    log_message(z80mem_log, "Store to unconnected IO %04x %02x", addr, value);
#ifdef Z80_4MHZ
    z80_clock_stretch();
#endif
}

#ifdef Z80_4MHZ
static BYTE z80_c64io_d000_read(WORD adr)
{
    z80_clock_stretch();
    return c64io_d000_read(adr);
}

static void z80_c64io_d000_store(WORD adr, BYTE val)
{
    z80_clock_stretch();
    c64io_d000_store(adr, val);
}

static BYTE z80_c64io_d100_read(WORD adr)
{
    z80_clock_stretch();
    return c64io_d100_read(adr);
}

static void z80_c64io_d100_store(WORD adr, BYTE val)
{
    z80_clock_stretch();
    c64io_d100_store(adr, val);
}

static BYTE z80_c64io_d200_read(WORD adr)
{
    z80_clock_stretch();
    return c64io_d200_read(adr);
}

static void z80_c64io_d200_store(WORD adr, BYTE val)
{
    z80_clock_stretch();
    c64io_d200_store(adr, val);
}

static BYTE z80_c64io_d300_read(WORD adr)
{
    z80_clock_stretch();
    return c64io_d300_read(adr);
}

static void z80_c64io_d300_store(WORD adr, BYTE val)
{
    z80_clock_stretch();
    c64io_d300_store(adr, val);
}

static BYTE z80_c64io_d400_read(WORD adr)
{
    z80_clock_stretch();
    return c64io_d400_read(adr);
}

static void z80_c64io_d400_store(WORD adr, BYTE val)
{
    z80_clock_stretch();
    c64io_d400_store(adr, val);
}

static BYTE z80_mmu_read(WORD adr)
{
    z80_clock_stretch();
    return mmu_read(adr);
}

static void z80_mmu_store(WORD adr, BYTE val)
{
    z80_clock_stretch();
    mmu_store(adr, val);
}

static BYTE z80_vdc_read(WORD adr)
{
    z80_clock_stretch();
    return vdc_read(adr);
}

static void z80_vdc_store(WORD adr, BYTE val)
{
    z80_clock_stretch();
    vdc_store(adr, val);
}

static BYTE z80_c64io_d700_read(WORD adr)
{
    z80_clock_stretch();
    return c64io_d700_read(adr);
}

static void z80_c64io_d700_store(WORD adr, BYTE val)
{
    z80_clock_stretch();
    c64io_d700_store(adr, val);
}

static BYTE z80_colorram_read(WORD adr)
{
    z80_clock_stretch();
    return colorram_read(adr);
}

static void z80_colorram_store(WORD adr, BYTE val)
{
    z80_clock_stretch();
    colorram_store(adr, val);
}

static BYTE z80_cia1_read(WORD adr)
{
    z80_clock_stretch();
    return cia1_read(adr);
}

static void z80_cia1_store(WORD adr, BYTE val)
{
    z80_clock_stretch();
    cia1_store(adr, val);
}

static BYTE z80_cia2_read(WORD adr)
{
    z80_clock_stretch();
    return cia2_read(adr);
}

static void z80_cia2_store(WORD adr, BYTE val)
{
    z80_clock_stretch();
    cia2_store(adr, val);
}

static BYTE z80_c64io_de00_read(WORD adr)
{
    z80_clock_stretch();
    return c64io_de00_read(adr);
}

static void z80_c64io_de00_store(WORD adr, BYTE val)
{
    z80_clock_stretch();
    c64io_de00_store(adr, val);
}

static BYTE z80_c64io_df00_read(WORD adr)
{
    z80_clock_stretch();
    return c64io_df00_read(adr);
}

static void z80_c64io_df00_store(WORD adr, BYTE val)
{
    z80_clock_stretch();
    c64io_df00_store(adr, val);
}
#else
#define z80_c64io_d000_read   c64io_d000_read
#define z80_c64io_d000_store  c64io_d000_store
#define z80_c64io_d100_read   c64io_d100_read
#define z80_c64io_d100_store  c64io_d100_store
#define z80_c64io_d200_read   c64io_d200_read
#define z80_c64io_d200_store  c64io_d200_store
#define z80_c64io_d300_read   c64io_d300_read
#define z80_c64io_d300_store  c64io_d300_store
#define z80_c64io_d400_read   c64io_d400_read
#define z80_c64io_d400_store  c64io_d400_store
#define z80_mmu_read          mmu_read
#define z80_mmu_store         mmu_store
#define z80_vdc_read          vdc_read
#define z80_vdc_store         vdc_store
#define z80_c64io_d700_read   c64io_d700_read
#define z80_c64io_d700_store  c64io_d700_store
#define z80_colorram_read     colorram_read
#define z80_colorram_store    colorram_store
#define z80_cia1_read         cia1_read
#define z80_cia1_store        cia1_store
#define z80_cia2_read         cia2_read
#define z80_cia2_store        cia2_store
#define z80_c64io_de00_read   c64io_de00_read
#define z80_c64io_de00_store  c64io_de00_store
#define z80_c64io_df00_read   c64io_df00_read
#define z80_c64io_df00_store  c64io_df00_store
#endif

#ifdef _MSC_VER
#pragma optimize("",off)
#endif

void z80mem_initialize(void)
{
    int i, j;

    /* Memory addess space.  */

    for (j = 0; j < NUM_CONFIGS; j++) {
        for (i = 0; i <= 0x100; i++) {
            mem_read_base_tab[j][i] = NULL;
            mem_read_limit_tab[j][i] = -1;
        }
    }

    mem_read_tab[0][0] = bios_read;
    mem_write_tab[0][0] = z80_store_zero;
    mem_read_tab[1][0] = bios_read;
    mem_write_tab[1][0] = z80_store_zero;
    mem_read_tab[2][0] = z80_read_zero;
    mem_write_tab[2][0] = z80_store_zero;
    mem_read_tab[3][0] = z80_read_zero;
    mem_write_tab[3][0] = z80_store_zero;
    mem_read_tab[4][0] = z80_read_zero;
    mem_write_tab[4][0] = z80_store_zero;
    mem_read_tab[5][0] = z80_read_zero;
    mem_write_tab[5][0] = z80_store_zero;
    mem_read_tab[6][0] = z80_read_zero;
    mem_write_tab[6][0] = z80_store_zero;
    mem_read_tab[7][0] = z80_read_zero;
    mem_write_tab[7][0] = z80_store_zero;

    mem_read_tab[0][1] = bios_read;
    mem_write_tab[0][1] = one_store;
    mem_read_tab[1][1] = bios_read;
    mem_write_tab[1][1] = one_store;
    mem_read_tab[2][1] = one_read;
    mem_write_tab[2][1] = one_store;
    mem_read_tab[3][1] = one_read;
    mem_write_tab[3][1] = one_store;
    mem_read_tab[4][1] = one_read;
    mem_write_tab[4][1] = one_store;
    mem_read_tab[5][1] = one_read;
    mem_write_tab[5][1] = one_store;
    mem_read_tab[6][1] = one_read;
    mem_write_tab[6][1] = one_store;
    mem_read_tab[7][1] = one_read;
    mem_write_tab[7][1] = one_store;

    for (i = 2; i < 0x10; i++) {
        mem_read_tab[0][i] = bios_read;
        mem_write_tab[0][i] = ram_store;
        mem_read_tab[1][i] = bios_read;
        mem_write_tab[1][i] = ram_store;
        mem_read_tab[2][i] = lo_read;
        mem_write_tab[2][i] = lo_store;
        mem_read_tab[3][i] = lo_read;
        mem_write_tab[3][i] = lo_store;
        mem_read_tab[4][i] = ram_read;
        mem_write_tab[4][i] = ram_store;
        mem_read_tab[5][i] = ram_read;
        mem_write_tab[5][i] = ram_store;
        mem_read_tab[6][i] = lo_read;
        mem_write_tab[6][i] = lo_store;
        mem_read_tab[7][i] = lo_read;
        mem_write_tab[7][i] = lo_store;
    }

    for (i = 0x10; i <= 0x13; i++) {
        mem_read_tab[0][i] = ram_read;
        mem_write_tab[0][i] = ram_store;
        mem_read_tab[1][i] = colorram_read;
        mem_write_tab[1][i] = colorram_store;
        mem_read_tab[2][i] = lo_read;
        mem_write_tab[2][i] = lo_store;
        mem_read_tab[3][i] = colorram_read;
        mem_write_tab[3][i] = colorram_store;
        mem_read_tab[4][i] = ram_read;
        mem_write_tab[4][i] = ram_store;
        mem_read_tab[5][i] = colorram_read;
        mem_write_tab[5][i] = colorram_store;
        mem_read_tab[6][i] = lo_read;
        mem_write_tab[6][i] = lo_store;
        mem_read_tab[7][i] = colorram_read;
        mem_write_tab[7][i] = colorram_store;
    }

    for (i = 0x14; i <= 0x3f; i++) {
        mem_read_tab[0][i] = ram_read;
        mem_write_tab[0][i] = ram_store;
        mem_read_tab[1][i] = ram_read;
        mem_write_tab[1][i] = ram_store;
        mem_read_tab[2][i] = lo_read;
        mem_write_tab[2][i] = lo_store;
        mem_read_tab[3][i] = lo_read;
        mem_write_tab[3][i] = lo_store;
        mem_read_tab[4][i] = ram_read;
        mem_write_tab[4][i] = ram_store;
        mem_read_tab[5][i] = ram_read;
        mem_write_tab[5][i] = ram_store;
        mem_read_tab[6][i] = lo_read;
        mem_write_tab[6][i] = lo_store;
        mem_read_tab[7][i] = lo_read;
        mem_write_tab[7][i] = lo_store;
    }

    for (j = 0; j < NUM_CONFIGS; j++) {
        for (i = 0x40; i <= 0xbf; i++) {
            mem_read_tab[j][i] = ram_read;
            mem_write_tab[j][i] = ram_store;
        }
    }

    for (i = 0xc0; i <= 0xcf; i++) {
        mem_read_tab[0][i] = ram_read;
        mem_write_tab[0][i] = ram_store;
        mem_read_tab[1][i] = ram_read;
        mem_write_tab[1][i] = ram_store;
        mem_read_tab[2][i] = top_shared_read;
        mem_write_tab[2][i] = top_shared_store;
        mem_read_tab[3][i] = top_shared_read;
        mem_write_tab[3][i] = top_shared_store;
        mem_read_tab[4][i] = ram_read;
        mem_write_tab[4][i] = ram_store;
        mem_read_tab[5][i] = ram_read;
        mem_write_tab[5][i] = ram_store;
        mem_read_tab[6][i] = top_shared_read;
        mem_write_tab[6][i] = top_shared_store;
        mem_read_tab[7][i] = top_shared_read;
        mem_write_tab[7][i] = top_shared_store;
    }

    for (i = 0xd0; i <= 0xdf; i++) {
        mem_read_tab[0][i] = ram_read;
        mem_write_tab[0][i] = ram_store;
        mem_read_tab[1][i] = ram_read;
        mem_write_tab[1][i] = ram_store;
        mem_read_tab[2][i] = top_shared_read;
        mem_write_tab[2][i] = top_shared_store;
        mem_read_tab[3][i] = top_shared_read;
        mem_write_tab[3][i] = top_shared_store;
        mem_read_tab[4][i] = ram_read;
        mem_write_tab[4][i] = ram_store;
        mem_read_tab[5][i] = ram_read;
        mem_write_tab[5][i] = ram_store;
        mem_read_tab[6][i] = top_shared_read;
        mem_write_tab[6][i] = top_shared_store;
        mem_read_tab[7][i] = top_shared_read;
        mem_write_tab[7][i] = top_shared_store;
    }

    for (i = 0xe0; i <= 0xfe; i++) {
        mem_read_tab[0][i] = ram_read;
        mem_write_tab[0][i] = ram_store;
        mem_read_tab[1][i] = ram_read;
        mem_write_tab[1][i] = ram_store;
        mem_read_tab[2][i] = top_shared_read;
        mem_write_tab[2][i] = top_shared_store;
        mem_read_tab[3][i] = top_shared_read;
        mem_write_tab[3][i] = top_shared_store;
        mem_read_tab[4][i] = ram_read;
        mem_write_tab[4][i] = ram_store;
        mem_read_tab[5][i] = ram_read;
        mem_write_tab[5][i] = ram_store;
        mem_read_tab[6][i] = top_shared_read;
        mem_write_tab[6][i] = top_shared_store;
        mem_read_tab[7][i] = top_shared_read;
        mem_write_tab[7][i] = top_shared_store;
    }

    for (j = 0; j < NUM_CONFIGS; j++) {
        mem_read_tab[j][0xff] = mmu_ffxx_read_z80;
        mem_write_tab[j][0xff] = mmu_ffxx_store;

        mem_read_tab[j][0x100] = mem_read_tab[j][0x0];
        mem_write_tab[j][0x100] = mem_write_tab[j][0x0];
    }

    z80mem_update_config(0);

    /* IO address space.  */

    /* At least we know what happens.  */
    for (i = 0; i <= 0x100; i++) {
        io_read_tab[i] = read_unconnected_io;
        io_write_tab[i] = store_unconnected_io;
    }
    io_read_tab[0xd0] = z80_c64io_d000_read;
    io_write_tab[0xd0] = z80_c64io_d000_store;
    io_read_tab[0xd1] = z80_c64io_d100_read;
    io_write_tab[0xd1] = z80_c64io_d100_store;
    io_read_tab[0xd2] = z80_c64io_d200_read;
    io_write_tab[0xd2] = z80_c64io_d200_store;
    io_read_tab[0xd3] = z80_c64io_d300_read;
    io_write_tab[0xd3] = z80_c64io_d300_store;

    io_read_tab[0xd4] = z80_c64io_d400_read;
    io_write_tab[0xd4] = z80_c64io_d400_store;

    io_read_tab[0xd5] = z80_mmu_read;
    io_write_tab[0xd5] = z80_mmu_store;

    io_read_tab[0xd6] = z80_vdc_read;
    io_write_tab[0xd6] = z80_vdc_store;

    io_read_tab[0xd7] = z80_c64io_d700_read;
    io_write_tab[0xd7] = z80_c64io_d700_store;

    io_read_tab[0xd8] = z80_colorram_read;
    io_write_tab[0xd8] = z80_colorram_store;
    io_read_tab[0xd9] = z80_colorram_read;
    io_write_tab[0xd9] = z80_colorram_store;
    io_read_tab[0xda] = z80_colorram_read;
    io_write_tab[0xda] = z80_colorram_store;
    io_read_tab[0xdb] = z80_colorram_read;
    io_write_tab[0xdb] = z80_colorram_store;

    io_read_tab[0xdc] = z80_cia1_read;
    io_write_tab[0xdc] = z80_cia1_store;
    io_read_tab[0xdd] = z80_cia2_read;
    io_write_tab[0xdd] = z80_cia2_store;

    io_read_tab[0xde] = z80_c64io_de00_read;
    io_write_tab[0xde] = z80_c64io_de00_store;
    io_read_tab[0xdf] = z80_c64io_df00_read;
    io_write_tab[0xdf] = z80_c64io_df00_store;
}

#ifdef _MSC_VER
#pragma optimize("",on)
#endif

void z80mem_update_config(int config)
{
    _z80mem_read_tab_ptr = mem_read_tab[config];
    _z80mem_write_tab_ptr = mem_write_tab[config];
    _z80mem_read_base_tab_ptr = mem_read_base_tab[config];
    z80mem_read_limit_tab_ptr = mem_read_limit_tab[config];

    z80_resync_limits();
}

int z80mem_load(void)
{
    if (z80mem_log == LOG_ERR) {
        z80mem_log = log_open("Z80MEM");
    }

    z80mem_initialize();

    return 0;
}
