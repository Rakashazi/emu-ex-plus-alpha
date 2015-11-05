/*
 * cbm5x0mem.c - CBM-5x0 memory handling.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#include "archdep.h"
#include "cbm2cart.h"
#include "cartridge.h"
#include "cbm2-resources.h"
#include "cbm2.h"
#include "cbm2acia.h"
#include "cbm2cia.h"
#include "cbm2mem.h"
#include "cbm2model.h"
#include "cbm2tpi.h"
#include "cia.h"
#include "kbdbuf.h"
#include "machine.h"
#include "mem.h"
#include "maincpu.h"
#include "monitor.h"
#include "ram.h"
#include "resources.h"
#include "sid.h"
#include "sid-resources.h"
#include "tpi.h"
#include "types.h"
#include "vsync.h"
#include "vicii-mem.h"
#include "vicii-phi1.h"
#include "vicii.h"

void cia1_set_extended_keyboard_rows_mask(BYTE foo)
{
}

/* ------------------------------------------------------------------------- */
/* The CBM-II memory. */

BYTE mem_ram[CBM2_RAM_SIZE];            /* 1M, banks 0-14 plus extension RAM
                                           in bank 15 */
BYTE mem_rom[CBM2_ROM_SIZE];            /* complete bank 15 ROM + video RAM */
BYTE mem_chargen_rom[CBM2_CHARGEN_ROM_SIZE];

/* Internal color memory.  */
static BYTE mem_color_ram[0x400];
BYTE *mem_color_ram_cpu, *mem_color_ram_vicii;

/* Pointer to the chargen ROM.  */
BYTE *mem_chargen_rom_ptr;

BYTE *mem_page_zero;
BYTE *mem_page_one;

/* selected banks for normal access and indirect accesses */
int cbm2mem_bank_exec = -1;
int cbm2mem_bank_ind = -1;

/* Memory read and write tables - banked. */
static read_func_ptr_t _mem_read_tab[16][0x101];
static store_func_ptr_t _mem_write_tab[16][0x101];
static BYTE *_mem_read_base_tab[16][0x101];
static int mem_read_limit_tab[3][0x101];

/* watch tables are fixed */
static read_func_ptr_t _mem_read_tab_watch[0x101];
static read_func_ptr_t _mem_read_ind_tab_watch[0x101];
static store_func_ptr_t _mem_write_tab_watch[0x101];
static store_func_ptr_t _mem_write_ind_tab_watch[0x101];

read_func_ptr_t *_mem_read_tab_ptr;
read_func_ptr_t *_mem_read_ind_tab_ptr;
store_func_ptr_t *_mem_write_tab_ptr;
store_func_ptr_t *_mem_write_ind_tab_ptr;
static BYTE **_mem_read_base_tab_ptr;
static int *mem_read_limit_tab_ptr;

int cbm2_init_ok = 0;

/* ------------------------------------------------------------------------- */

/* state of tpi pc6/7 */
static int c500_vbank = 0;

/* 1= static video matrix RAM (phi2); 0= bank 0 */
static int c500_statvid = 1;

/* 1= character ROM in bank 15 (phi1); 0= bank 0 */
static int c500_vicdotsel = 1;

void c500_set_phi2_bank(int b)
{
    if (b == c500_statvid) {
        return;
    }

    if (b) {    /* bank 15 */
        /* video memory at $c000/d000 depending on d818 */
        vicii_set_phi2_addr_options(0x13ff, 0xc000);
        /* no chargen mapping */
        vicii_set_phi2_chargen_addr_options(0, 1);
        /* memory mapping */
        vicii_set_phi2_vbank(3);       /* necessary? */
        vicii_set_phi2_ram_base(mem_rom);
    } else {
        /* video memory in bank 0 */
        vicii_set_phi2_addr_options(0xffff, 0x0000);
        /* no chargen mapping */
        vicii_set_phi2_chargen_addr_options(0, 1);
        /* memory mapping */
        vicii_set_phi2_vbank(c500_vbank);
        vicii_set_phi2_ram_base(mem_ram);
    }

    c500_statvid = b;
}

void c500_set_phi1_bank(int b)
{
    if (b == c500_vicdotsel) {
        return;
    }

    if (b) {    /* bank 15 */
        /* video memory at $c000/c800 depending on d818 */
        vicii_set_phi1_addr_options(0x0fff, 0xc000);
        /* no chargen mapping */
        vicii_set_phi1_chargen_addr_options(0xc000, 0xc000);
        /* memory mapping */
        vicii_set_phi1_vbank(3);       /* necessary? */
        vicii_set_phi1_ram_base(mem_rom);
    } else {
        /* video memory in bank 0 */
        vicii_set_phi1_addr_options(0xffff, 0x0000);
        /* no chargen mapping */
        vicii_set_phi1_chargen_addr_options(0, 1);
        /* memory mapping */
        vicii_set_phi1_vbank(c500_vbank);
        vicii_set_phi1_ram_base(mem_ram);
    }

    c500_vicdotsel = b;
}

void cbm2_set_tpi2pc(BYTE b)
{
    int vbank = (b & 0xc0) >> 6;
    c500_vbank = vbank;

    if (!c500_vicdotsel) {
        vicii_set_phi1_vbank(vbank);
    }
    if (!c500_statvid) {
        vicii_set_phi2_vbank(vbank);
    }
}

void cbm2_set_tpi1ca(int a)
{
    c500_set_phi2_bank(a);
}

void cbm2_set_tpi1cb(int a)
{
    c500_set_phi1_bank(a);
}

/* ------------------------------------------------------------------------- */

void cbm2mem_set_bank_exec(int val)
{
    int i;

    val &= 0x0f;
    if (val != cbm2mem_bank_exec) {
        cbm2mem_bank_exec = val;

        _mem_read_tab_ptr = _mem_read_tab[cbm2mem_bank_exec];
        _mem_write_tab_ptr = _mem_write_tab[cbm2mem_bank_exec];
        _mem_read_base_tab_ptr = _mem_read_base_tab[cbm2mem_bank_exec];
        mem_read_limit_tab_ptr = mem_read_limit_tab[(cbm2mem_bank_exec < 15)
                                                    ? 0 : 1];
        if (!_mem_read_base_tab_ptr[0]) {
            /* disable fast opcode fetch when bank_base is null, i.e.
               set all limits to 0 when no RAM available.
               This might also happen when jumping to open mem in
               bank 15, though. */
            mem_read_limit_tab_ptr = mem_read_limit_tab[2];
        }

        maincpu_resync_limits();

        /* set all register mirror locations */
        for (i = 0; i < 16; i++) {
            mem_ram[i << 16] = val;
        }

        mem_page_zero = _mem_read_base_tab_ptr[0];
        mem_page_one = _mem_read_base_tab_ptr[1];

        /* This sets the pointers to otherwise non-mapped memory, to
           avoid that the CPU code uses illegal memory and segfaults. */
        if (!mem_page_zero) {
            mem_page_zero = mem_ram + 0xf0000;
        }
        if (!mem_page_one) {
            mem_page_one = mem_ram + 0xf0100;
        }
    }
}

void cbm2mem_set_bank_ind(int val)
{
    int i;
    val &= 0x0f;

    if (val != cbm2mem_bank_ind) {
        cbm2mem_bank_ind = val;
        _mem_read_ind_tab_ptr = _mem_read_tab[cbm2mem_bank_ind];
        _mem_write_ind_tab_ptr = _mem_write_tab[cbm2mem_bank_ind];
        /* set all register mirror locations */
        for (i = 0; i < 16; i++) {
            mem_ram[(i << 16) + 1] = val;
        }
    }
}

/* ------------------------------------------------------------------------- */

void zero_store(WORD addr, BYTE value)
{
    if (addr == 0) {
        cbm2mem_set_bank_exec(value);
    } else
    if (addr == 1) {
        cbm2mem_set_bank_ind(value);
    }

    _mem_write_tab_ptr[0]((WORD)(addr & 0xff), value);
}

#define STORE_ZERO(bank)                                 \
    static void store_zero_##bank(WORD addr, BYTE value) \
    {                                                    \
        addr &= 0xff;                                    \
                                                         \
        if (addr == 0) {                                 \
            cbm2mem_set_bank_exec(value);                \
        } else if (addr == 1) {                          \
            cbm2mem_set_bank_ind(value);                 \
        }                                                \
                                                         \
        mem_ram[(0x##bank << 16) | addr] = value;        \
    }

#define READ_ZERO(bank)                                   \
    static BYTE read_zero_##bank(WORD addr)               \
    {                                                     \
        return mem_ram[(0x##bank << 16) | (addr & 0xff)]; \
    }

#define READ_RAM(bank)                           \
    static BYTE read_ram_##bank(WORD addr)       \
    {                                            \
        return mem_ram[(0x##bank << 16) | addr]; \
    }

#define STORE_RAM(bank)                                \
    static void store_ram_##bank(WORD addr, BYTE byte) \
    {                                                  \
        mem_ram[(0x##bank << 16) | addr] = byte;       \
    }

STORE_ZERO(0)
STORE_ZERO(1)
STORE_ZERO(2)
STORE_ZERO(3)
STORE_ZERO(4)
STORE_ZERO(5)
STORE_ZERO(6)
STORE_ZERO(7)
STORE_ZERO(8)
STORE_ZERO(9)
STORE_ZERO(A)
STORE_ZERO(B)
STORE_ZERO(C)
STORE_ZERO(D)
STORE_ZERO(E)
STORE_ZERO(F)

READ_ZERO(0)
READ_ZERO(1)
READ_ZERO(2)
READ_ZERO(3)
READ_ZERO(4)
READ_ZERO(5)
READ_ZERO(6)
READ_ZERO(7)
READ_ZERO(8)
READ_ZERO(9)
READ_ZERO(A)
READ_ZERO(B)
READ_ZERO(C)
READ_ZERO(D)
READ_ZERO(E)
READ_ZERO(F)

STORE_RAM(0)
STORE_RAM(1)
STORE_RAM(2)
STORE_RAM(3)
STORE_RAM(4)
STORE_RAM(5)
STORE_RAM(6)
STORE_RAM(7)
STORE_RAM(8)
STORE_RAM(9)
STORE_RAM(A)
STORE_RAM(B)
STORE_RAM(C)
STORE_RAM(D)
STORE_RAM(E)
STORE_RAM(F)

READ_RAM(0)
READ_RAM(1)
READ_RAM(2)
READ_RAM(3)
READ_RAM(4)
READ_RAM(5)
READ_RAM(6)
READ_RAM(7)
READ_RAM(8)
READ_RAM(9)
READ_RAM(A)
READ_RAM(B)
READ_RAM(C)
READ_RAM(D)
READ_RAM(E)
READ_RAM(F)

static store_func_ptr_t store_zero_tab[16] = {
    store_zero_0, store_zero_1, store_zero_2, store_zero_3,
    store_zero_4, store_zero_5, store_zero_6, store_zero_7,
    store_zero_8, store_zero_9, store_zero_A, store_zero_B,
    store_zero_C, store_zero_D, store_zero_E, store_zero_F
};

static store_func_ptr_t store_ram_tab[16] = {
    store_ram_0, store_ram_1, store_ram_2, store_ram_3,
    store_ram_4, store_ram_5, store_ram_6, store_ram_7,
    store_ram_8, store_ram_9, store_ram_A, store_ram_B,
    store_ram_C, store_ram_D, store_ram_E, store_ram_F
};

static read_func_ptr_t read_ram_tab[16] = {
    read_ram_0, read_ram_1, read_ram_2, read_ram_3,
    read_ram_4, read_ram_5, read_ram_6, read_ram_7,
    read_ram_8, read_ram_9, read_ram_A, read_ram_B,
    read_ram_C, read_ram_D, read_ram_E, read_ram_F
};

static read_func_ptr_t read_zero_tab[16] = {
    read_zero_0, read_zero_1, read_zero_2, read_zero_3,
    read_zero_4, read_zero_5, read_zero_6, read_zero_7,
    read_zero_8, read_zero_9, read_zero_A, read_zero_B,
    read_zero_C, read_zero_D, read_zero_E, read_zero_F
};


void store_zeroX(WORD addr, BYTE value)
{
    if (addr == 0) {
        cbm2mem_set_bank_exec(value);
    } else if (addr == 1) {
        cbm2mem_set_bank_ind(value);
    }
}

BYTE rom_read(WORD addr)
{
    return mem_rom[addr];
}

BYTE read_chargen(WORD addr)
{
    return mem_chargen_rom[addr & 0xfff];
}

void rom_store(WORD addr, BYTE value)
{
    mem_rom[addr] = value;
}

static BYTE read_unused(WORD addr)
{
    return 0xff; /* (addr >> 8) & 0xff; */
}

static void store_dummy(WORD addr, BYTE value)
{
    return;
}

/* ------------------------------------------------------------------------- */

/* Functions for watchpoint memory access.  */

static BYTE zero_read_watch(WORD addr)
{
    addr &= 0xff;
    monitor_watch_push_load_addr(addr, e_comp_space);
    return _mem_read_tab[cbm2mem_bank_exec][0](addr);
}

static void zero_store_watch(WORD addr, BYTE value)
{
    addr &= 0xff;
    monitor_watch_push_store_addr(addr, e_comp_space);
    _mem_write_tab[cbm2mem_bank_exec][0](addr, value);
}

BYTE read_watch(WORD addr)
{
    monitor_watch_push_load_addr(addr, e_comp_space);
    return _mem_read_tab[cbm2mem_bank_exec][addr >> 8](addr);
}

void store_watch(WORD addr, BYTE value)
{
    monitor_watch_push_store_addr(addr, e_comp_space);
    _mem_write_tab[cbm2mem_bank_exec][addr >> 8](addr, value);
}

BYTE read_ind_watch(WORD addr)
{
    monitor_watch_push_load_addr(addr, e_comp_space);
    return _mem_read_tab[cbm2mem_bank_ind][addr >> 8](addr);
}

void store_ind_watch(WORD addr, BYTE value)
{
    monitor_watch_push_store_addr(addr, e_comp_space);
    _mem_write_tab[cbm2mem_bank_ind][addr >> 8](addr, value);
}

/* ------------------------------------------------------------------------- */

/* Generic memory access.  */

void mem_store(WORD addr, BYTE value)
{
    _mem_write_tab_ptr[addr >> 8](addr, value);
}

BYTE mem_read(WORD addr)
{
    return _mem_read_tab_ptr[addr >> 8](addr);
}

/* ------------------------------------------------------------------------- */

void store_io(WORD addr, BYTE value)
{
    switch (addr & 0xf800) {
        case 0xd000:
            rom_store(addr, value);     /* video RAM mapped here... */
            if (addr >= 0xd400) {
                colorram_store(addr, value);
            }
            return;
        case 0xd800:
            switch (addr & 0xff00) {
                case 0xd800:
                    vicii_store(addr, value);
                    return;
                case 0xd900:
                    return;             /* disk units */
                case 0xda00:
                    sid_store((WORD)(addr & 0xff), value);
                    return;
                case 0xdb00:
                    return;             /* coprocessor */
                case 0xdc00:
                    cia1_store((WORD)(addr & 0x0f), value);
                    return;
                case 0xdd00:
                    acia1_store((WORD)(addr & 0x03), value);
                    return;
                case 0xde00:
                    tpi1_store((WORD)(addr & 0x07), value);
                    return;
                case 0xdf00:
                    tpi2_store((WORD)(addr & 0x07), value);
                    return;
            }
    }
}

BYTE read_io(WORD addr)
{
    switch (addr & 0xf800) {
        case 0xd000:
            return rom_read(addr);
        case 0xd800:
            switch (addr & 0xff00) {
                case 0xd800:
                    return vicii_read(addr);
                case 0xd900:
                    return read_unused(addr);
                case 0xda00:
                    return sid_read(addr);
                case 0xdb00:
                    return read_unused(addr);
                case 0xdc00:
                    return cia1_read((WORD)(addr & 0x0f));
                case 0xdd00:
                    return acia1_read((WORD)(addr & 0x03));
                case 0xde00:
                    /* FIXME: VIC-II irq? */
                    /* if ((machine_class == VICE_MACHINE_CBM5x0) && ((addr & 7) == 2)) {
                           return tpi1_read(addr&7)|1; }   */
                    return tpi1_read((WORD)(addr & 0x07));
                case 0xdf00:
                    return tpi2_read((WORD)(addr & 0x07));
            }
    }
    return read_unused(addr);
}


/* FIXME: TODO! */
void mem_toggle_watchpoints(int flag, void *context)
{
    if (flag) {
        _mem_read_tab_ptr = _mem_read_tab_watch;
        _mem_read_ind_tab_ptr = _mem_read_ind_tab_watch;
        _mem_write_tab_ptr = _mem_write_tab_watch;
        _mem_write_ind_tab_ptr = _mem_write_ind_tab_watch;
    } else {
        cbm2mem_set_bank_exec(cbm2mem_bank_exec);
        cbm2mem_set_bank_ind(cbm2mem_bank_ind);
    }
}

/* ------------------------------------------------------------------------- */
/* handle CPU reset */

void mem_reset(void)
{
    cbm2mem_set_bank_exec(15);
    cbm2mem_set_bank_ind(15);

    c500_set_phi1_bank(15);
    c500_set_phi2_bank(15);
}

/* ------------------------------------------------------------------------- */

void colorram_store(WORD addr, BYTE value)
{
    mem_color_ram[addr & 0x3ff] = value & 0xf;
}

BYTE colorram_read(WORD addr)
{
    return mem_color_ram[addr & 0x3ff] | (vicii_read_phi1() & 0xf0);
}

/* ------------------------------------------------------------------------- */

void mem_initialize_memory(void)
{
    int i;

    mem_chargen_rom_ptr = mem_chargen_rom;
    mem_color_ram_cpu = mem_color_ram;
    mem_color_ram_vicii = mem_color_ram;

    /* first the tables that hold the predefined bank mappings */
    for (i = 0; i < 16; i++) {          /* 16 banks possible */
        mem_initialize_memory_bank(i);
    }

    /* set bank limit tables for optimized opcode fetch */
    for (i = 256; i >= 0; i--) {
        mem_read_limit_tab[0][i] = 0xfffd;      /* all RAM banks go here */
        mem_read_limit_tab[2][i] = 0;           /* all empty banks go here */

        if (!_mem_read_base_tab[15][i]) {
            mem_read_limit_tab[1][i] = 0;
        } else
        if (i < 0x08) { /* system RAM */
            mem_read_limit_tab[1][i] = 0x07fd;
        } else
        if (i < 0x10) { /* ROM/RAM 0800-0FFF */
            mem_read_limit_tab[1][i] = 0x0ffd;
        } else
        if (i < 0x20) { /* ROM/RAM 1000-1FFF */
            mem_read_limit_tab[1][i] = 0x1ffd;
        } else
        if (i < 0x40) { /* ROM/RAM 2000-3FFF */
            mem_read_limit_tab[1][i] = 0x3ffd;
        } else
        if (i < 0x60) { /* ROM/RAM 4000-5FFF */
            mem_read_limit_tab[1][i] = 0x5ffd;
        } else
        if (i < 0x80) { /* ROM/RAM 6000-7FFF */
            mem_read_limit_tab[1][i] = 0x7ffd;
        } else
        if (i < 0xc0) { /* ROM 8000-BFFF */
            mem_read_limit_tab[1][i] = 0xbffd;
        } else
        if (i < 0xd0) {  /* C000-CFFF */
            mem_read_limit_tab[1][i] = 0xcffd;
        } else
        if (i < 0xe0) { /* I/O D000-DFFF */
            mem_read_limit_tab[1][i] = 0;
        } else {        /* ROM E000-FFFF */
            mem_read_limit_tab[1][i] = 0xfffd;
        }
    }

    /* set watchpoint tables */
    for (i = 0; i <= 0x100; i++) {
        _mem_read_tab_watch[i] = read_watch;
        _mem_read_ind_tab_watch[i] = read_ind_watch;
        _mem_write_tab_watch[i] = store_watch;
        _mem_write_ind_tab_watch[i] = store_ind_watch;
    }
    /* FIXME: what about _ind_tab_watch ? */
    _mem_read_tab_watch[0] = zero_read_watch;
    _mem_write_tab_watch[0] = zero_store_watch;

    vicii_set_chargen_addr_options(0x7000, 0x1000);
}

void mem_initialize_memory_bank(int i)
{
    int j;

    switch (i) {
        case 0:
            for (j = 255; j >= 0; j--) {
                _mem_read_tab[i][j] = read_ram_tab[i];
                _mem_write_tab[i][j] = store_ram_tab[i];
                _mem_read_base_tab[i][j] = mem_ram + (i << 16) + (j << 8);
            }
            _mem_write_tab[i][0] = store_zero_tab[i];
            _mem_read_tab[i][0] = read_zero_tab[i];
            break;
        case 1:
            for (j = 255; j >= 0; j--) {
                _mem_read_tab[i][j] = read_ram_tab[i];
                _mem_write_tab[i][j] = store_ram_tab[i];
                _mem_read_base_tab[i][j] = mem_ram + (i << 16) + (j << 8);
            }
            _mem_write_tab[i][0] = store_zero_tab[i];
            _mem_read_tab[i][0] = read_zero_tab[i];
            break;
        case 2:
            if (ramsize >= 128) {
                for (j = 255; j >= 0; j--) {
                    _mem_read_tab[i][j] = read_ram_tab[i];
                    _mem_write_tab[i][j] = store_ram_tab[i];
                    _mem_read_base_tab[i][j] = mem_ram + (i << 16) + (j << 8);
                }
                _mem_write_tab[i][0] = store_zero_tab[i];
                _mem_read_tab[i][0] = read_zero_tab[i];
                break;
            }
        case 3:
        case 4:
            if (ramsize >= 256) {
                for (j = 255; j >= 0; j--) {
                    _mem_read_tab[i][j] = read_ram_tab[i];
                    _mem_write_tab[i][j] = store_ram_tab[i];
                    _mem_read_base_tab[i][j] = mem_ram + (i << 16) + (j << 8);
                }
                _mem_write_tab[i][0] = store_zero_tab[i];
                _mem_read_tab[i][0] = read_zero_tab[i];
                break;
            }
        case 5:
        case 6:
        case 7:
            if (ramsize >= 512) {
                for (j = 255; j >= 0; j--) {
                    _mem_read_tab[i][j] = read_ram_tab[i];
                    _mem_write_tab[i][j] = store_ram_tab[i];
                    _mem_read_base_tab[i][j] = mem_ram + (i << 16) + (j << 8);
                }
                _mem_write_tab[i][0] = store_zero_tab[i];
                _mem_read_tab[i][0] = read_zero_tab[i];
                break;
            }
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
            if (ramsize >= 1024) {
                for (j = 255; j >= 0; j--) {
                    _mem_read_tab[i][j] = read_ram_tab[i];
                    _mem_write_tab[i][j] = store_ram_tab[i];
                    _mem_read_base_tab[i][j] = mem_ram + (i << 16) + (j << 8);
                }
                _mem_write_tab[i][0] = store_zero_tab[i];
                _mem_read_tab[i][0] = read_zero_tab[i];
                break;
            }
            /* fallback for ramsize < some_value */
            for (j = 255; j >= 0; j--) {
                _mem_read_tab[i][j] = read_unused;
                _mem_write_tab[i][j] = store_dummy;
                _mem_read_base_tab[i][j] = NULL;
            }
            _mem_write_tab[i][0] = store_zeroX;
            break;
        case 15:
            for (j = 0; j < 0x08; j++) {
                _mem_read_tab[i][j] = read_ram_F;
                _mem_write_tab[i][j] = store_ram_F;
                _mem_read_base_tab[i][j] = mem_ram + (i << 16) + (j << 8);
            }
            for (; j < 0xc0; j++) { /* 0800-BFFF */
                _mem_read_tab[i][j] = rom_read;
                _mem_write_tab[i][j] = store_dummy;
                _mem_read_base_tab[i][j] = mem_rom + (j << 8);
            }
            for (; j < 0xd0; j++) { /* C000-CFFF */
                _mem_read_tab[i][j] = read_chargen;
                _mem_write_tab[i][j] = store_dummy;
                _mem_read_base_tab[i][j] = mem_chargen_rom + ((j << 8) & 0x0f);
            }
            for (; j < 0xe0; j++) { /* D000-DFFF */
                _mem_read_tab[i][j] = read_io;
                _mem_write_tab[i][j] = store_io;
                _mem_read_base_tab[i][j] = NULL;
            }
            for (; j < 0x100; j++) {
                _mem_read_tab[i][j] = rom_read;
                _mem_write_tab[i][j] = store_dummy;
                _mem_read_base_tab[i][j] = mem_rom + (j << 8);
            }

            if (cart08_ram) {
                for (j = 0x08; j < 0x10; j++) {
                    _mem_read_tab[i][j] = read_ram_F;
                    _mem_write_tab[i][j] = store_ram_F;
                    _mem_read_base_tab[i][j] = mem_ram + (i << 16) + (j << 8);
                }
            }
            if (cart1_ram) {
                for (j = 0x10; j < 0x20; j++) {
                    _mem_read_tab[i][j] = read_ram_F;
                    _mem_write_tab[i][j] = store_ram_F;
                    _mem_read_base_tab[i][j] = mem_ram + (i << 16) + (j << 8);
                }
            }
            if (cart2_ram) {
                for (j = 0x20; j < 0x40; j++) {
                    _mem_read_tab[i][j] = read_ram_F;
                    _mem_write_tab[i][j] = store_ram_F;
                    _mem_read_base_tab[i][j] = mem_ram + (i << 16) + (j << 8);
                }
            }
            if (cart4_ram) {
                for (j = 0x40; j < 0x60; j++) {
                    _mem_read_tab[i][j] = read_ram_F;
                    _mem_write_tab[i][j] = store_ram_F;
                    _mem_read_base_tab[i][j] = mem_ram + (i << 16) + (j << 8);
                }
            }
            if (cart6_ram) {
                for (j = 0x60; j < 0x80; j++) {
                    _mem_read_tab[i][j] = read_ram_F;
                    _mem_write_tab[i][j] = store_ram_F;
                    _mem_read_base_tab[i][j] = mem_ram + (i << 16) + (j << 8);
                }
            }
            if (cartC_ram) {
                for (j = 0xc0; j < 0xd0; j++) {
                    _mem_read_tab[i][j] = read_ram_F;
                    _mem_write_tab[i][j] = store_ram_F;
                    _mem_read_base_tab[i][j] = mem_ram + (i << 16) + (j << 8);
                }
            }

            _mem_write_tab[i][0] = store_zero_F;
            _mem_read_tab[i][0] = read_zero_F;
            _mem_read_base_tab[i][0] = mem_ram + 0xf0000;
            break;
    }
    _mem_read_tab[i][0x100] = _mem_read_tab[i][0];
    _mem_write_tab[i][0x100] = _mem_write_tab[i][0];
    _mem_read_base_tab[i][0x100] = _mem_read_base_tab[i][0];
}

void mem_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
{
    BYTE *p = _mem_read_base_tab_ptr[addr >> 8];

    *base = (p == NULL) ? NULL : (p - (addr & 0xff00));
    *start = addr; /* TODO */
    *limit = mem_read_limit_tab_ptr[addr >> 8];
}

void mem_powerup(void)
{
    int i;

    ram_init(mem_ram, CBM2_RAM_SIZE);

    for (i = 0; i < 0x800; i += 0x80) {
        memset(mem_rom + i, 0, 0x40);
        memset(mem_rom + i + 0x40, 0xff, 0x40);
        memset(mem_rom + 0x800 + i, 0, 0x40);
        memset(mem_rom + 0x800 + i + 0x40, 0xff, 0x40);
        memset(mem_rom + 0xd000 + i, 0, 0x40);
        memset(mem_rom + 0xd000 + i + 0x40, 0xff, 0x40);
    }

    cbm2mem_bank_exec = 0;
    cbm2mem_bank_ind = 0;
    cbm2mem_set_bank_exec(15);
    cbm2mem_set_bank_ind(15);
}

/* ------------------------------------------------------------------------- */

/* FIXME: To do!  */

void mem_get_basic_text(WORD *start, WORD *end)
{
}

void mem_set_basic_text(WORD start, WORD end)
{
}

void mem_inject(DWORD addr, BYTE value)
{
    /* just call mem_store() to be safe.
       This could possibly be changed to write straight into the
       memory array.  mem_ram[addr & mask] = value; */
    mem_store((WORD)(addr & 0xffff), value);
}

/* ------------------------------------------------------------------------- */

int mem_rom_trap_allowed(WORD addr)
{
    return 1;   /* (addr >= 0xf000) && !(map_reg & 0x80); */
}

void mem_set_tape_sense(int value)
{
}

/* ------------------------------------------------------------------------- */

/* Banked memory access functions for the monitor.  */

static BYTE peek_bank_io(WORD addr)
{
    switch (addr & 0xf800) {
        case 0xc000:
        case 0xc800:
            return read_unused(addr);
        case 0xd000:
            return rom_read(addr);
        case 0xd800:
            switch (addr & 0xff00) {
                case 0xd800:
                    return vicii_peek(addr);
                case 0xd900:
                    return read_unused(addr);
                case 0xda00:
                    return sid_read(addr);
                case 0xdb00:
                    return read_unused(addr);
                case 0xdc00:
                    return cia1_peek(addr);
                case 0xdd00:
                    return acia1_peek(addr);
                case 0xde00:
                    return tpi1_peek((WORD)(addr & 0x07));
                case 0xdf00:
                    return tpi2_peek((WORD)(addr & 0x07));
            }
    }
    return read_unused(addr);
}

/* Exported banked memory access functions for the monitor.  */

static const char *banknames[] = {
    "default", "cpu", "ram0", "ram1", "ram2", "ram3",
    "ram4", "ram5", "ram6", "ram7", "ram8", "ram9",
    "ramA", "ramB", "ramC", "ramD", "ramE", "ramF",
    "romio", "io", NULL
};

static const int banknums[] = {
    17, 17, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 16
};

const char **mem_bank_list(void)
{
    return banknames;
}

int mem_bank_from_name(const char *name)
{
    int i = 0;

    while (banknames[i]) {
        if (!strcmp(name, banknames[i])) {
            return banknums[i];
        }
        i++;
    }
    return -1;
}

BYTE mem_bank_read(int bank, WORD addr, void *context)
{
    switch (bank) {
        case 17:                /* current */
            return mem_read(addr);
        case 16:                 /* romio */
            if (addr >= 0xd000 && addr < 0xe000) {
                return read_io(addr);
            }
            return _mem_read_tab[15][addr >> 8](addr);
        default:
            if (bank >= 0 && bank < 15) {
                return read_ram_tab[bank](addr);
            }
    }
    return read_unused(addr);
}

BYTE mem_bank_peek(int bank, WORD addr, void *context)
{
    if (bank == 16) {
        if (addr >= 0xc000 && addr < 0xe000) {
            return peek_bank_io(addr);
        }
    }
    return mem_bank_read(bank, addr, context);
}

void mem_bank_write(int bank, WORD addr, BYTE byte, void *context)
{
    switch (bank) {
        case 17:                 /* current */
            mem_store(addr, byte);
            return;
        case 16:
            if (addr >= 0xd000 && addr <= 0xdfff) {
                store_io(addr, byte);
                return;
            }
            _mem_write_tab[15][addr >> 8](addr, byte);
            return;
        default:
            if (bank >= 0 && bank < 16) {
                if (addr & 0xff00) {
                    store_ram_tab[bank](addr, byte);
                } else {
                    store_zero_tab[bank](addr, byte);
                }
                return;
            }
    }
    store_dummy(addr, byte);
}

static int mem_dump_io(WORD addr)
{
    if ((addr >= 0xd800) && (addr <= 0xd82e)) {
        return vicii_dump();
    } else if ((addr >= 0xda00) && (addr <= 0xda1f)) {
        /* return sidcore_dump(machine_context.sid); */ /* FIXME */
    } else if ((addr >= 0xdc00) && (addr <= 0xdc0f)) {
        return ciacore_dump(machine_context.cia1);
    } else if ((addr >= 0xdd00) && (addr <= 0xdd03)) {
        /* return acia_dump(machine_context.acia); */ /* FIXME */
    } else if ((addr >= 0xde00) && (addr <= 0xde07)) {
        return tpicore_dump(machine_context.tpi1);
    } else if ((addr >= 0xdf00) && (addr <= 0xdf07)) {
        return tpicore_dump(machine_context.tpi2);
    }
    return -1;
}

mem_ioreg_list_t *mem_ioreg_list_get(void *context)
{
    mem_ioreg_list_t *mem_ioreg_list = NULL;

    mon_ioreg_add_list(&mem_ioreg_list, "VIC-II", 0xd800, 0xd82e, mem_dump_io);
    mon_ioreg_add_list(&mem_ioreg_list, "SID", 0xda00, 0xda1f, mem_dump_io);
    mon_ioreg_add_list(&mem_ioreg_list, "CIA1", 0xdc00, 0xdc0f, mem_dump_io);
    mon_ioreg_add_list(&mem_ioreg_list, "ACIA1", 0xdd00, 0xdd03, mem_dump_io);
    mon_ioreg_add_list(&mem_ioreg_list, "TPI1", 0xde00, 0xde07, mem_dump_io);
    mon_ioreg_add_list(&mem_ioreg_list, "TPI2", 0xdf00, 0xdf07, mem_dump_io);

    return mem_ioreg_list;
}

void mem_get_screen_parameter(WORD *base, BYTE *rows, BYTE *columns, int *bank)
{
    *base = 0xd000;
    *rows = 25;
    *columns = 40;
    *bank = 16;
}

void mem_color_ram_to_snapshot(BYTE *color_ram)
{
    memcpy(color_ram, mem_color_ram, 0x400);
}

void mem_color_ram_from_snapshot(BYTE *color_ram)
{
    memcpy(mem_color_ram, color_ram, 0x400);
}
