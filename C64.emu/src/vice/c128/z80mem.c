/**\file    z80mem.c
 * \brief   Z80 memory handling
 *
 * \author  Andreas Boose <viceteam@t-online.de>
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

#include "c128cart.h"
#include "c128mem.h"
#include "c128mmu.h"
#include "c64cia.h"
#include "c128memrom.h"
#include "c64memrom.h"
#include "cartio.h"
#include "cmdline.h"
#include "functionrom.h"
#include "log.h"
#include "mem.h"
#include "resources.h"
#include "reu.h"
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
uint8_t z80bios_rom[0x1000];

/* Logging.  */
static log_t z80mem_log = LOG_ERR;

/* Pointers to the currently used memory read and write tables.  */
read_func_ptr_t *_z80mem_read_tab_ptr;
store_func_ptr_t *_z80mem_write_tab_ptr;
uint8_t **_z80mem_read_base_tab_ptr;
int *z80mem_read_limit_tab_ptr;

int z80mem_config;

#define NUM_CONFIGS 16
#define NUM_Z80_C128MODE_CONFIGS 8

/* Memory read and write tables.  */
static store_func_ptr_t mem_write_tab[NUM_CONFIGS][0x101];
static read_func_ptr_t mem_read_tab[NUM_CONFIGS][0x101];
static uint8_t *mem_read_base_tab[NUM_CONFIGS][0x101];
static int mem_read_limit_tab[NUM_CONFIGS][0x101];

store_func_ptr_t io_write_tab[0x101];
read_func_ptr_t io_read_tab[0x101];

/* ------------------------------------------------------------------------- */

/* Generic memory access.  */
#if 0
static void z80mem_store(uint16_t addr, uint8_t value)
{
    _z80mem_write_tab_ptr[addr >> 8](addr, value);
}

static uint8_t z80mem_read(uint16_t addr)
{
    return _z80mem_read_tab_ptr[addr >> 8](addr);
}
#endif

uint8_t bios_read(uint16_t addr)
{
    return z80bios_rom[addr & 0x0fff];
}

void bios_store(uint16_t addr, uint8_t value)
{
    z80bios_rom[addr] = value;
}

static uint8_t read_unconnected_io(uint16_t addr)
{
#ifdef Z80_4MHZ
    z80_clock_stretch();
#endif
    return _z80mem_read_tab_ptr[addr >> 8](addr);
}

static void store_unconnected_io(uint16_t addr, uint8_t value)
{
#ifdef Z80_4MHZ
    z80_clock_stretch();
#endif
    _z80mem_write_tab_ptr[addr >> 8](addr, value);
}

#ifdef Z80_4MHZ
static uint8_t z80_c64io_d000_read(uint16_t adr)
{
    z80_clock_stretch();
    return c64io_d000_read(adr);
}

static void z80_c64io_d000_store(uint16_t adr, uint8_t val)
{
    z80_clock_stretch();
    c64io_d000_store(adr, val);
}

static uint8_t z80_c64io_d100_read(uint16_t adr)
{
    z80_clock_stretch();
    return c64io_d100_read(adr);
}

static void z80_c64io_d100_store(uint16_t adr, uint8_t val)
{
    z80_clock_stretch();
    c64io_d100_store(adr, val);
}

static uint8_t z80_c64io_d200_read(uint16_t adr)
{
    z80_clock_stretch();
    return c64io_d200_read(adr);
}

static void z80_c64io_d200_store(uint16_t adr, uint8_t val)
{
    z80_clock_stretch();
    c64io_d200_store(adr, val);
}

static uint8_t z80_c64io_d300_read(uint16_t adr)
{
    z80_clock_stretch();
    return c64io_d300_read(adr);
}

static void z80_c64io_d300_store(uint16_t adr, uint8_t val)
{
    z80_clock_stretch();
    c64io_d300_store(adr, val);
}

static uint8_t z80_c64io_d400_read(uint16_t adr)
{
    z80_clock_stretch();
    return c64io_d400_read(adr);
}

static void z80_c64io_d400_store(uint16_t adr, uint8_t val)
{
    z80_clock_stretch();
    c64io_d400_store(adr, val);
}

static uint8_t z80_mmu_read(uint16_t adr)
{
    z80_clock_stretch();
    return mmu_read(adr);
}

static void z80_mmu_store(uint16_t adr, uint8_t val)
{
    z80_clock_stretch();
    mmu_store(adr, val);
}

static uint8_t z80_vdc_read(uint16_t adr)
{
    z80_clock_stretch();
    return vdc_read(adr);
}

static void z80_vdc_store(uint16_t adr, uint8_t val)
{
    z80_clock_stretch();
    vdc_store(adr, val);
}

static uint8_t z80_c64io_d700_read(uint16_t adr)
{
    z80_clock_stretch();
    return c64io_d700_read(adr);
}

static void z80_c64io_d700_store(uint16_t adr, uint8_t val)
{
    z80_clock_stretch();
    c64io_d700_store(adr, val);
}

static uint8_t z80_colorram_read(uint16_t adr)
{
    z80_clock_stretch();
    return colorram_read(adr);
}

static void z80_colorram_store(uint16_t adr, uint8_t val)
{
    z80_clock_stretch();
    colorram_store(adr, val);
}

static uint8_t z80_cia1_read(uint16_t adr)
{
    z80_clock_stretch();
    return cia1_read(adr);
}

static void z80_cia1_store(uint16_t adr, uint8_t val)
{
    z80_clock_stretch();
    cia1_store(adr, val);
}

static uint8_t z80_cia2_read(uint16_t adr)
{
    z80_clock_stretch();
    return cia2_read(adr);
}

static void z80_cia2_store(uint16_t adr, uint8_t val)
{
    z80_clock_stretch();
    cia2_store(adr, val);
}

static uint8_t z80_c64io_de00_read(uint16_t adr)
{
    z80_clock_stretch();
    return c64io_de00_read(adr);
}

static void z80_c64io_de00_store(uint16_t adr, uint8_t val)
{
    z80_clock_stretch();
    c64io_de00_store(adr, val);
}

static uint8_t z80_c64io_df00_read(uint16_t adr)
{
    z80_clock_stretch();
    return c64io_df00_read(adr);
}

static void z80_c64io_df00_store(uint16_t adr, uint8_t val)
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

#define z80_c64io_d000_peek   c64io_d000_peek
#define z80_c64io_d100_peek   c64io_d100_peek
#define z80_c64io_d200_peek   c64io_d200_peek
#define z80_c64io_d300_peek   c64io_d300_peek
#define z80_c64io_d400_peek   c64io_d400_peek
#define z80_mmu_peek          mmu_peek
#define z80_vdc_peek          vdc_peek
#define z80_c64io_d700_peek   c64io_d700_peek
#define z80_colorram_peek     colorram_peek
#define z80_cia1_peek         cia1_peek
#define z80_cia2_peek         cia2_peek
#define z80_c64io_de00_peek   c64io_de00_peek
#define z80_c64io_df00_peek   c64io_df00_peek

static uint8_t z80mem_lo_rom_area_read(uint16_t adr)
{
    if (mmu[0] & 2) {
        return ram_read(adr);
    }
    return basic_lo_read(adr);
}

static uint8_t z80mem_lo_rom_area_peek(uint16_t adr)
{
    if (mmu[0] & 2) {
        return ram_peek(adr);
    }
    return c128memrom_basic_rom[adr - 0x4000];
}

static void z80mem_lo_rom_area_store(uint16_t adr, uint8_t val)
{
    if (mmu[0] & 2) {
        ram_store(adr, val);
    } else {
        basic_lo_store(adr, val);
    }
}

static uint8_t z80mem_mid_rom_area_read(uint16_t adr)
{
    switch ((mmu[0] & 0xc) >> 2) {
        case Z80_C128_ROM:
            return basic_hi_read(adr);
            break;
        case Z80_C128_INT_FUNC:
            return internal_function_rom_read(adr);
            break;
        case Z80_C128_EXT_FUNC:
            return external_function_rom_read(adr);
            break;
        case Z80_C128_RAM:
            return ram_read(adr);
            break;
    }
    return 0; /* should not get here */
}

static uint8_t z80mem_mid_rom_area_peek(uint16_t adr)
{
    switch ((mmu[0] & 0xc) >> 2) {
        case Z80_C128_ROM:
            return c128memrom_basic_rom[adr - 0x4000];
            break;
        case Z80_C128_INT_FUNC:
            return internal_function_rom_peek(adr);
            break;
        case Z80_C128_EXT_FUNC:
            return external_function_rom_peek(adr);
            break;
        case Z80_C128_RAM:
            return ram_peek(adr);
            break;
    }
    return 0; /* should not get here */
}

static void z80mem_mid_rom_area_store(uint16_t adr, uint8_t val)
{
    switch ((mmu[0] & 0xc) >> 2) {
        case Z80_C128_ROM:
            basic_hi_store(adr, val);
            break;
        case Z80_C128_INT_FUNC:
            internal_function_rom_store(adr, val);
            break;
        case Z80_C128_EXT_FUNC:
            external_function_rom_store(adr, val);
            break;
        case Z80_C128_RAM:
            ram_store(adr, val);
            break;
    }
}

static uint8_t z80mem_editor_rom_area_read(uint16_t adr)
{
    switch ((mmu[0] & 0x30) >> 4) {
        case Z80_C128_ROM:
            return editor_read(adr);
            break;
        case Z80_C128_INT_FUNC:
            return internal_function_rom_read(adr);
            break;
        case Z80_C128_EXT_FUNC:
            return external_function_rom_read(adr);
            break;
        case Z80_C128_RAM:
            return top_shared_read(adr);
            break;
    }
    return 0; /* should not get here */
}

static uint8_t z80mem_editor_rom_area_peek(uint16_t adr)
{
    switch ((mmu[0] & 0x30) >> 4) {
        case Z80_C128_ROM:
            return c128memrom_basic_rom[adr - 0x4000];
            break;
        case Z80_C128_INT_FUNC:
            return internal_function_rom_peek(adr);
            break;
        case Z80_C128_EXT_FUNC:
            return external_function_rom_peek(adr);
            break;
        case Z80_C128_RAM:
            return top_shared_peek(adr);
            break;
    }
    return 0; /* should not get here */
}

static void z80mem_editor_rom_area_store(uint16_t adr, uint8_t val)
{
    switch ((mmu[0] & 0xc) >> 2) {
        case Z80_C128_ROM:
            editor_store(adr, val);
            break;
        case Z80_C128_INT_FUNC:
            internal_function_rom_store(adr, val);
            break;
        case Z80_C128_EXT_FUNC:
            external_function_rom_store(adr, val);
            break;
        case Z80_C128_RAM:
            top_shared_store(adr, val);
            break;
    }
}

static uint8_t z80mem_chargen_rom_area_read(uint16_t adr)
{
    switch ((mmu[0] & 0x30) >> 4) {
        case Z80_C128_ROM:
            return chargen_read(adr);
            break;
        case Z80_C128_INT_FUNC:
            return internal_function_rom_read(adr);
            break;
        case Z80_C128_EXT_FUNC:
            return external_function_rom_read(adr);
            break;
        case Z80_C128_RAM:
            return top_shared_read(adr);
            break;
    }
    return 0; /* should not get here */
}

static uint8_t z80mem_chargen_rom_area_peek(uint16_t adr)
{
    switch ((mmu[0] & 0x30) >> 4) {
        case Z80_C128_ROM:
            return mem_chargen_rom_ptr[adr & 0x0fff];
            break;
        case Z80_C128_INT_FUNC:
            return internal_function_rom_peek(adr);
            break;
        case Z80_C128_EXT_FUNC:
            return external_function_rom_peek(adr);
            break;
        case Z80_C128_RAM:
            return top_shared_peek(adr);
            break;
    }
    return 0; /* should not get here */
}

static void z80mem_chargen_rom_area_store(uint16_t adr, uint8_t val)
{
    switch ((mmu[0] & 0xc) >> 2) {
        case Z80_C128_ROM:
            hi_store(adr, val);
            break;
        case Z80_C128_INT_FUNC:
            internal_function_rom_store(adr, val);
            break;
        case Z80_C128_EXT_FUNC:
            external_function_rom_store(adr, val);
            break;
        case Z80_C128_RAM:
            top_shared_store(adr, val);
            break;
    }
}

static uint8_t z80mem_hi_rom_area_read(uint16_t adr)
{
    switch ((mmu[0] & 0x30) >> 4) {
        case Z80_C128_ROM:
            return hi_read(adr);
            break;
        case Z80_C128_INT_FUNC:
            return internal_function_rom_read(adr);
            break;
        case Z80_C128_EXT_FUNC:
            return external_function_rom_read(adr);
            break;
        case Z80_C128_RAM:
            return top_shared_read(adr);
            break;
    }
    return 0; /* should not get here */
}

static uint8_t z80mem_hi_rom_area_peek(uint16_t adr)
{
    switch ((mmu[0] & 0x30) >> 4) {
        case Z80_C128_ROM:
            return c128memrom_kernal_rom[adr & 0x1fff];
            break;
        case Z80_C128_INT_FUNC:
            return internal_function_rom_peek(adr);
            break;
        case Z80_C128_EXT_FUNC:
            return external_function_rom_peek(adr);
            break;
        case Z80_C128_RAM:
            return top_shared_peek(adr);
            break;
    }
    return 0; /* should not get here */
}

static void z80mem_hi_rom_area_store(uint16_t adr, uint8_t val)
{
    switch ((mmu[0] & 0x30) >> 4) {
        case Z80_C128_ROM:
            hi_store(adr, val);
            break;
        case Z80_C128_INT_FUNC:
            internal_function_rom_store(adr, val);
            break;
        case Z80_C128_EXT_FUNC:
            external_function_rom_store(adr, val);
            break;
        case Z80_C128_RAM:
            top_shared_store(adr, val);
            break;
    }
}

/* in() from $0000-$0fff in bank 0 get translated to memory read from RAM at $d000-$dfff */
static uint8_t z80_io_ram_bank0_read(uint16_t adr)
{
    uint16_t real_adr = (adr & 0x0fff) | 0xd000;

    if (mmu[0] & 0x40) {
        return _z80mem_read_tab_ptr[adr >> 8](adr);
    }

    return top_shared_read(real_adr);
}

/* out() to $0000-$0fff in bank 0 get translated to memory store to RAM at $d000-$dfff */
static void z80_io_ram_bank0_store(uint16_t adr, uint8_t val)
{
    uint16_t real_adr = (adr & 0x0fff) | 0xd000;

    if (mmu[0] & 0x40) {
        _z80mem_write_tab_ptr[adr >> 8](adr, val);
    } else {
        top_shared_store(real_adr, val);
    }
}

static uint8_t c64mode_colorram_read(uint16_t adr)
{
    if (!(mmu[0] & 1)) {
        return colorram_read(adr);
    }
    return lo_read(adr);
}

static void c64mode_colorram_store(uint16_t adr, uint8_t val)
{
    if (!(mmu[0] & 1)) {
        colorram_store(adr, val);
    }
    lo_store(adr, val);
}

static void z80_c64mode_ffxx_store(uint16_t addr, uint8_t value)
{
    if (addr == 0xff00) {
        reu_dma(-1);
    } else {
        top_shared_store(addr, value);
    }
}

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

    /* z80 c128 mode memory map for the zero page */
    mem_read_tab[0][0] = bios_read;
    mem_write_tab[0][0] = z80_io_ram_bank0_store;
    mem_read_tab[1][0] = bios_read;
    mem_write_tab[1][0] = z80_io_ram_bank0_store;
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

    /* z80 c64 mode memory map for the zero page */
    mem_read_tab[8][0] = z80_read_zero;
    mem_write_tab[8][0] = z80_store_zero;
    mem_read_tab[9][0] = z80_read_zero;
    mem_write_tab[9][0] = z80_store_zero;
    mem_read_tab[10][0] = z80_read_zero;
    mem_write_tab[10][0] = z80_store_zero;
    mem_read_tab[11][0] = z80_read_zero;
    mem_write_tab[11][0] = z80_store_zero;
    mem_read_tab[12][0] = z80_read_zero;
    mem_write_tab[12][0] = z80_store_zero;
    mem_read_tab[13][0] = z80_read_zero;
    mem_write_tab[13][0] = z80_store_zero;
    mem_read_tab[14][0] = z80_read_zero;
    mem_write_tab[14][0] = z80_store_zero;
    mem_read_tab[15][0] = z80_read_zero;
    mem_write_tab[15][0] = z80_store_zero;

    /* z80 c128 mode memory map for the stack page */
    mem_read_tab[0][1] = bios_read;
    mem_write_tab[0][1] = z80_io_ram_bank0_store;
    mem_read_tab[1][1] = bios_read;
    mem_write_tab[1][1] = z80_io_ram_bank0_store;
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

    /* z80 c64 mode memory map for the stack page */
    mem_read_tab[8][1] = one_read;
    mem_write_tab[8][1] = one_store;
    mem_read_tab[9][1] = one_read;
    mem_write_tab[9][1] = one_store;
    mem_read_tab[10][1] = one_read;
    mem_write_tab[10][1] = one_store;
    mem_read_tab[11][1] = one_read;
    mem_write_tab[11][1] = one_store;
    mem_read_tab[12][1] = one_read;
    mem_write_tab[12][1] = one_store;
    mem_read_tab[13][1] = one_read;
    mem_write_tab[13][1] = one_store;
    mem_read_tab[14][1] = one_read;
    mem_write_tab[14][1] = one_store;
    mem_read_tab[15][1] = one_read;
    mem_write_tab[15][1] = one_store;

    for (i = 2; i < 0x10; i++) {
        /* z80 c128 mode memory map for $0200-$0fff */
        mem_read_tab[0][i] = bios_read;
        mem_write_tab[0][i] = z80_io_ram_bank0_store;
        mem_read_tab[1][i] = bios_read;
        mem_write_tab[1][i] = z80_io_ram_bank0_store;
        mem_read_tab[2][i] = lo_read;
        mem_write_tab[2][i] = lo_store;
        mem_read_tab[3][i] = lo_read;
        mem_write_tab[3][i] = lo_store;
        mem_read_tab[4][i] = ram_read; /* lo_read? for bank 2 */
        mem_write_tab[4][i] = ram_store; /* lo_store? for bank 2 */
        mem_read_tab[5][i] = ram_read; /* lo_read? for bank 2 */
        mem_write_tab[5][i] = ram_store; /* lo_store? for bank 2 */
        mem_read_tab[6][i] = lo_read;
        mem_write_tab[6][i] = lo_store;
        mem_read_tab[7][i] = lo_read;
        mem_write_tab[7][i] = lo_store;

        /* z80 c64 mode memory map for $0200-$0fff */
        mem_read_tab[8][i] = lo_read;
        mem_write_tab[8][i] = lo_store;
        mem_read_tab[9][i] = lo_read;
        mem_write_tab[9][i] = lo_store;
        mem_read_tab[10][i] = lo_read;
        mem_write_tab[10][i] = lo_store;
        mem_read_tab[11][i] = lo_read;
        mem_write_tab[11][i] = lo_store;
        mem_read_tab[12][i] = lo_read;
        mem_write_tab[12][i] = lo_store;
        mem_read_tab[13][i] = lo_read;
        mem_write_tab[13][i] = lo_store;
        mem_read_tab[14][i] = lo_read;
        mem_write_tab[14][i] = lo_store;
        mem_read_tab[15][i] = lo_read;
        mem_write_tab[15][i] = lo_store;
    }

    for (i = 0x10; i <= 0x13; i++) {
        /* z80 c128 mode memory map for $1000-$13ff */
        mem_read_tab[0][i] = ram_read;
        mem_write_tab[0][i] = ram_store;
        mem_read_tab[1][i] = colorram_read;
        mem_write_tab[1][i] = colorram_store;
        mem_read_tab[2][i] = lo_read;
        mem_write_tab[2][i] = lo_store;
        mem_read_tab[3][i] = colorram_read;
        mem_write_tab[3][i] = colorram_store;
        mem_read_tab[4][i] = ram_read; /* lo_read? for bank 2 */
        mem_write_tab[4][i] = ram_store; /* lo_store? for bank 2 */
        mem_read_tab[5][i] = colorram_read;
        mem_write_tab[5][i] = colorram_store;
        mem_read_tab[6][i] = lo_read;
        mem_write_tab[6][i] = lo_store;
        mem_read_tab[7][i] = colorram_read;
        mem_write_tab[7][i] = colorram_store;

        /* z80 c64 mode memory map for $1000-$13ff */
        mem_read_tab[8][i] = c64mode_colorram_read;
        mem_write_tab[8][i] = c64mode_colorram_store;
        mem_read_tab[9][i] = c64mode_colorram_read;
        mem_write_tab[9][i] = c64mode_colorram_store;
        mem_read_tab[10][i] = c64mode_colorram_read;
        mem_write_tab[10][i] = c64mode_colorram_store;
        mem_read_tab[11][i] = c64mode_colorram_read;
        mem_write_tab[11][i] = c64mode_colorram_store;
        mem_read_tab[12][i] = c64mode_colorram_read;
        mem_write_tab[12][i] = c64mode_colorram_store;
        mem_read_tab[13][i] = c64mode_colorram_read;
        mem_write_tab[13][i] = c64mode_colorram_store;
        mem_read_tab[14][i] = c64mode_colorram_read;
        mem_write_tab[14][i] = c64mode_colorram_store;
        mem_read_tab[15][i] = c64mode_colorram_read;
        mem_write_tab[15][i] = c64mode_colorram_store;
    }

    for (i = 0x14; i <= 0x3f; i++) {
        /* z80 c128 mode memory map for $1400-$3fff */
        mem_read_tab[0][i] = ram_read;
        mem_write_tab[0][i] = ram_store;
        mem_read_tab[1][i] = ram_read;
        mem_write_tab[1][i] = ram_store;
        mem_read_tab[2][i] = lo_read;
        mem_write_tab[2][i] = lo_store;
        mem_read_tab[3][i] = lo_read;
        mem_write_tab[3][i] = lo_store;
        mem_read_tab[4][i] = ram_read; /* lo_read? for bank 2 */
        mem_write_tab[4][i] = ram_store; /* lo_store? for bank 2 */
        mem_read_tab[5][i] = ram_read; /* lo_read? for bank 2 */
        mem_write_tab[5][i] = ram_store; /* lo_store? for bank 2 */
        mem_read_tab[6][i] = lo_read;
        mem_write_tab[6][i] = lo_store;
        mem_read_tab[7][i] = lo_read;
        mem_write_tab[7][i] = lo_store;

        /* z80 c64 mode memory map for $1400-$3fff */
        mem_read_tab[8][i] = lo_read;
        mem_write_tab[8][i] = lo_store;
        mem_read_tab[9][i] = lo_read;
        mem_write_tab[9][i] = lo_store;
        mem_read_tab[10][i] = lo_read;
        mem_write_tab[10][i] = lo_store;
        mem_read_tab[11][i] = lo_read;
        mem_write_tab[11][i] = lo_store;
        mem_read_tab[12][i] = lo_read;
        mem_write_tab[12][i] = lo_store;
        mem_read_tab[13][i] = lo_read;
        mem_write_tab[13][i] = lo_store;
        mem_read_tab[14][i] = lo_read;
        mem_write_tab[14][i] = lo_store;
        mem_read_tab[15][i] = lo_read;
        mem_write_tab[15][i] = lo_store;
    }

    for (i = 0x40; i <= 0x7f; i++) {
        /* z80 c128 mode memory map for $4000-$7fff */
        for (j = 0; j < NUM_Z80_C128MODE_CONFIGS; j++) {
            mem_read_tab[j][i] = z80mem_lo_rom_area_read;
            mem_write_tab[j][i] = z80mem_lo_rom_area_store;
        }

        /* z80 c64 mode memory map for $4000-$7fff */
        for (j = NUM_Z80_C128MODE_CONFIGS; j < NUM_CONFIGS; j++) {
            mem_read_tab[j][i] = ram_read;
            mem_write_tab[j][i] = ram_store;
        }
    }

    for (i = 0x80; i <= 0x9f; i++) {
        /* z80 c128 mode memory map for $8000-$9fff */
        for (j = 0; j < NUM_Z80_C128MODE_CONFIGS; j++) {
            mem_read_tab[j][i] = z80mem_mid_rom_area_read;
            mem_write_tab[j][i] = z80mem_mid_rom_area_store;
        }

        /* z80 c64 mode memory map for $8000-$9fff */
        for (j = NUM_Z80_C128MODE_CONFIGS; j < NUM_CONFIGS; j++) {
            mem_read_tab[j][i] = ram_read;
            mem_write_tab[j][i] = ram_store;
        }
    }

    for (i = 0xa0; i <= 0xbf; i++) {
        /* z80 c128 mode memory map for $a000-$bfff */
        for (j = 0; j < NUM_Z80_C128MODE_CONFIGS; j++) {
            mem_read_tab[j][i] = z80mem_mid_rom_area_read;
            mem_write_tab[j][i] = z80mem_mid_rom_area_store;
        }

        /* z80 c64 mode memory map for $a000-$bfff */
        mem_read_tab[8][i] = ram_read;
        mem_write_tab[8][i] = ram_store;
        mem_read_tab[9][i] = ram_read;
        mem_write_tab[9][i] = ram_store;
        mem_read_tab[10][i] = ram_read;
        mem_write_tab[10][i] = ram_store;
        mem_read_tab[11][i] = c64memrom_basic64_read;
        mem_write_tab[11][i] = ram_store;
        mem_read_tab[12][i] = ram_read;
        mem_write_tab[12][i] = ram_store;
        mem_read_tab[13][i] = ram_read;
        mem_write_tab[13][i] = ram_store;
        mem_read_tab[14][i] = ram_read;
        mem_write_tab[14][i] = ram_store;
        mem_read_tab[15][i] = c64memrom_basic64_read;
        mem_write_tab[15][i] = ram_store;
    }

    for (i = 0xc0; i <= 0xcf; i++) {
        /* z80 c128 mode memory map for $c000-$cfff */
        for (j = 0; j < NUM_Z80_C128MODE_CONFIGS; j++) {
            mem_read_tab[j][i] = z80mem_editor_rom_area_read;
            mem_write_tab[j][i] = z80mem_editor_rom_area_store;
        }

        /* z80 c64 mode memory map for $c000-$cfff */
        for (j = NUM_Z80_C128MODE_CONFIGS; j < NUM_CONFIGS; j++) {
            mem_read_tab[j][i] = top_shared_read;
            mem_write_tab[j][i] = top_shared_store;
        }
    }

    /* z80 c128 mode memory map for $d000-$dfff */
    for (i = 0xd0; i <= 0xdf; i++) {
        for (j = 0; j < NUM_Z80_C128MODE_CONFIGS; j++) {
            mem_read_tab[j][i] = z80mem_chargen_rom_area_read;
            mem_write_tab[j][i] = z80mem_chargen_rom_area_store;
        }
    }

    /* z80 c64 mode memory map for $d000-$dfff */
    for (i = 0xd0; i <= 0xdf; i++) {
        mem_read_tab[8][i] = top_shared_read;
        mem_write_tab[8][i] = top_shared_store;
        mem_read_tab[9][i] = chargen_read;
        mem_write_tab[9][i] = top_shared_store;
        mem_read_tab[10][i] = chargen_read;
        mem_write_tab[10][i] = top_shared_store;
        mem_read_tab[11][i] = chargen_read;
        mem_write_tab[11][i] = top_shared_store;
        mem_read_tab[12][i] = top_shared_read;
        mem_write_tab[12][i] = top_shared_store;
    }
    for (j = 13; j <= 15; j++) {
       mem_read_tab[j][0xd0] = z80_c64io_d000_read;
       mem_write_tab[j][0xd0] = z80_c64io_d000_store;
       mem_read_tab[j][0xd1] = z80_c64io_d100_read;
       mem_write_tab[j][0xd1] = z80_c64io_d100_store;
       mem_read_tab[j][0xd2] = z80_c64io_d200_read;
       mem_write_tab[j][0xd2] = z80_c64io_d200_store;
       mem_read_tab[j][0xd3] = z80_c64io_d300_read;
       mem_write_tab[j][0xd3] = z80_c64io_d300_store;
       mem_read_tab[j][0xd4] = z80_c64io_d400_read;
       mem_write_tab[j][0xd4] = z80_c64io_d400_store;
       mem_read_tab[j][0xd5] = z80_c64io_d700_read;
       mem_write_tab[j][0xd5] = z80_c64io_d700_store;
       mem_read_tab[j][0xd6] = z80_vdc_read;
       mem_write_tab[j][0xd6] = z80_vdc_store;
       mem_read_tab[j][0xd7] = z80_c64io_d700_read;
       mem_write_tab[j][0xd7] = z80_c64io_d700_store;
       mem_read_tab[j][0xd8] = z80_colorram_read;
       mem_write_tab[j][0xd8] = z80_colorram_store;
       mem_read_tab[j][0xd9] = z80_colorram_read;
       mem_write_tab[j][0xd9] = z80_colorram_store;
       mem_read_tab[j][0xda] = z80_colorram_read;
       mem_write_tab[j][0xda] = z80_colorram_store;
       mem_read_tab[j][0xdb] = z80_colorram_read;
       mem_write_tab[j][0xdb] = z80_colorram_store;
       mem_read_tab[j][0xdc] = z80_cia1_read;
       mem_write_tab[j][0xdc] = z80_cia1_store;
       mem_read_tab[j][0xdd] = z80_cia2_read;
       mem_write_tab[j][0xdd] = z80_cia2_store;
       mem_read_tab[j][0xde] = z80_c64io_de00_read;
       mem_write_tab[j][0xde] = z80_c64io_de00_store;
       mem_read_tab[j][0xdf] = z80_c64io_df00_read;
       mem_write_tab[j][0xdf] = z80_c64io_df00_store;
    }

    for (i = 0xe0; i <= 0xfe; i++) {
        /* z80 c128 mode memory map for $e000-$feff */
        for (j = 0; j < NUM_Z80_C128MODE_CONFIGS; j++) {
            mem_read_tab[j][i] = z80mem_hi_rom_area_read;
            mem_write_tab[j][i] = z80mem_hi_rom_area_store;
        }

        /* z80 c64 mode memory map for $e000-$feff */
        mem_read_tab[8][i] = top_shared_read;
        mem_write_tab[8][i] = top_shared_store;
        mem_read_tab[9][i] = top_shared_read;
        mem_write_tab[9][i] = top_shared_store;
        mem_read_tab[10][i] = c64memrom_kernal64_read;
        mem_write_tab[10][i] = top_shared_store;
        mem_read_tab[11][i] = c64memrom_kernal64_read;
        mem_write_tab[11][i] = top_shared_store;
        mem_read_tab[12][i] = top_shared_read;
        mem_write_tab[12][i] = top_shared_store;
        mem_read_tab[13][i] = top_shared_read;
        mem_write_tab[13][i] = top_shared_store;
        mem_read_tab[14][i] = c64memrom_kernal64_read;
        mem_write_tab[14][i] = top_shared_store;
        mem_read_tab[15][i] = c64memrom_kernal64_read;
        mem_write_tab[15][i] = top_shared_store;
    }

    /* z80 c128 mode memory map for $ff00-$ffff */
    for (j = 0; j < NUM_Z80_C128MODE_CONFIGS; j++) {
        mem_read_tab[j][0xff] = mmu_ffxx_read;
        mem_write_tab[j][0xff] = mmu_ffxx_store;
    }

    /* z80 c64 mode memory map for $ff00-$ffff */
    mem_read_tab[8][0xff] = top_shared_read;
    mem_write_tab[8][0xff] = z80_c64mode_ffxx_store;
    mem_read_tab[9][0xff] = top_shared_read;
    mem_write_tab[9][0xff] = z80_c64mode_ffxx_store;
    mem_read_tab[10][0xff] = c64memrom_kernal64_read;
    mem_write_tab[10][0xff] = z80_c64mode_ffxx_store;
    mem_read_tab[11][0xff] = c64memrom_kernal64_read;
    mem_write_tab[11][0xff] = z80_c64mode_ffxx_store;
    mem_read_tab[12][0xff] = top_shared_read;
    mem_write_tab[12][0xff] = z80_c64mode_ffxx_store;
    mem_read_tab[13][0xff] = top_shared_read;
    mem_write_tab[13][0xff] = z80_c64mode_ffxx_store;
    mem_read_tab[14][0xff] = c64memrom_kernal64_read;
    mem_write_tab[14][0xff] = z80_c64mode_ffxx_store;
    mem_read_tab[15][0xff] = c64memrom_kernal64_read;
    mem_write_tab[15][0xff] = z80_c64mode_ffxx_store;

    for (j = 0; j < NUM_CONFIGS; j++) {
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

    for (i = 0; i <= 0xf; i++) {
        io_read_tab[i] = z80_io_ram_bank0_read;
        io_write_tab[i] = z80_io_ram_bank0_store;
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

    io_read_tab[0xd5] = z80_c128_mmu_read;
    io_write_tab[0xd5] = z80_c128_mmu_store;

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

static int c64mode_bit = 0;

void z80mem_update_config(int config)
{
    z80mem_config = config;

    _z80mem_read_tab_ptr = mem_read_tab[config];
    _z80mem_write_tab_ptr = mem_write_tab[config];
    _z80mem_read_base_tab_ptr = mem_read_base_tab[config];
    z80mem_read_limit_tab_ptr = mem_read_limit_tab[config];

    /* when switching to c64 mode, disable mmu i/o access */
    if (!c64mode_bit && (config & 8)) {
        io_read_tab[0xd5] = read_unconnected_io;
        io_write_tab[0xd5] = store_unconnected_io;
        c64mode_bit = 1;
    }

    /* when switching to c128 mode, enable mmu i/o access */
    if (c64mode_bit && !(config & 8)) {
        io_read_tab[0xd5] = z80_mmu_read;
        io_write_tab[0xd5] = z80_mmu_store;
        c64mode_bit = 0;
    }

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

/* "config" here is between 0 and 15 */
uint8_t z80mem_peek_with_config(int config, uint16_t addr, void *context)
{
    int j = config & 15;
    int i = addr >> 8;

    uint8_t res;
    /* save old config */
    uint8_t *old_bank = ram_bank;

    /* switch to new config */
    /* This may not be entirely correct given that the MMU has a lot of
       weird behavior when the registers are not "zero" in c64 mode.
       We will just set the memory banks for now. */
    mmu_set_ram_bank((j & 6) << 5);

    /* create a macro to simplify the restoration of the previous config */
#define result(var)        \
    res = var;           \
    ram_bank = old_bank; \
    return res;

    if (j & 8) {
        /* z80 c64 mode memory map */
        /* bottom 3 bits of config honor c46 mem_config */
        switch (i >> 4) {
            case 0x0:
                switch (i & 0x0f) {
                    case 0x00:
                        result(z80_peek_zero(addr));
                        break;
                    case 0x01:
                        result(one_peek(addr));
                        break;
                    default:
                        result(lo_peek(addr));
                        break;
                }
            case 0x1:
                switch (i & 0x0f) {
                    case 0x00:
                    case 0x01:
                    case 0x02:
                    case 0x03:
                        result(colorram_read(addr));
                        break;
                    default:
                        break;
                }
                /* FALL THROUGH */
            case 0x2:
            case 0x3:
                result(lo_peek(addr));
                break;
            case 0x4:
            case 0x5:
            case 0x6:
            case 0x7:
            case 0x8:
            case 0x9:
                result(ram_peek(addr));
            case 0xa:
            case 0xb:
                if ((j & 3) == 3) {
                    result(c64memrom_basic64_read(addr));
                } else {
                    result(ram_peek(addr));
                }
            case 0xc:
                result(top_shared_peek(addr));
            case 0xd:
                switch (j) {
                    case 8:
                    case 12:
                        result(top_shared_peek(addr));
                    case 9:
                    case 10:
                    case 11:
                        result(mem_chargen_rom_ptr[addr & 0x0fff]);
                    default:
                        switch (i & 0x0f) {
                            case 0x0:
                                result(z80_c64io_d000_peek(addr));
                            case 0x1:
                                result(z80_c64io_d100_peek(addr));
                            case 0x2:
                                result(z80_c64io_d200_peek(addr));
                            case 0x3:
                                result(z80_c64io_d300_peek(addr));
                            case 0x4:
                                result(z80_c64io_d400_peek(addr));
                            case 0x5:
                            case 0x7:
                                result(z80_c64io_d700_peek(addr));
                            case 0x6:
                                result(z80_vdc_peek(addr));
                            case 0x8:
                            case 0x9:
                            case 0xa:
                            case 0xb:
                                result(z80_colorram_peek(addr));
                            case 0xc:
                                result(z80_cia1_peek(addr));
                            case 0xd:
                                result(z80_cia2_peek(addr));
                            case 0xe:
                                result(z80_c64io_de00_peek(addr));
                            case 0xf:
                                result(z80_c64io_df00_peek(addr));
                        }
                        break;
                }
                break;
            case 0xe:
            case 0xf:
                if (j & 2) {
                    result(c64memrom_kernal64_read(addr));
                } else {
                    result(top_shared_peek(addr));
                }
        }


    } else {
        /* z80 c128 mode memory map */
        /* bit 0 of mem_config is 0 for RAM and 1 for IO */
        /* bit 1 of mem_config is A16 */
        /* bit 2 of mem_config is A17 */
        switch (i >> 4) {
            case 0x0:
                switch (i & 0x0f) {
                    case 0x00:
                        if (j & 6) {
                            result(z80_peek_zero(addr));
                        } else {
                            result(bios_read(addr));
                        }
                        break;
                    case 0x01:
                        if (j & 6) {
                            result(one_peek(addr));
                        } else {
                            result(bios_read(addr));
                        }
                        break;
                    default:
                        if (j & 6) {
                            result(lo_peek(addr));
                        } else {
                            result(bios_read(addr));
                        }
                        break;
                }
            case 0x1:
                switch (i & 0x0f) {
                    case 0x00:
                    case 0x01:
                    case 0x02:
                    case 0x03:
                        if (j & 1) {
                            result(colorram_read(addr));
                        } else if (j & 6) {
                            result(lo_peek(addr));
                        } else {
                            result(ram_peek(addr));
                        }
                        break;
                    default:
                        break;
                }
                /* FALL THROUGH */
            case 0x2:
            case 0x3:
                if (j & 6) {
                    result(lo_peek(addr));
                } else {
                    result(ram_peek(addr));
                }
                break;
            case 0x4:
            case 0x5:
            case 0x6:
            case 0x7:
                result(z80mem_lo_rom_area_peek(addr));
            case 0x8:
            case 0x9:
            case 0xa:
            case 0xb:
                result(z80mem_mid_rom_area_peek(addr));
            case 0xc:
                result(z80mem_editor_rom_area_peek(addr));
            case 0xd:
                result(z80mem_chargen_rom_area_peek(addr));
            case 0xe:
            case 0xf:
                if (addr >= 0xff00 && addr <= 0xff04) {
                    result(mmu[addr & 0xf]);
                } else {
                    result(z80mem_hi_rom_area_peek(addr));
                }
        }
    }

    result(0);
}

