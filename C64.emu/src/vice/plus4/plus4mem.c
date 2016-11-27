/*
 * plus4mem.c -- Plus4 memory handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Tibor Biczo <crown@axelero.hu>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "cartio.h"
#include "datasette.h"
#include "digiblaster.h"
#include "iecbus.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "plus4iec.h"
#include "plus4mem.h"
#include "plus4memcsory256k.h"
#include "plus4memhannes256k.h"
#include "plus4memlimit.h"
#include "plus4memrom.h"
#include "plus4pio1.h"
#include "plus4pio2.h"
#include "plus4tcbm.h"
#include "ram.h"
#include "resources.h"
#include "tapeport.h"
#include "ted.h"
#include "ted-mem.h"
#include "types.h"

static int hard_reset_flag = 1;

/* ------------------------------------------------------------------------- */

/* Number of possible memory configurations.  */
#define NUM_CONFIGS     32

/* The Plus4 memory.  */
BYTE mem_ram[PLUS4_RAM_SIZE];

#ifdef USE_EMBEDDED
#include "plus43plus1lo.h"
#include "plus43plus1hi.h"
#else
BYTE extromlo1[PLUS4_BASIC_ROM_SIZE];
BYTE extromhi1[PLUS4_KERNAL_ROM_SIZE];
#endif

BYTE extromlo2[PLUS4_BASIC_ROM_SIZE];
BYTE extromlo3[PLUS4_BASIC_ROM_SIZE];
BYTE extromhi2[PLUS4_KERNAL_ROM_SIZE];
BYTE extromhi3[PLUS4_KERNAL_ROM_SIZE];

/* Pointers to the currently used memory read and write tables.  */
read_func_ptr_t *_mem_read_tab_ptr;
store_func_ptr_t *_mem_write_tab_ptr;
static BYTE **_mem_read_base_tab_ptr;
static int *mem_read_limit_tab_ptr;

/* Memory read and write tables.  */
static store_func_ptr_t mem_write_tab[NUM_CONFIGS][0x101];
static read_func_ptr_t mem_read_tab[NUM_CONFIGS][0x101];
static BYTE *mem_read_base_tab[NUM_CONFIGS][0x101];
static int mem_read_limit_tab[NUM_CONFIGS][0x101];

static store_func_ptr_t mem_write_tab_watch[0x101];
static read_func_ptr_t mem_read_tab_watch[0x101];

/* Processor port.  */
pport_t pport;

/* Current memory configuration.  */
unsigned int mem_config;

/* ------------------------------------------------------------------------- */

#define RAM0 mem_ram + 0x0000
#define RAM4 mem_ram + 0x4000
#define RAM8 mem_ram + 0x8000
#define RAMC mem_ram + 0xc000

static BYTE *chargen_tab[8][16] = {
    /* 0000-3fff, RAM selected  */
    {       RAM0, RAM0, RAM0, RAM0,
            RAM0, RAM0, RAM0, RAM0,
            RAM0, RAM0, RAM0, RAM0,
            RAM0, RAM0, RAM0, RAM0 },
    /* 4000-7fff, RAM selected  */
    {       RAM4, RAM4, RAM4, RAM4,
            RAM4, RAM4, RAM4, RAM4,
            RAM4, RAM4, RAM4, RAM4,
            RAM4, RAM4, RAM4, RAM4 },
    /* 8000-bfff, RAM selected  */
    {       RAM8, RAM8, RAM8, RAM8,
            RAM8, RAM8, RAM8, RAM8,
            RAM8, RAM8, RAM8, RAM8,
            RAM8, RAM8, RAM8, RAM8 },
    /* c000-ffff, RAM selected  */
    {       RAMC, RAMC, RAMC, RAMC,
            RAMC, RAMC, RAMC, RAMC,
            RAMC, RAMC, RAMC, RAMC,
            RAMC, RAMC, RAMC, RAMC },

    /* 0000-3fff, ROM selected  */
    {       RAM0, RAM0, RAM0, RAM0,
            RAM0, RAM0, RAM0, RAM0,
            RAM0, RAM0, RAM0, RAM0,
            RAM0, RAM0, RAM0, RAM0 },
    /* 4000-7fff, ROM selected  */
    {       RAM4, RAM4, RAM4, RAM4,
            RAM4, RAM4, RAM4, RAM4,
            RAM4, RAM4, RAM4, RAM4,
            RAM4, RAM4, RAM4, RAM4 },
    /* 8000-bfff, ROM selected  */
    {  plus4memrom_basic_rom, extromlo1, extromlo2, extromlo3,
       plus4memrom_basic_rom, extromlo1, extromlo2, extromlo3,
       plus4memrom_basic_rom, extromlo1, extromlo2, extromlo3,
       plus4memrom_basic_rom, extromlo1, extromlo2, extromlo3 },
    /* c000-ffff, ROM selected  */
    {  plus4memrom_kernal_rom, plus4memrom_kernal_rom,
       plus4memrom_kernal_rom, plus4memrom_kernal_rom,
       extromhi1, extromhi1, extromhi1, extromhi1,
       extromhi2, extromhi2, extromhi2, extromhi2,
       extromhi3, extromhi3, extromhi3, extromhi3 }
};


BYTE *mem_get_tedmem_base(unsigned int segment)
{
    return chargen_tab[segment][mem_config >> 1];
}

/* ------------------------------------------------------------------------- */

/* Tape motor status.  */
static BYTE old_port_data_out = 0xff;

/* Tape write line status.  */
static BYTE old_port_write_bit = 0xff;

/* Tape read input.  */
static BYTE tape_read = 0xff;

static BYTE tape_write_in = 0xff;
static BYTE tape_motor_in = 0xff;

/* Current watchpoint state. 1 = watchpoints active, 0 = no watchpoints */
static int watchpoints_active;

inline static void mem_proc_port_store(void)
{
    /*  Correct clock */
    ted_handle_pending_alarms(maincpu_rmw_flag + 1);

    pport.data_out = (pport.data_out & ~pport.dir)
                     | (pport.data & pport.dir);

    if (((~pport.dir | pport.data) & 0x02) != old_port_write_bit) {
        old_port_write_bit = (~pport.dir | pport.data) & 0x02;
        tapeport_toggle_write_bit((~pport.dir | ~pport.data) & 0x02);
    }

    (*iecbus_callback_write)((BYTE)~pport.data_out, last_write_cycle);

    if (((pport.dir & pport.data) & 0x08) != old_port_data_out) {
        old_port_data_out = (pport.dir & pport.data) & 0x08;
        tapeport_set_motor(!old_port_data_out);
    }
}

inline static BYTE mem_proc_port_read(WORD addr)
{
    BYTE tmp;
    BYTE input;

    /*  Correct clock */
    ted_handle_pending_alarms(0);

    if (addr == 0) {
        return pport.dir;
    }

    input = ((*iecbus_callback_read)(maincpu_clk) & 0xc0);
    if (tape_read) {
        input |= 0x10;
    } else {
        input &= ~0x10;
    }

    if (tape_write_in) {
        input |= 0x02;
    } else {
        input &= ~0x02;
    }

    if (tape_motor_in) {
        input |= 0x08;
    } else {
        input &= ~0x08;
    }

    tmp = ((input & ~pport.dir) | (pport.data_out & pport.dir)) & 0xdf;

    return tmp;
}

void mem_proc_port_trigger_flux_change(unsigned int on)
{
    /*printf("FLUXCHANGE\n");*/
    tape_read = on;
}

void mem_proc_port_set_write_in(int val)
{
    tape_write_in = val;
}

void mem_proc_port_set_motor_in(int val)
{
    tape_motor_in = val;
}

/* ------------------------------------------------------------------------- */

BYTE zero_read(WORD addr)
{
    addr &= 0xff;

    switch ((BYTE)addr) {
        case 0:
        case 1:
            return mem_proc_port_read(addr);
    }
    if (!cs256k_enabled) {
        return mem_ram[addr];
    } else {
        return cs256k_read(addr);
    }
}

void zero_store(WORD addr, BYTE value)
{
    addr &= 0xff;

    switch ((BYTE)addr) {
        case 0:
            if (pport.dir != value) {
                pport.dir = value & 0xdf;
                mem_proc_port_store();
            }
            if (!cs256k_enabled) {
                mem_ram[addr] = value;
            } else {
                cs256k_store(addr, value);
            }
            break;
        case 1:
            if (pport.data != value) {
                pport.data = value;
                mem_proc_port_store();
            }
            if (!cs256k_enabled) {
                mem_ram[addr] = value;
            } else {
                cs256k_store(addr, value);
            }
            break;
        default:
            mem_ram[addr] = value;
    }
}

/* ------------------------------------------------------------------------- */

static void mem_config_set(unsigned int config)
{
    mem_config = config;

    if (watchpoints_active) {
        _mem_read_tab_ptr = mem_read_tab_watch;
        _mem_write_tab_ptr = mem_write_tab_watch;
    } else {
        _mem_read_tab_ptr = mem_read_tab[mem_config];
        _mem_write_tab_ptr = mem_write_tab[mem_config];
    }

    _mem_read_base_tab_ptr = mem_read_base_tab[mem_config];
    mem_read_limit_tab_ptr = mem_read_limit_tab[mem_config];

    maincpu_resync_limits();
}

void mem_config_ram_set(unsigned int config)
{
    mem_config_set((mem_config & ~0x01) | config);
}

void mem_config_rom_set(unsigned int config)
{
    mem_config_set((mem_config & ~0x1e) | config);
}

/* ------------------------------------------------------------------------- */

static BYTE zero_read_watch(WORD addr)
{
    addr &= 0xff;
    monitor_watch_push_load_addr(addr, e_comp_space);
    return mem_read_tab[mem_config][0](addr);
}

static void zero_store_watch(WORD addr, BYTE value)
{
    addr &= 0xff;
    monitor_watch_push_store_addr(addr, e_comp_space);
    mem_write_tab[mem_config][0](addr, value);
}

static BYTE read_watch(WORD addr)
{
    monitor_watch_push_load_addr(addr, e_comp_space);
    return mem_read_tab[mem_config][addr >> 8](addr);
}


static void store_watch(WORD addr, BYTE value)
{
    monitor_watch_push_store_addr(addr, e_comp_space);
    mem_write_tab[mem_config][addr >> 8](addr, value);
}

void mem_toggle_watchpoints(int flag, void *context)
{
    if (flag) {
        _mem_read_tab_ptr = mem_read_tab_watch;
        _mem_write_tab_ptr = mem_write_tab_watch;
    } else {
        _mem_read_tab_ptr = mem_read_tab[mem_config];
        _mem_write_tab_ptr = mem_write_tab[mem_config];
    }
    watchpoints_active = flag;
}

/* ------------------------------------------------------------------------- */

static BYTE ram_read(WORD addr)
{
    return mem_ram[addr];
}

static BYTE ram_read_32k(WORD addr)
{
    return mem_ram[addr & 0x7fff];
}

static BYTE ram_read_16k(WORD addr)
{
    return mem_ram[addr & 0x3fff];
}

static void ram_store(WORD addr, BYTE value)
{
    mem_ram[addr] = value;
}

static void ram_store_32k(WORD addr, BYTE value)
{
    mem_ram[addr & 0x7fff] = value;
}

static void ram_store_16k(WORD addr, BYTE value)
{
    mem_ram[addr & 0x3fff] = value;
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

/*
    note: the TED pseudo registers at ff3e/3f can't be read.

    FIXME: "You should also note that if RAM is selected (after a write to ff3f)
    the contents of RAM are never visible in these locations (usually seems to
    return FF) Likewise, writing to these locations is never mirrored to
    underlying RAM either. You can prove this quite easily on a stock 16k
    machine, where the memory is mirrored 4x across the entire address space."
*/
static BYTE h256k_ram_ffxx_read(WORD addr)
{
    if ((addr >= 0xff20) && (addr != 0xff3e) && (addr != 0xff3f)) {
        return h256k_read(addr);
    }

    return ted_read(addr);
}

static BYTE cs256k_ram_ffxx_read(WORD addr)
{
    if ((addr >= 0xff20) && (addr != 0xff3e) && (addr != 0xff3f)) {
        return cs256k_read(addr);
    }

    return ted_read(addr);
}

static BYTE ram_ffxx_read(WORD addr)
{
    if ((addr >= 0xff20) && (addr != 0xff3e) && (addr != 0xff3f)) {
        return ram_read(addr);
    }

    return ted_read(addr);
}

static BYTE ram_ffxx_read_32k(WORD addr)
{
    if ((addr >= 0xff20) && (addr != 0xff3e) && (addr != 0xff3f)) {
        return ram_read_32k(addr);
    }

    return ted_read(addr);
}

static BYTE ram_ffxx_read_16k(WORD addr)
{
    if ((addr >= 0xff20) && (addr != 0xff3e) && (addr != 0xff3f)) {
        return ram_read_16k(addr);
    }

    return ted_read(addr);
}


static void h256k_ram_ffxx_store(WORD addr, BYTE value)
{
    if (addr < 0xff20 || addr == 0xff3e || addr == 0xff3f) {
        ted_store(addr, value);
    } else {
        h256k_store(addr, value);
    }
}

static void cs256k_ram_ffxx_store(WORD addr, BYTE value)
{
    if (addr < 0xff20 || addr == 0xff3e || addr == 0xff3f) {
        ted_store(addr, value);
    } else {
        cs256k_store(addr, value);
    }
}

static void ram_ffxx_store(WORD addr, BYTE value)
{
    if (addr < 0xff20 || addr == 0xff3e || addr == 0xff3f) {
        ted_store(addr, value);
    } else {
        ram_store(addr, value);
    }
}

static void ram_ffxx_store_32k(WORD addr, BYTE value)
{
    if (addr < 0xff20 || addr == 0xff3e || addr == 0xff3f) {
        ted_store(addr, value);
    } else {
        ram_store_32k(addr, value);
    }
}

static void ram_ffxx_store_16k(WORD addr, BYTE value)
{
    if (addr < 0xff20 || addr == 0xff3e || addr == 0xff3f) {
        ted_store(addr, value);
    } else {
        ram_store_16k(addr, value);
    }
}
/*
    If the ROM is currently visible, ie after a write to ff3e, then reading ff3e
    and ff3f returns the contents of the underlying ROM, exactly as is does with
    ff20 - ff3d.
*/
static BYTE rom_ffxx_read(WORD addr)
{
    if (addr >= 0xff20) {
        return plus4memrom_rom_read(addr);
    }

    return ted_read(addr);
}

static void rom_ffxx_store(WORD addr, BYTE value)
{
    if (addr < 0xff20 || addr == 0xff3e || addr == 0xff3f) {
        ted_store(addr, value);
    } else {
        ram_store(addr, value);
    }
}

static void h256k_rom_ffxx_store(WORD addr, BYTE value)
{
    if (addr < 0xff20 || addr == 0xff3e || addr == 0xff3f) {
        ted_store(addr, value);
    } else {
        h256k_store(addr, value);
    }
}

static void cs256k_rom_ffxx_store(WORD addr, BYTE value)
{
    if (addr < 0xff20 || addr == 0xff3e || addr == 0xff3f) {
        ted_store(addr, value);
    } else {
        cs256k_store(addr, value);
    }
}

static void rom_ffxx_store_32k(WORD addr, BYTE value)
{
    if (addr < 0xff20 || addr == 0xff3e || addr == 0xff3f) {
        ted_store(addr, value);
    } else {
        ram_store_32k(addr, value);
    }
}

static void rom_ffxx_store_16k(WORD addr, BYTE value)
{
    if (addr < 0xff20 || addr == 0xff3e || addr == 0xff3f) {
        ted_store(addr, value);
    } else {
        ram_store_16k(addr, value);
    }
}

/* FIXME: this always returns 0x00, but it should return the
          last data floating on the bus. (cpu/ted/dma) */
BYTE read_unused(WORD addr)
{
    return 0;
}

static void mem_config_rom_set_store(WORD addr, BYTE value)
{
    mem_config_rom_set((addr & 0xf) << 1);
}

/* ------------------------------------------------------------------------- */

static void set_write_hook(int config, int page, store_func_t *f)
{
    mem_write_tab[config][page] = f;
}

void mem_initialize_memory(void)
{
    int i, j;
    int ram_size;

    if (resources_get_int("RamSize", &ram_size) < 0) {
        return;
    }

    switch (ram_size) {
        default:
        case 256:
        case 64:
            for (i = 0; i < 16; i++) {
                chargen_tab[1][i] = RAM4;
                chargen_tab[2][i] = RAM8;
                chargen_tab[3][i] = RAMC;
                chargen_tab[5][i] = RAM4;
            }
            break;
        case 32:
            for (i = 0; i < 16; i++) {
                chargen_tab[1][i] = RAM4;
                chargen_tab[2][i] = RAM0;
                chargen_tab[3][i] = RAM4;
                chargen_tab[5][i] = RAM4;
            }
            break;
        case 16:
            for (i = 0; i < 16; i++) {
                chargen_tab[1][i] = RAM0;
                chargen_tab[2][i] = RAM0;
                chargen_tab[3][i] = RAM0;
                chargen_tab[5][i] = RAM0;
            }
            break;
    }

    mem_limit_init(mem_read_limit_tab);

    /* setup watchpoint tables */
    mem_read_tab_watch[0] = zero_read_watch;
    mem_write_tab_watch[0] = zero_store_watch;
    for (i = 1; i <= 0x100; i++) {
        mem_read_tab_watch[i] = read_watch;
        mem_write_tab_watch[i] = store_watch;
    }

    /* Default is RAM.  */
    for (i = 0; i < NUM_CONFIGS; i++) {
        set_write_hook(i, 0, zero_store);
        mem_read_tab[i][0] = zero_read;
        mem_read_base_tab[i][0] = mem_ram;
        for (j = 1; j <= 0xff; j++) {
            switch (ram_size) {
                case 4096:
                case 1024:
                case 256:
                    if (h256k_enabled && j < 0x10) {
                        mem_read_tab[i][j] = ram_read;
                        mem_write_tab[i][j] = ted_mem_vbank_store;
                    }
                    if (h256k_enabled && j >= 0x10) {
                        mem_read_tab[i][j] = h256k_read;
                        mem_write_tab[i][j] = h256k_store;
                    }
                    if (cs256k_enabled) {
                        mem_read_tab[i][j] = cs256k_read;
                        mem_write_tab[i][j] = cs256k_store;
                    }
                    mem_read_base_tab[i][j] = mem_ram + (j << 8);
                    break;
                default:
                case 64:
                    mem_read_tab[i][j] = ram_read;
                    mem_read_base_tab[i][j] = mem_ram + (j << 8);
                    mem_write_tab[i][j] = ted_mem_vbank_store;
                    break;
                case 32:
                    mem_read_tab[i][j] = ram_read_32k;
                    mem_read_base_tab[i][j] = mem_ram + ((j & 0x7f) << 8);
                    mem_write_tab[i][j] = ted_mem_vbank_store_32k;
                    break;
                case 16:
                    mem_read_tab[i][j] = ram_read_16k;
                    mem_read_base_tab[i][j] = mem_ram + ((j & 0x3f) << 8);
                    mem_write_tab[i][j] = ted_mem_vbank_store_16k;
                    break;
            }
#if 0
            if ((j & 0xc0) == (k << 6)) {
                switch (j & 0x3f) {
                    case 0x39:
                        mem_write_tab[i][j] = ted_mem_vbank_39xx_store;
                        break;
                    case 0x3f:
                        mem_write_tab[i][j] = ted_mem_vbank_3fxx_store;
                        break;
                    default:
                        mem_write_tab[i][j] = ted_mem_vbank_store;
                }
            } else {
#endif
#if 0
        }
#endif
        }
#if 0
        mem_read_tab[i][0xff] = ram_read;
        mem_read_base_tab[i][0xff] = ram + 0xff00;
        set_write_hook(i, 0xff, ram_store);
#endif
    }

    /* Setup BASIC ROM and extension ROMs at $8000-$BFFF.  */
    for (i = 0x80; i <= 0xbf; i++) {
        mem_read_tab[1][i] = plus4memrom_basic_read;
        mem_read_base_tab[1][i] = plus4memrom_basic_rom + ((i & 0x3f) << 8);
        mem_read_tab[3][i] = plus4memrom_extromlo1_read;
        mem_read_base_tab[3][i] = extromlo1 + ((i & 0x3f) << 8);
        mem_read_tab[5][i] = plus4memrom_extromlo2_read;
        mem_read_base_tab[5][i] = extromlo2 + ((i & 0x3f) << 8);
        mem_read_tab[7][i] = plus4memrom_extromlo3_read;
        mem_read_base_tab[7][i] = extromlo3 + ((i & 0x3f) << 8);
        mem_read_tab[9][i] = plus4memrom_basic_read;
        mem_read_base_tab[9][i] = plus4memrom_basic_rom + ((i & 0x3f) << 8);
        mem_read_tab[11][i] = plus4memrom_extromlo1_read;
        mem_read_base_tab[11][i] = extromlo1 + ((i & 0x3f) << 8);
        mem_read_tab[13][i] = plus4memrom_extromlo2_read;
        mem_read_base_tab[13][i] = extromlo2 + ((i & 0x3f) << 8);
        mem_read_tab[15][i] = plus4memrom_extromlo3_read;
        mem_read_base_tab[15][i] = extromlo3 + ((i & 0x3f) << 8);
        mem_read_tab[17][i] = plus4memrom_basic_read;
        mem_read_base_tab[17][i] = plus4memrom_basic_rom + ((i & 0x3f) << 8);
        mem_read_tab[19][i] = plus4memrom_extromlo1_read;
        mem_read_base_tab[19][i] = extromlo1 + ((i & 0x3f) << 8);
        mem_read_tab[21][i] = plus4memrom_extromlo2_read;
        mem_read_base_tab[21][i] = extromlo2 + ((i & 0x3f) << 8);
        mem_read_tab[23][i] = plus4memrom_extromlo3_read;
        mem_read_base_tab[23][i] = extromlo3 + ((i & 0x3f) << 8);
        mem_read_tab[25][i] = plus4memrom_basic_read;
        mem_read_base_tab[25][i] = plus4memrom_basic_rom + ((i & 0x3f) << 8);
        mem_read_tab[27][i] = plus4memrom_extromlo1_read;
        mem_read_base_tab[27][i] = extromlo1 + ((i & 0x3f) << 8);
        mem_read_tab[29][i] = plus4memrom_extromlo2_read;
        mem_read_base_tab[29][i] = extromlo2 + ((i & 0x3f) << 8);
        mem_read_tab[31][i] = plus4memrom_extromlo3_read;
        mem_read_base_tab[31][i] = extromlo3 + ((i & 0x3f) << 8);
    }

    /* Setup Kernal ROM and extension ROMs at $E000-$FFFF.  */
    for (i = 0xc0; i <= 0xff; i++) {
        mem_read_tab[1][i] = plus4memrom_kernal_read;
        mem_read_base_tab[1][i] = plus4memrom_kernal_trap_rom
                                  + ((i & 0x3f) << 8);
        mem_read_tab[3][i] = plus4memrom_kernal_read;
        mem_read_base_tab[3][i] = plus4memrom_kernal_trap_rom
                                  + ((i & 0x3f) << 8);
        mem_read_tab[5][i] = plus4memrom_kernal_read;
        mem_read_base_tab[5][i] = plus4memrom_kernal_trap_rom
                                  + ((i & 0x3f) << 8);
        mem_read_tab[7][i] = plus4memrom_kernal_read;
        mem_read_base_tab[7][i] = plus4memrom_kernal_trap_rom
                                  + ((i & 0x3f) << 8);
        mem_read_tab[9][i] = plus4memrom_extromhi1_read;
        mem_read_base_tab[9][i] = extromhi1 + ((i & 0x3f) << 8);
        mem_read_tab[11][i] = plus4memrom_extromhi1_read;
        mem_read_base_tab[11][i] = extromhi1 + ((i & 0x3f) << 8);
        mem_read_tab[13][i] = plus4memrom_extromhi1_read;
        mem_read_base_tab[13][i] = extromhi1 + ((i & 0x3f) << 8);
        mem_read_tab[15][i] = plus4memrom_extromhi1_read;
        mem_read_base_tab[15][i] = extromhi1 + ((i & 0x3f) << 8);
        mem_read_tab[17][i] = plus4memrom_extromhi2_read;
        mem_read_base_tab[17][i] = extromhi2 + ((i & 0x3f) << 8);
        mem_read_tab[19][i] = plus4memrom_extromhi2_read;
        mem_read_base_tab[19][i] = extromhi2 + ((i & 0x3f) << 8);
        mem_read_tab[21][i] = plus4memrom_extromhi2_read;
        mem_read_base_tab[21][i] = extromhi2 + ((i & 0x3f) << 8);
        mem_read_tab[23][i] = plus4memrom_extromhi2_read;
        mem_read_base_tab[23][i] = extromhi2 + ((i & 0x3f) << 8);
        mem_read_tab[25][i] = plus4memrom_extromhi3_read;
        mem_read_base_tab[25][i] = extromhi3 + ((i & 0x3f) << 8);
        mem_read_tab[27][i] = plus4memrom_extromhi3_read;
        mem_read_base_tab[27][i] = extromhi3 + ((i & 0x3f) << 8);
        mem_read_tab[29][i] = plus4memrom_extromhi3_read;
        mem_read_base_tab[29][i] = extromhi3 + ((i & 0x3f) << 8);
        mem_read_tab[31][i] = plus4memrom_extromhi3_read;
        mem_read_base_tab[31][i] = extromhi3 + ((i & 0x3f) << 8);
    }

    for (i = 0; i < NUM_CONFIGS; i += 2) {
        mem_read_tab[i + 1][0xfc] = plus4memrom_kernal_read;
        mem_read_base_tab[i + 1][0xfc] = plus4memrom_kernal_trap_rom
                                         + ((0xfc & 0x3f) << 8);

        mem_read_tab[i + 0][0xfd] = plus4io_fd00_read;
        mem_write_tab[i + 0][0xfd] = plus4io_fd00_store;
        mem_read_base_tab[i + 0][0xfd] = NULL;
        mem_read_tab[i + 1][0xfd] = plus4io_fd00_read;
        mem_write_tab[i + 1][0xfd] = plus4io_fd00_store;
        mem_read_base_tab[i + 1][0xfd] = NULL;

        mem_read_tab[i + 0][0xfe] = plus4io_fe00_read;
        mem_write_tab[i + 0][0xfe] = plus4io_fe00_store;
        mem_read_base_tab[i + 0][0xfe] = NULL;
        mem_read_tab[i + 1][0xfe] = plus4io_fe00_read;
        mem_write_tab[i + 1][0xfe] = plus4io_fe00_store;
        mem_read_base_tab[i + 1][0xfe] = NULL;

        switch (ram_size) {
            case 4096:
            case 1024:
            case 256:
                if (h256k_enabled) {
                    mem_read_tab[i + 0][0xff] = h256k_ram_ffxx_read;
                    mem_write_tab[i + 0][0xff] = h256k_ram_ffxx_store;
                    mem_write_tab[i + 1][0xff] = h256k_rom_ffxx_store;
                }
                if (cs256k_enabled) {
                    mem_read_tab[i + 0][0xff] = cs256k_ram_ffxx_read;
                    mem_write_tab[i + 0][0xff] = cs256k_ram_ffxx_store;
                    mem_write_tab[i + 1][0xff] = cs256k_rom_ffxx_store;
                }
                mem_read_base_tab[i + 0][0xff] = NULL;
                mem_read_tab[i + 1][0xff] = rom_ffxx_read;
                mem_read_base_tab[i + 1][0xff] = NULL;
                break;
            default:
            case 64:
                mem_read_tab[i + 0][0xff] = ram_ffxx_read;
                mem_write_tab[i + 0][0xff] = ram_ffxx_store;
                mem_read_base_tab[i + 0][0xff] = NULL;
                mem_read_tab[i + 1][0xff] = rom_ffxx_read;
                mem_write_tab[i + 1][0xff] = rom_ffxx_store;
                mem_read_base_tab[i + 1][0xff] = NULL;
                break;
            case 32:
                mem_read_tab[i + 0][0xff] = ram_ffxx_read_32k;
                mem_write_tab[i + 0][0xff] = ram_ffxx_store_32k;
                mem_read_base_tab[i + 0][0xff] = NULL;
                mem_read_tab[i + 1][0xff] = rom_ffxx_read;
                mem_write_tab[i + 1][0xff] = rom_ffxx_store_32k;
                mem_read_base_tab[i + 1][0xff] = NULL;
                break;
            case 16:
                mem_read_tab[i + 0][0xff] = ram_ffxx_read_16k;
                mem_write_tab[i + 0][0xff] = ram_ffxx_store_16k;
                mem_read_base_tab[i + 0][0xff] = NULL;
                mem_read_tab[i + 1][0xff] = rom_ffxx_read;
                mem_write_tab[i + 1][0xff] = rom_ffxx_store_16k;
                mem_read_base_tab[i + 1][0xff] = NULL;
                break;
        }

        mem_read_tab[i + 0][0x100] = mem_read_tab[i + 0][0];
        mem_write_tab[i + 0][0x100] = mem_write_tab[i + 0][0];
        mem_read_base_tab[i + 0][0x100] = mem_read_base_tab[i + 0][0];
        mem_read_tab[i + 1][0x100] = mem_read_tab[i + 1][0];
        mem_write_tab[i + 1][0x100] = mem_write_tab[i + 1][0];
        mem_read_base_tab[i + 1][0x100] = mem_read_base_tab[i + 1][0];
    }
    if (hard_reset_flag) {
        hard_reset_flag = 0;
        mem_config = 1;
    }
    _mem_read_tab_ptr = mem_read_tab[mem_config];
    _mem_write_tab_ptr = mem_write_tab[mem_config];
    _mem_read_base_tab_ptr = mem_read_base_tab[mem_config];
    mem_read_limit_tab_ptr = mem_read_limit_tab[mem_config];
}

void mem_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
{
    BYTE *p = _mem_read_base_tab_ptr[addr >> 8];

    *base = (p == NULL) ? NULL : (p - (addr & 0xff00));
    *start = addr; /* TODO */
    *limit = mem_read_limit_tab_ptr[addr >> 8];
}

/* ------------------------------------------------------------------------- */

/* Initialize RAM for power-up.  */
void mem_powerup(void)
{
    ram_init(mem_ram, 0x10000);

    hard_reset_flag = 1;
}

/* ------------------------------------------------------------------------- */

/* FIXME: this part needs to be checked.  */

void mem_get_basic_text(WORD *start, WORD *end)
{
    if (start != NULL) {
        *start = mem_ram[0x2b] | (mem_ram[0x2c] << 8);
    }
    if (end != NULL) {
        *end = mem_ram[0x2d] | (mem_ram[0x2e] << 8);
    }
}

void mem_set_basic_text(WORD start, WORD end)
{
    mem_ram[0x2b] = mem_ram[0xac] = start & 0xff;
    mem_ram[0x2c] = mem_ram[0xad] = start >> 8;
    mem_ram[0x2d] = mem_ram[0x2f] = mem_ram[0x31] = mem_ram[0xae] = end & 0xff;
    mem_ram[0x2e] = mem_ram[0x30] = mem_ram[0x32] = mem_ram[0xaf] = end >> 8;
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
    return addr >= 0x8000 && (mem_config & 0x1);
}

/* ------------------------------------------------------------------------- */

/* Exported banked memory access functions for the monitor.  */

static const char *banknames[] = {
    "default", "cpu", "ram", "rom", "io", "funcrom", "cart1rom", "cart2rom", NULL
};

static const int banknums[] = { 1, 0, 1, 2, 6, 3, 4, 5 };

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

void store_bank_io(WORD addr, BYTE byte)
{
    if (addr >= 0xfd00 && addr <= 0xfdff) {
        plus4io_fd00_store(addr, byte);
    }

    if (addr >= 0xfe00 && addr <= 0xfeff) {
        plus4io_fe00_store(addr, byte);
    }

    if ((addr >= 0xff00) && (addr <= 0xff3f)) {
        ted_store(addr, byte);
    } else {
        mem_store(addr, byte);
    }
}

/* read i/o without side-effects */
static BYTE peek_bank_io(WORD addr)
{
    if ((addr >= 0xff00) && (addr <= 0xff3f)) {
        return ted_peek(addr);
    }

    if (addr >= 0xfd00 && addr <= 0xfdff) {
        return plus4io_fd00_peek(addr);
    }

    if (addr >= 0xfe00 && addr <= 0xfeff) {
        return plus4io_fe00_peek(addr);
    }
    return read_unused(addr);
}

/* read i/o with side-effects */
static BYTE read_bank_io(WORD addr)
{
    if ((addr >= 0xff00) && (addr <= 0xff3f)) {
        return ted_peek(addr);
    }

    if (addr >= 0xfd00 && addr <= 0xfdff) {
        return plus4io_fd00_read(addr);
    }

    if (addr >= 0xfe00 && addr <= 0xfeff) {
        return plus4io_fe00_read(addr);
    }

    return read_unused(addr);
}

/* read memory without side-effects */
BYTE mem_bank_peek(int bank, WORD addr, void *context)
{
    switch (bank) {
        case 0:                   /* current */
            /* FIXME: we must check for which bank is currently active, and only use peek_bank_io
                      when needed. doing this without checking is wrong, but we do it anyways to
                      avoid side effects
           */
            if (addr >= 0xfd00) {
                return peek_bank_io(addr);
            }
            break;
        case 6:                   /* io */
            if (addr >= 0xfd00) {
                return peek_bank_io(addr);
            }
            break;
    }

    return mem_bank_read(bank, addr, context); /* FIXME */
}

/* read memory with side-effects */
BYTE mem_bank_read(int bank, WORD addr, void *context)
{
    switch (bank) {
        case 0:                   /* current */
            return mem_read(addr);
            break;
        case 1:                   /* ram */
            break;
        case 2:                   /* rom */
            if (addr >= 0x8000 && addr <= 0xbfff) {
                return plus4memrom_basic_rom[addr & 0x3fff];
            }
            if (addr >= 0xc000) {
                return plus4memrom_kernal_rom[addr & 0x3fff];
            }
            break;
        case 3:                   /* funcrom */
            if (addr >= 0x8000 && addr <= 0xbfff) {
                return extromlo1[addr & 0x3fff];
            }
            if (addr >= 0xc000) {
                return extromhi1[addr & 0x3fff];
            }
            break;
        case 4:                   /* cart1rom */
            if (addr >= 0x8000 && addr <= 0xbfff) {
                return extromlo2[addr & 0x3fff];
            }
            if (addr >= 0xc000) {
                return extromhi2[addr & 0x3fff];
            }
            break;
        case 5:                   /* cart2rom */
            if (addr >= 0x8000 && addr <= 0xbfff) {
                return extromlo3[addr & 0x3fff];
            }
            if (addr >= 0xc000) {
                return extromhi3[addr & 0x3fff];
            }
            break;
        case 6:                   /* i/o */
            if (addr >= 0xfd00) {
                return read_bank_io(addr);
            }
            return mem_read(addr);
            break;
    }
    return mem_ram[addr];
}

void mem_bank_write(int bank, WORD addr, BYTE byte, void *context)
{
    switch (bank) {
        case 0:                 /* current */
            mem_store(addr, byte);
            return;
        case 1:                 /* ram */
            break;
        case 2:                 /* rom */
            if (addr >= 0x8000 && addr <= 0xbfff) {
                return;
            }
            if (addr >= 0xc000) {
                return;
            }
            break;
        case 3:                 /* funcrom */
            if (addr >= 0x8000 && addr <= 0xbfff) {
                return;
            }
            if (addr >= 0xc000) {
                return;
            }
            break;
        case 4:                 /* cart1rom */
            if (addr >= 0x8000 && addr <= 0xbfff) {
                return;
            }
            if (addr >= 0xc000) {
                return;
            }
            break;
        case 5:                 /* cart2rom */
            if (addr >= 0x8000 && addr <= 0xbfff) {
                return;
            }
            if (addr >= 0xc000) {
                return;
            }
            break;
        case 6:                 /* i/o */
            store_bank_io(addr, byte);
            return;
    }
    mem_ram[addr] = byte;
}

static int mem_dump_io(void *context, WORD addr)
{
    if ((addr >= 0xff00) && (addr <= 0xff3f)) {
        /* return ted_dump(machine_context.ted); */ /* FIXME */
    }
    return -1;
}

mem_ioreg_list_t *mem_ioreg_list_get(void *context)
{
    mem_ioreg_list_t *mem_ioreg_list = NULL;

    io_source_ioreg_add_list(&mem_ioreg_list);
    mon_ioreg_add_list(&mem_ioreg_list, "TED", 0xff00, 0xff3f, mem_dump_io, NULL);

    return mem_ioreg_list;
}

void mem_get_screen_parameter(WORD *base, BYTE *rows, BYTE *columns, int *bank)
{
    *base = (ted_peek(0xff14) & 0xf8) << 8 | 0x400;
    *rows = 25;
    *columns = 40;
    *bank = 0;
}

/* ------------------------------------------------------------------------- */

typedef struct mem_config_s {
    char *mem_8000;
    char *mem_c000;
} mem_config_t;

static mem_config_t mem_config_table[] = {
    { "BASIC",  "KERNAL" }, /* 0xfdd0 */
    { "3+1",    "KERNAL" }, /* 0xfdd1 */
    { "CART-1", "KERNAL" }, /* 0xfdd2 */
    { "CART-2", "KERNAL" }, /* 0xfdd3 */
    { "BASIC",  "3+1"    }, /* 0xfdd4 */
    { "3+1",    "3+1"    }, /* 0xfdd5 */
    { "CART-1", "3+1"    }, /* 0xfdd6 */
    { "CART-2", "3+1"    }, /* 0xfdd7 */
    { "BASIC",  "CART-1" }, /* 0xfdd8 */
    { "3+1",    "CART-1" }, /* 0xfdd9 */
    { "CART-1", "CART-1" }, /* 0xfdda */
    { "CART-2"  "CART-1" }, /* 0xfddb */
    { "BASIC",  "CART-2" }, /* 0xfddc */
    { "3+1",    "CART-2" }, /* 0xfddd */
    { "CART-1", "CART-2" }, /* 0xfdde */
    { "CART-2"  "CART-2" }  /* 0xfddf */
};

static int memconfig_dump(void)
{
    mon_out("$8000-$BFFF: %s", (mem_config & 1) ? mem_config_table[mem_config >> 1].mem_8000 : "RAM");
    mon_out("$C000-$FFFF: %s", (mem_config & 1) ? mem_config_table[mem_config >> 1].mem_c000 : "RAM");

    return 0;
}

/* ------------------------------------------------------------------------- */

static io_source_t mem_config_device = {
    "MEMCONFIG",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xfdd0, 0xfddf, 0xf,
    0, /* read is never valid */
    mem_config_rom_set_store,
    NULL, /* no read */
    NULL, /* no peek */
    memconfig_dump,
    0, /* dummy (not a cartridge) */
    IO_PRIO_NORMAL,
    0
};

static io_source_t pio1_with_mirrors_device = {
    "PIO1",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xfd10, 0xfd1f, 1,
    1, /* read is always valid */
    pio1_store,
    pio1_read,
    NULL, /* no peek */
    NULL, /* nothing to dump */
    0, /* dummy (not a cartridge) */
    IO_PRIO_NORMAL,
    0
};

static io_source_t pio1_only_device = {
    "PIO1",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xfd10, 0xfd10, 1,
    1, /* read is always valid */
    pio1_store,
    pio1_read,
    NULL, /* no peek */
    NULL, /* nothing to dump */
    0, /* dummy (not a cartridge) */
    IO_PRIO_NORMAL,
    0
};

static io_source_t pio2_device = {
    "PIO2",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xfd30, 0xfd3f, 1,
    1, /* read is always valid */
    pio2_store,
    pio2_read,
    NULL, /* no peek */
    NULL, /* nothing to dump */
    0, /* dummy (not a cartridge) */
    IO_PRIO_NORMAL,
    0
};

static io_source_t tcbm1_device = {
    "TCBM1",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xfee0, 0xfeff, 0x1f,
    1, /* read is always valid */
    plus4tcbm1_store,
    plus4tcbm1_read,
    NULL, /* no peek */
    NULL, /* TODO: dump */
    0, /* dummy (not a cartridge) */
    IO_PRIO_NORMAL,
    0
};

static io_source_t tcbm2_device = {
    "TCBM2",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xfec0, 0xfedf, 0x1f,
    1, /* read is always valid */
    plus4tcbm2_store,
    plus4tcbm2_read,
    NULL, /* no peek */
    NULL, /* TODO: dump */
    0, /* dummy (not a cartridge) */
    IO_PRIO_NORMAL,
    0
};

static io_source_list_t *mem_config_list_item = NULL;
static io_source_list_t *pio1_list_item = NULL;
static io_source_list_t *pio2_list_item = NULL;
static io_source_list_t *tcbm1_list_item = NULL;
static io_source_list_t *tcbm2_list_item = NULL;

static int pio1_devices_blocking_mirror = 0;

void plus4_pio1_init(int block)
{
    int rereg = 0;

    if (pio1_devices_blocking_mirror == 0 || (pio1_devices_blocking_mirror == 1 && block == -1)) {
        io_source_unregister(pio1_list_item);
        rereg = 1;
    }

    pio1_devices_blocking_mirror += block;

    if (rereg) {
        if (!pio1_devices_blocking_mirror) {
            pio1_list_item = io_source_register(&pio1_with_mirrors_device);
        } else {
            pio1_list_item = io_source_register(&pio1_only_device);
        }
    }
}

/* C16/C232/PLUS4/V364-specific I/O initialization, only common devices. */
void plus4io_init(void)
{
    mem_config_list_item = io_source_register(&mem_config_device);
    pio1_list_item = io_source_register(&pio1_with_mirrors_device);
    pio2_list_item = io_source_register(&pio2_device);
    tcbm1_list_item = io_source_register(&tcbm1_device);
    tcbm2_list_item = io_source_register(&tcbm2_device);
}
