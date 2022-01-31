/*
 * cbm2mem.c - CBM-6x0/7x0 memory handling.
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
#include "cartio.h"
#include "cartridge.h"
#include "cbm2-resources.h"
#include "cbm2.h"
#include "cbm2acia.h"
#include "cbm2cart.h"
#include "cbm2cia.h"
#include "cbm2mem.h"
#include "cbm2model.h"
#include "cbm2tpi.h"
#include "cia.h"
#include "crtc-mem.h"
#include "crtc.h"
#include "crtctypes.h"
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

void cia1_set_extended_keyboard_rows_mask(uint8_t foo)
{
}

/* ------------------------------------------------------------------------- */
/* The CBM-II memory. */

uint8_t mem_ram[CBM2_RAM_SIZE];            /* 1M, banks 0-14 plus extension RAM
                                           in bank 15 */
uint8_t mem_rom[CBM2_ROM_SIZE];            /* complete bank 15 ROM + video RAM */
uint8_t mem_chargen_rom[CBM2_CHARGEN_ROM_SIZE];

/* Pointer to the chargen ROM.  */
uint8_t *mem_chargen_rom_ptr;

uint8_t *mem_page_zero;
uint8_t *mem_page_one;

/* selected banks for normal access and indirect accesses */
int cbm2mem_bank_exec = -1;
int cbm2mem_bank_ind = -1;

/* Memory read and write tables - banked. */
static read_func_ptr_t _mem_read_tab[16][0x101];
static store_func_ptr_t _mem_write_tab[16][0x101];
static uint8_t *_mem_read_base_tab[16][0x101];
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
read_func_ptr_t *_mem_read_tab_ptr_dummy;
store_func_ptr_t *_mem_write_tab_ptr_dummy;
read_func_ptr_t *_mem_read_ind_tab_ptr_dummy;
store_func_ptr_t *_mem_write_ind_tab_ptr_dummy;

static uint8_t **_mem_read_base_tab_ptr;
static int *mem_read_limit_tab_ptr;

int cbm2_init_ok = 0;

/* Current watchpoint state. 
          0 = no watchpoints
    bit0; 1 = watchpoints active
    bit1; 2 = watchpoints trigger on dummy accesses
*/
static int watchpoints_active = 0;

/* ------------------------------------------------------------------------- */

void cbm2_set_tpi2pc(uint8_t b)
{
}

void cbm2_set_tpi1ca(int a)
{
    crtc_set_chargen_offset((a) ? 256 : 0);
}

void cbm2_set_tpi1cb(int a)
{
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
        _mem_read_tab_ptr_dummy = _mem_read_tab[cbm2mem_bank_exec];
        _mem_write_tab_ptr_dummy = _mem_write_tab[cbm2mem_bank_exec];
        
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
uint8_t zero_read(uint16_t addr)
{
    addr &= 0xff;

    switch ((uint8_t)addr) {
        case 0:
            return cbm2mem_bank_exec;
        case 1:
            return cbm2mem_bank_ind;
    }

    return mem_page_zero[addr & 0xff];
}

void zero_store(uint16_t addr, uint8_t value)
{
    if (addr == 0) {
        cbm2mem_set_bank_exec(value);
    } else
    if (addr == 1) {
        cbm2mem_set_bank_ind(value);
    }

    _mem_write_tab_ptr[0]((uint16_t)(addr & 0xff), value);
}

#define STORE_ZERO(bank)                                 \
    static void store_zero_##bank(uint16_t addr, uint8_t value) \
    {                                                    \
        addr &= 0xff;                                    \
                                                         \
        if (addr == 0) {                                 \
            cbm2mem_set_bank_exec(value);                \
        } else if (addr == 1) {                          \
            cbm2mem_set_bank_ind(value);                 \
        } else {                                         \
            mem_ram[(0x##bank << 16) | addr] = value;    \
        }                                                \
    }

#define READ_ZERO(bank)                                   \
    static uint8_t read_zero_##bank(uint16_t addr)               \
    {                                                     \
        return mem_ram[(0x##bank << 16) | (addr & 0xff)]; \
    }

#define READ_RAM(bank)                             \
    static uint8_t read_ram_##bank(uint16_t addr)         \
    {                                              \
        return mem_ram[(0x##bank << 16) | addr];   \
    }

#define STORE_RAM(bank)                                       \
    static void store_ram_##bank(uint16_t addr, uint8_t byte) \
    {                                                         \
        if (addr == 0) {                                      \
            cbm2mem_set_bank_exec(byte);                      \
        } else if (addr == 1) {                               \
            cbm2mem_set_bank_ind(byte);                       \
        } else {                                              \
            mem_ram[(0x##bank << 16) | addr] = byte;          \
        }                                                     \
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


static void store_zeroX(uint16_t addr, uint8_t value)
{
    if (addr == 0) {
        cbm2mem_set_bank_exec(value);
    } else if (addr == 1) {
        cbm2mem_set_bank_ind(value);
    }
}

uint8_t rom_read(uint16_t addr)
{
    return mem_rom[addr];
}


void rom_store(uint16_t addr, uint8_t value)
{
    mem_rom[addr] = value;
}

uint8_t read_unused(uint16_t addr)
{
    if (addr == 0) {
        return cbm2mem_bank_exec;
    } else if (addr == 1) {
        return cbm2mem_bank_ind;
    }
    return 0xff; /* (addr >> 8) & 0xff; */
}

static void store_dummy(uint16_t addr, uint8_t value)
{
    return;
}

/* ------------------------------------------------------------------------- */

/* Functions for watchpoint memory access.  */

static uint8_t zero_read_watch(uint16_t addr)
{
    addr &= 0xff;
    monitor_watch_push_load_addr(addr, e_comp_space);
    return _mem_read_tab[cbm2mem_bank_exec][0](addr);
}

static void zero_store_watch(uint16_t addr, uint8_t value)
{
    addr &= 0xff;
    monitor_watch_push_store_addr(addr, e_comp_space);
    _mem_write_tab[cbm2mem_bank_exec][0](addr, value);
}

static uint8_t read_watch(uint16_t addr)
{
    monitor_watch_push_load_addr(addr, e_comp_space);
    return _mem_read_tab[cbm2mem_bank_exec][addr >> 8](addr);
}

static void store_watch(uint16_t addr, uint8_t value)
{
    monitor_watch_push_store_addr(addr, e_comp_space);
    _mem_write_tab[cbm2mem_bank_exec][addr >> 8](addr, value);
}

static uint8_t read_ind_watch(uint16_t addr)
{
    monitor_watch_push_load_addr(addr, e_comp_space);
    return _mem_read_tab[cbm2mem_bank_ind][addr >> 8](addr);
}

static void store_ind_watch(uint16_t addr, uint8_t value)
{
    monitor_watch_push_store_addr(addr, e_comp_space);
    _mem_write_tab[cbm2mem_bank_ind][addr >> 8](addr, value);
}

/* ------------------------------------------------------------------------- */

/* Generic memory access.  */

void mem_store(uint16_t addr, uint8_t value)
{
    _mem_write_tab_ptr[addr >> 8](addr, value);
}

uint8_t mem_read(uint16_t addr)
{
    return _mem_read_tab_ptr[addr >> 8](addr);
}

/* ------------------------------------------------------------------------- */

static void store_io(uint16_t addr, uint8_t value)
{
    switch (addr & 0xf800) {
        case 0xd000:
            rom_store(addr, value);     /* video RAM mapped here... */
            return;
        case 0xd800:
            switch (addr & 0xff00) {
                case 0xd800:
                    cbm2io_d800_store(addr, value);
                    return;
                case 0xd900:
                    cbm2io_d900_store(addr, value);
                    return;
                case 0xda00:
                    cbm2io_da00_store(addr, value);
                    return;
                case 0xdb00:
                    cbm2io_db00_store(addr, value);
                    return;
                case 0xdc00:
                    cbm2io_dc00_store(addr, value);
                    return;
                case 0xdd00:
                    cbm2io_dd00_store(addr, value);
                    return;
                case 0xde00:
                    cbm2io_de00_store(addr, value);
                    return;
                case 0xdf00:
                    cbm2io_df00_store(addr, value);
                    return;
            }
    }
}

static uint8_t read_io(uint16_t addr)
{
    uint8_t last_access = 0xff;

    switch (addr & 0xf800) {
        case 0xd000:
            return rom_read(addr);
        case 0xd800:
            switch (addr & 0xff00) {
                case 0xd800:
                    last_access = cbm2io_d800_read(addr);
                    break;
                case 0xd900:
                    last_access = cbm2io_d900_read(addr);
                    break;
                case 0xda00:
                    last_access = cbm2io_da00_read(addr);
                    break;
                case 0xdb00:
                    last_access = cbm2io_db00_read(addr);
                    break;
                case 0xdc00:
                    last_access = cbm2io_dc00_read(addr);
                    break;
                case 0xdd00:
                    last_access = cbm2io_dd00_read(addr);
                    break;
                case 0xde00:
                    last_access = cbm2io_de00_read(addr);
                    break;
                case 0xdf00:
                    last_access = cbm2io_df00_read(addr);
                    break;
            }
    }
    return last_access;
}

/* called by mem_toggle_watchpoints() */
static void mem_update_tab_ptrs(int flag)
{
    if (flag) {
        _mem_read_tab_ptr = _mem_read_tab_watch;
        _mem_write_tab_ptr = _mem_write_tab_watch;
        _mem_read_ind_tab_ptr = _mem_read_ind_tab_watch;
        _mem_write_ind_tab_ptr = _mem_write_ind_tab_watch;
        if (flag > 1) {
            /* enable watchpoints on dummy accesses */
            _mem_read_tab_ptr_dummy = _mem_read_tab_watch;
            _mem_write_tab_ptr_dummy = _mem_write_tab_watch;
            _mem_read_ind_tab_ptr_dummy = _mem_read_ind_tab_watch;
            _mem_write_ind_tab_ptr_dummy = _mem_write_ind_tab_watch;
        } else {
            _mem_read_tab_ptr_dummy = _mem_read_tab[cbm2mem_bank_exec];
            _mem_write_tab_ptr_dummy = _mem_write_tab[cbm2mem_bank_exec];
            _mem_read_ind_tab_ptr_dummy = _mem_read_tab[cbm2mem_bank_ind];
            _mem_write_ind_tab_ptr_dummy = _mem_write_tab[cbm2mem_bank_ind];
        }
    } else {
        /* all watchpoints disabled */
        _mem_read_tab_ptr = _mem_read_tab[cbm2mem_bank_exec];
        _mem_write_tab_ptr = _mem_write_tab[cbm2mem_bank_exec];
        _mem_read_tab_ptr_dummy = _mem_read_tab[cbm2mem_bank_exec];
        _mem_write_tab_ptr_dummy = _mem_write_tab[cbm2mem_bank_exec];
        _mem_read_ind_tab_ptr = _mem_read_tab[cbm2mem_bank_ind];
        _mem_write_ind_tab_ptr = _mem_write_tab[cbm2mem_bank_ind];
        _mem_read_ind_tab_ptr_dummy = _mem_read_tab[cbm2mem_bank_ind];
        _mem_write_ind_tab_ptr_dummy = _mem_write_tab[cbm2mem_bank_ind];
    }
}

/* FIXME: TODO! */
void mem_toggle_watchpoints(int flag, void *context)
{
    mem_update_tab_ptrs(flag);
    watchpoints_active = flag;
    if (!flag) {
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
}

/* ------------------------------------------------------------------------- */

void mem_initialize_memory(void)
{
    int i;

    mem_chargen_rom_ptr = mem_chargen_rom;

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
            mem_read_limit_tab[1][i] = 0;
        } else
        if (i < 0xe0) { /* I/O D000-DFFF */
            mem_read_limit_tab[1][i] = 0;
        } else {        /* ROM E000-FFFF */
            mem_read_limit_tab[1][i] = 0xfffd;
        }
    }

    /* setup watchpoint tables */
    for (i = 0; i <= 0x100; i++) {
        _mem_read_tab_watch[i] = read_watch;
        _mem_read_ind_tab_watch[i] = read_ind_watch;
        _mem_write_tab_watch[i] = store_watch;
        _mem_write_ind_tab_watch[i] = store_ind_watch;
    }
    /* FIXME: what about _ind_tab_watch ? */
    _mem_read_tab_watch[0] = zero_read_watch;
    _mem_write_tab_watch[0] = zero_store_watch;
}

void mem_initialize_memory_bank(int i)
{
    int j;

    switch (i) {
        case 0:
            if (ramsize >= 512) {
                for (j = 255; j >= 0; j--) {
                    _mem_read_tab[i][j] = read_ram_tab[i];
                    _mem_write_tab[i][j] = store_ram_tab[i];
                    _mem_read_base_tab[i][j] = mem_ram + (i << 16) + (j << 8);
                }
                _mem_write_tab[i][0] = store_zero_tab[i];
                _mem_read_tab[i][0] = read_zero_tab[i];
            } else {
                for (j = 255; j >= 0; j--) {
                    _mem_read_tab[i][j] = read_unused;
                    _mem_write_tab[i][j] = store_dummy;
                    _mem_read_base_tab[i][j] = NULL;
                }
                _mem_write_tab[i][0] = store_zeroX;
            }
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
            /* If this failed, we'll be handled by the failure case in case 14 */
            /* FALL THROUGH */
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
            /* If this failed, we'll be handled by the failure case in case 14 */
            /* FALL THROUGH */
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
            /* If this failed, we'll be handled by the failure case in case 14 */
            /* FALL THROUGH */
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
                _mem_read_tab[i][j] = read_unused;
                _mem_write_tab[i][j] = store_dummy;
                _mem_read_base_tab[i][j] = NULL;
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

void mem_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
    uint8_t *p = _mem_read_base_tab_ptr[addr >> 8];

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

void mem_get_basic_text(uint16_t *start, uint16_t *end)
{
    if (start != NULL) {
        *start = mem_page_zero[0x2d] | (mem_page_zero[0x2e] << 8);
    }
    if (end != NULL) {
        *end = mem_page_zero[0x2f] | (mem_page_zero[0x30] << 8);
    }
}

/* FIXME: This is likely incomplete!  */
void mem_set_basic_text(uint16_t start, uint16_t end)
{
    mem_page_zero[0x2d] = start & 0xff;
    mem_page_zero[0x2e] = start >> 8;
    mem_page_zero[0x2f] = end & 0xff;
    mem_page_zero[0x30] = end >> 8;
}

/* this function should always read from the screen currently used by the kernal
   for output, normally this does just return system ram - except when the 
   videoram is not memory mapped.
   used by autostart to "read" the kernal messages
*/
uint8_t mem_read_screen(uint16_t addr)
{
    return mem_read(addr);
}

void mem_inject(uint32_t addr, uint8_t value)
{
    /* since this is used by autostart, this should point to basic memory */
    /* FIXME: handle > 64kb */
    mem_ram[0x10000 + (addr & 0xffff)] = value;
}
/* In banked memory architectures this will always write to the bank that
   contains the keyboard buffer and "number of keys in buffer", regardless of
   what the CPU "sees" currently.
   In all other cases this just writes to the first 64kb block, usually by
   wrapping to mem_inject().
*/
void mem_inject_key(uint16_t addr, uint8_t value)
{
    /* write to "romio" bank */
    mem_bank_write(16, addr, value, NULL);
}
/* ------------------------------------------------------------------------- */

int mem_rom_trap_allowed(uint16_t addr)
{
    return 1;   /* (addr >= 0xf000) && !(map_reg & 0x80); */
}

void mem_set_tape_sense(int value)
{
}

/* ------------------------------------------------------------------------- */

/* Banked memory access functions for the monitor.  */

static uint8_t peek_bank_io(uint16_t addr)
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
                    return cbm2io_d800_peek(addr);
                case 0xd900:
                    return cbm2io_d900_peek(addr);
                case 0xda00:
                    return cbm2io_da00_peek(addr);
                case 0xdb00:
                    return cbm2io_db00_peek(addr);
                case 0xdc00:
                    return cbm2io_dc00_peek(addr);
                case 0xdd00:
                    return cbm2io_dd00_peek(addr);
                case 0xde00:
                    return cbm2io_de00_peek(addr);
                case 0xdf00:
                    return cbm2io_df00_peek(addr);
            }
    }
    return read_unused(addr);
}

/* Exported banked memory access functions for the monitor.  */

#define MAXBANKS (2 + 16 + 2)

static const char *banknames[MAXBANKS + 1] = {
    "default", "cpu",
    /* by convention, a "bank array" has a 2-hex-digit bank index appended */
    "ram00", "ram01", "ram02", "ram03", "ram04", "ram05", "ram06", "ram07",
    "ram08", "ram09", "ram0a", "ram0b", "ram0c", "ram0d", "ram0e", "ram0f",
    "romio", "io",
    NULL
};

static const int banknums[MAXBANKS + 1] = {
    17, 17,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 16,
    -1
};

static const int bankindex[MAXBANKS + 1] = {
    -1, -1,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    -1, -1,
    -1
};

static const int bankflags[MAXBANKS + 1] = {
    0, 0,
    MEM_BANK_ISARRAY | MEM_BANK_ISARRAYFIRST, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY | MEM_BANK_ISARRAYLAST,
    0, 0,
    -1
};

const char **mem_bank_list(void)
{
    return banknames;
}

const int *mem_bank_list_nos(void) {
    return banknums;
}

/* return bank number for a given literal bank name */
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

/* return current index for a given bank */
int mem_bank_index_from_bank(int bank)
{
    int i = 0;

    while (banknums[i] > -1) {
        if (banknums[i] == bank) {
            return bankindex[i];
        }
        i++;
    }
    return -1;
}

int mem_bank_flags_from_bank(int bank)
{
    int i = 0;

    while (banknums[i] > -1) {
        if (banknums[i] == bank) {
            return bankflags[i];
        }
        i++;
    }
    return -1;
}

uint8_t mem_bank_read(int bank, uint16_t addr, void *context)
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

/* used by monitor if sfx off */
uint8_t mem_bank_peek(int bank, uint16_t addr, void *context)
{
    if (bank == 16) {
        if (addr >= 0xc000 && addr < 0xe000) {
            return peek_bank_io(addr);
        }
    }
    return mem_bank_read(bank, addr, context);
}

void mem_bank_write(int bank, uint16_t addr, uint8_t byte, void *context)
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

/* used by monitor if sfx off */
void mem_bank_poke(int bank, uint16_t addr, uint8_t byte, void *context)
{
    mem_bank_write(bank, addr, byte, context);
}

mem_ioreg_list_t *mem_ioreg_list_get(void *context)
{
    mem_ioreg_list_t *mem_ioreg_list = NULL;

    io_source_ioreg_add_list(&mem_ioreg_list);

    return mem_ioreg_list;
}

void mem_get_screen_parameter(uint16_t *base, uint8_t *rows, uint8_t *columns, int *bank)
{
    *base = 0xd000;
    *bank = 16;
    *columns = crtc_peek_register(CRTC_REG_HDISP);
    *rows = crtc_peek_register(CRTC_REG_VDISP);
}

/* used by autostart to locate and "read" kernal output on the current screen
 * this function should return whatever the kernal currently uses, regardless
 * what is currently visible/active in the UI 
 */
void mem_get_cursor_parameter(uint16_t *screen_addr, uint8_t *cursor_column, uint8_t *line_length, int *blinking)
{
    *screen_addr = zero_read(0xc8) + zero_read(0xc9) * 256; /* Current Screen Line Address */
    *cursor_column = zero_read(0xcb);    /* Cursor Column on Current Line */

    *line_length = crtc_peek_register(CRTC_REG_HDISP); /* Physical Screen Line Length */
    /* Cursor Blink enable: 1 = Flash Cursor, 0 = Cursor disabled, -1 = n/a */
    *blinking = (crtc_peek_register(CRTC_REG_CURSORSTART) != 1);
    /* printf("mem_get_cursor_parameter %04x %d %d %d\n", *screen_addr, *cursor_column, *line_length, *blinking); */
}

/* ------------------------------------------------------------------------- */

static int cia1_dump(void)
{
    return ciacore_dump(machine_context.cia1);
}

static int tpi1_dump(void)
{
    return tpicore_dump(machine_context.tpi1);
}

static int tpi2_dump(void)
{
    return tpicore_dump(machine_context.tpi2);
}

/* ------------------------------------------------------------------------- */

static io_source_t crtc_device = {
    "CRTC",                /* name of the chip */
    IO_DETACH_NEVER,       /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
    0xd800, 0xd8ff, 0x01,  /* range for the chip, regs:$d800-$d801, mirrors: $d802-$d8ff */
    1,                     /* read is always valid */
    crtc_store,            /* store function */
    NULL,                  /* NO poke function */
    crtc_read,             /* read function */
    crtc_peek,             /* peek function */
    crtc_dump,             /* chip state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_HIGH,          /* high priority, chip never involved in collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_t sid_device = {
    "SID",                 /* name of the chip */
    IO_DETACH_NEVER,       /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
    0xda00, 0xdaff, 0x1f,  /* range for the chip, regs:$da00-$da1f, mirrors:$da20-$daff */
    1,                     /* read is always valid */
    sid_store,             /* store function */
    NULL,                  /* NO poke function */
    sid_read,              /* read function */
    sid_peek,              /* peek function */
    sid_dump,              /* chip state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_HIGH,          /* high priority, chip never involved in collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_t cia_device = {
    "CIA",                 /* name of the chip */
    IO_DETACH_NEVER,       /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
    0xdc00, 0xdcff, 0x0f,  /* range for the chip, regs:$dc00-$dc0f, mirrors:$dc10-$dcff */
    1,                     /* read is always valid */
    cia1_store,            /* store function */
    NULL,                  /* NO poke function */
    cia1_read,             /* read function */
    cia1_peek,             /* peek function */
    cia1_dump,             /* chip state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_HIGH,          /* high priority, chip never involved in collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_t acia_device = {
    "ACIA",                /* name of the chip */
    IO_DETACH_NEVER,       /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
    0xdd00, 0xddff, 0x03,  /* range for the chip, regs:$dd00-$dd03, mirrors:$dd04-$ddff */
    1,                     /* read is always valid */
    acia1_store,           /* store function */
    NULL,                  /* NO poke function */
    acia1_read,            /* read function */
    acia1_peek,            /* peek function */
    acia1_dump,            /* chip state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_HIGH,          /* high priority, chip never involved in collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_t tpi1_device = {
    "TPI1",                /* name of the chip */
    IO_DETACH_NEVER,       /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
    0xde00, 0xdeff, 0x07,  /* range for the chip, regs:$de00-$de07, mirrors:$de08-$deff */
    1,                     /* read is always valid */
    tpi1_store,            /* store function */
    NULL,                  /* NO poke function */
    tpi1_read,             /* read function */
    tpi1_peek,             /* peek function */
    tpi1_dump,             /* chip state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_HIGH,          /* high priority, chip never involved in collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_t tpi2_device = {
    "TPI2",                /* name of the chip */
    IO_DETACH_NEVER,       /* chip is never involved in collisions, so no detach */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
    0xdf00, 0xdfff, 0x07,  /* range for the chip, regs:$df00-$df07, mirrors:$df08-$dfff */
    1,                     /* read is always valid */
    tpi2_store,            /* store function */
    NULL,                  /* NO poke function */
    tpi2_read,             /* read function */
    tpi2_peek,             /* peek function */
    tpi2_dump,             /* chip state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_HIGH,          /* high priority, chip never involved in collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *crtc_list_item = NULL;
static io_source_list_t *sid_list_item = NULL;
static io_source_list_t *cia_list_item = NULL;
static io_source_list_t *acia_list_item = NULL;
static io_source_list_t *tpi1_list_item = NULL;
static io_source_list_t *tpi2_list_item = NULL;

/* CBM2-specific I/O initialization. */
void cbm2io_init(void)
{
    crtc_list_item = io_source_register(&crtc_device);
    sid_list_item = io_source_register(&sid_device);
    cia_list_item = io_source_register(&cia_device);
    acia_list_item = io_source_register(&acia_device);
    tpi1_list_item = io_source_register(&tpi1_device);
    tpi2_list_item = io_source_register(&tpi2_device);
}
