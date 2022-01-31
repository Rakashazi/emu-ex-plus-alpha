/*
 * c64dtvmem.c -- C64DTV memory handling.
 *
 * Written by
 *  M.Kiesel <mayne@users.sourceforge.net>
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Daniel Kahlin <daniel@kahlin.net>
 * Based on code by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "c64dtvmem.h"
#include "c64dtvblitter.h"
#include "c64dtvcpu.h"
#include "c64dtvdma.h"
#include "c64dtvflash.h"
#include "cartio.h"
#include "cia.h"
#include "cmdline.h"
#include "debugcart.h"
#include "lib.h"
#include "log.h"
#include "util.h"
#include "resources.h"
#include "mos6510dtv.h"
#include "interrupt.h"
#include "alarm.h"
#include "hummeradc.h"
#include "ps2mouse.h"

/* TODO this is a hack */
#define C64_RAM_SIZE 0x200000

#ifndef C64DTV
#define C64DTV
#endif

/* start of c64dtvmem_main.c */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c64.h"
#include "c64-resources.h"
#include "c64cia.h"
#include "c64mem.h"
#include "c64dtvmeminit.h"
#include "c64memrom.h"
#include "c64pla.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "ram.h"
#include "sid.h"
#include "vicii-mem.h"
#include "vicii-phi1.h"
#include "vicii.h"
#include "viciitypes.h"

/* #define DBGMEM */

#ifdef DBGMEM
#define DBG(x) log_message(c64dtvmem_log, x);
#else
#define DBG(x)
#endif

/* C64 memory-related resources.  */

/* ------------------------------------------------------------------------- */

/* Number of possible memory configurations.  */
#define NUM_CONFIGS     8
/* Number of possible video banks (16K each).  */
#define NUM_VBANKS      4

/* The C64 memory, see ../mem.h.  */
uint8_t mem_ram[C64_RAM_SIZE];

uint8_t mem_chargen_rom[C64_CHARGEN_ROM_SIZE];

/* Internal color memory.  */
uint8_t *mem_color_ram_cpu;
uint8_t *mem_color_ram_vicii; /* unused; needed by vicii-fetch.c */

/* Pointer to the chargen ROM.  */
uint8_t *mem_chargen_rom_ptr;

/* Pointers to the currently used memory read and write tables.  */
read_func_ptr_t *_mem_read_tab_ptr;
store_func_ptr_t *_mem_write_tab_ptr;
read_func_ptr_t *_mem_read_tab_ptr_dummy;
store_func_ptr_t *_mem_write_tab_ptr_dummy;
static uint8_t **_mem_read_base_tab_ptr;
static uint32_t *mem_read_limit_tab_ptr;

/* Memory read and write tables.  */
static store_func_ptr_t mem_write_tab[NUM_VBANKS][NUM_CONFIGS][0x101];
static read_func_ptr_t mem_read_tab[NUM_CONFIGS][0x101];
static uint8_t *mem_read_base_tab[NUM_CONFIGS][0x101];
static uint32_t mem_read_limit_tab[NUM_CONFIGS][0x101];

static store_func_ptr_t mem_write_tab_watch[0x101];
static read_func_ptr_t mem_read_tab_watch[0x101];

/* Current video bank (0, 1, 2 or 3).  */
static int vbank;

/* Current memory configuration.  */
static int mem_config;

/* Current watchpoint state. 
          0 = no watchpoints
    bit0; 1 = watchpoints active
    bit1; 2 = watchpoints trigger on dummy accesses
*/
static int watchpoints_active;

/* ------------------------------------------------------------------------- */

static uint8_t zero_read_watch(uint16_t addr)
{
    addr &= 0xff;
    monitor_watch_push_load_addr(addr, e_comp_space);
    return mem_read_tab[mem_config][0](addr);
}

static void zero_store_watch(uint16_t addr, uint8_t value)
{
    addr &= 0xff;
    monitor_watch_push_store_addr(addr, e_comp_space);
    mem_write_tab[vbank][mem_config][0](addr, value);
}

static uint8_t read_watch(uint16_t addr)
{
    monitor_watch_push_load_addr(addr, e_comp_space);
    return mem_read_tab[mem_config][addr >> 8](addr);
}

static void store_watch(uint16_t addr, uint8_t value)
{
    monitor_watch_push_store_addr(addr, e_comp_space);
    mem_write_tab[vbank][mem_config][addr >> 8](addr, value);
}

/* called by mem_pla_config_changed(), mem_toggle_watchpoints() */
static void mem_update_tab_ptrs(int flag)
{
    if (flag) {
        _mem_read_tab_ptr = mem_read_tab_watch;
        _mem_write_tab_ptr = mem_write_tab_watch;
        if (flag > 1) {
            /* enable watchpoints on dummy accesses */
            _mem_read_tab_ptr_dummy = mem_read_tab_watch;
            _mem_write_tab_ptr_dummy = mem_write_tab_watch;
        } else {
            _mem_read_tab_ptr_dummy = mem_read_tab[mem_config];
            _mem_write_tab_ptr_dummy = mem_write_tab[vbank][mem_config];
        }
    } else {
        /* all watchpoints disabled */
        _mem_read_tab_ptr = mem_read_tab[mem_config];
        _mem_write_tab_ptr = mem_write_tab[vbank][mem_config];
        _mem_read_tab_ptr_dummy = mem_read_tab[mem_config];
        _mem_write_tab_ptr_dummy = mem_write_tab[vbank][mem_config];
    }
}

void mem_toggle_watchpoints(int flag, void *context)
{
    mem_update_tab_ptrs(flag);
    watchpoints_active = flag;
}
/* ------------------------------------------------------------------------- */

void mem_pla_config_changed(void)
{
    mem_config = (((~pport.dir | pport.data) & 0x7));

    c64pla_config_changed(0, 0, 0, 1, 0x17);

    mem_update_tab_ptrs(watchpoints_active);

    _mem_read_base_tab_ptr = mem_read_base_tab[mem_config];
    mem_read_limit_tab_ptr = mem_read_limit_tab[mem_config];

    maincpu_resync_limits();
}

uint8_t zero_read(uint16_t addr)
{
    addr &= 0xff;

    switch ((uint8_t)addr) {
        case 0:
            return pport.dir_read;
        case 1:
            return pport.data_read;
    }

    return mem_ram[addr & 0xff];
}

void zero_store(uint16_t addr, uint8_t value)
{
    addr &= 0xff;

    switch ((uint8_t)addr) {
        case 0:
            if (vbank == 0) {
                vicii_mem_vbank_store((uint16_t)0, vicii_read_phi1_lowlevel());
            } else {
                mem_ram[0] = vicii_read_phi1_lowlevel();
                machine_handle_pending_alarms(maincpu_rmw_flag + 1);
            }
            if (pport.dir != value) {
                pport.dir = value;
                mem_pla_config_changed();
            }
            break;
        case 1:
            if (vbank == 0) {
                vicii_mem_vbank_store((uint16_t)1, vicii_read_phi1_lowlevel());
            } else {
                mem_ram[1] = vicii_read_phi1_lowlevel();
                machine_handle_pending_alarms(maincpu_rmw_flag + 1);
            }
            if (pport.data != value) {
                pport.data = value;
                mem_pla_config_changed();
            }
            break;
        default:
            if (vbank == 0) {
                vicii_mem_vbank_store(addr, value);
            } else {
                mem_ram[addr] = value;
            }
    }
}

/* ------------------------------------------------------------------------- */

uint8_t chargen_read(uint16_t addr)
{
    return c64dtvflash_read(addr);
}

/*void chargen_store(uint16_t addr, uint8_t value)
{
    return;
}*/

uint8_t ram_read(uint16_t addr)
{
    return mem_ram[addr];
}

void ram_store(uint16_t addr, uint8_t value)
{
    mem_ram[addr] = value;
}

void ram_hi_store(uint16_t addr, uint8_t value)
{
    if (vbank == 3) {
        vicii_mem_vbank_3fxx_store(addr, value);
    } else {
        mem_ram[addr] = value;
    }
}

/* ------------------------------------------------------------------------- */

void mem_set_write_hook(int config, int page, store_func_t *f)
{
    int i;

    for (i = 0; i < NUM_VBANKS; i++) {
        mem_write_tab[i][config][page] = f;
    }
}

void mem_read_tab_set(unsigned int base, unsigned int index,
                      read_func_ptr_t read_func)
{
    mem_read_tab[base][index] = read_func;
}

void mem_read_base_set(unsigned int base, unsigned int index, uint8_t *mem_ptr)
{
    mem_read_base_tab[base][index] = mem_ptr;
}

void mem_initialize_memory(void)
{
    int i, j, k;

    mem_chargen_rom_ptr = &c64dtvflash_mem[0x1000]; /* FIXME: 1000 or 9000 based on vicii bank */
    mem_color_ram_cpu = &mem_ram[0x01d800];
    mem_color_ram_vicii = NULL;

    /* setup watchpoint tables */
    mem_read_tab_watch[0] = zero_read_watch;
    mem_write_tab_watch[0] = zero_store_watch;
    for (i = 1; i <= 0x100; i++) {
        mem_read_tab_watch[i] = read_watch;
        mem_write_tab_watch[i] = store_watch;
    }

    /* Default is RAM.  */
    for (i = 0; i < NUM_CONFIGS; i++) {
        mem_set_write_hook(i, 0, zero_store);
        mem_read_tab[i][0] = zero_read;
        mem_read_base_tab[i][0] = mem_ram;
        for (j = 1; j <= 0xfe; j++) {
            mem_read_tab[i][j] = ram_read;
            mem_read_base_tab[i][j] = mem_ram + (j << 8);
            for (k = 0; k < NUM_VBANKS; k++) {
                if ((j & 0xc0) == (k << 6)) {
                    switch (j & 0x3f) {
                        case 0x39:
                            mem_write_tab[k][i][j] = vicii_mem_vbank_39xx_store;
                            break;
                        case 0x3f:
                            mem_write_tab[k][i][j] = vicii_mem_vbank_3fxx_store;
                            break;
                        default:
                            mem_write_tab[k][i][j] = vicii_mem_vbank_store;
                    }
                } else {
                    mem_write_tab[k][i][j] = ram_store;
                }
            }
        }
        mem_read_tab[i][0xff] = ram_read;
        mem_read_base_tab[i][0xff] = mem_ram + 0xff00;

        /* vbank access is handled within `ram_hi_store()'.  */
        mem_set_write_hook(i, 0xff, ram_hi_store);
    }

    /* Setup character generator ROM at $D000-$DFFF (memory configs 1, 2,
       3).  */
    for (i = 0xd0; i <= 0xdf; i++) {
        mem_read_tab[1][i] = chargen_read;
        mem_read_tab[2][i] = chargen_read;
        mem_read_tab[3][i] = chargen_read;
        mem_read_base_tab[1][i] = NULL;
        mem_read_base_tab[2][i] = NULL;
        mem_read_base_tab[3][i] = NULL;
    }

    c64dtvmeminit(0);
    c64dtvmem_limit_init(mem_read_limit_tab);

    for (i = 0; i < NUM_CONFIGS; i++) {
        mem_read_tab[i][0x100] = mem_read_tab[i][0];
        for (j = 0; j < NUM_VBANKS; j++) {
            mem_write_tab[j][i][0x100] = mem_write_tab[j][i][0];
        }
        mem_read_base_tab[i][0x100] = mem_read_base_tab[i][0];
    }

    _mem_read_tab_ptr = mem_read_tab[7];
    _mem_write_tab_ptr = mem_write_tab[vbank][7];
    _mem_read_base_tab_ptr = mem_read_base_tab[7];
    mem_read_limit_tab_ptr = mem_read_limit_tab[7];

    vicii_set_chargen_addr_options(0x7000, 0x1000);

    c64pla_pport_reset();

    /* Setup initial memory configuration.  */
    mem_pla_config_changed();
    c64dtvmem_init_config();
}

void mem_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
#ifdef FEATURE_CPUMEMHISTORY
    *base = NULL;
    *start = addr;
    *limit = 0;
#else
    int bank = addr >> 14;
    int paddr;
    uint8_t *p;
    uint32_t limits;

    if ((((dtv_registers[8] >> (bank * 2)) & 0x03) == 0x00)) {
        if (c64dtvflash_state) {
            *base = NULL; /* not idle */
            *limit = 0;
            *start = 0;
            return;
        }
        paddr = (((int)dtv_registers[12 + bank]) << 14) & (C64_RAM_SIZE - 1);
        *base = &c64dtvflash_mem[paddr] - (addr & 0xc000);
        *limit = (addr & 0xc000) | 0x3ffd;
        *start = addr & 0xc000;
        return;
    }
    paddr = (((int)dtv_registers[12 + bank]) << 14) & (C64_RAM_SIZE - 1);
    if (paddr < 0x10000) {
        paddr |= addr & 0x3fff;
        p = _mem_read_base_tab_ptr[paddr >> 8];
        if (p) { /* easy */
            if (addr > 1) {
                *base = p - (addr & 0xff00);
                limits = mem_read_limit_tab_ptr[paddr >> 8];
                *limit = (limits & 0xffff) - paddr + addr;
                *start = (limits >> 16) - paddr + addr;
            } else {
                *base = NULL;
                *limit = 0;
                *start = 0;
            }
            return;
        } else {
            if (!c64dtvflash_state) {
                read_func_ptr_t ptr = _mem_read_tab_ptr[paddr >> 8];
                if (ptr == c64memrom_kernal64_read) {
                    int mapping = c64dtvmem_memmapper[0];
                    paddr = ((mapping & 0x1f) << 16) + (paddr & ~0x3fff) - (addr & 0xc000);
                    *base = ((mapping & 0xc0) ? mem_ram : c64dtvflash_mem) + paddr;
                    *limit = (addr & 0xe000) | 0x1ffd;
                    *start = addr & 0xe000;
                    return;
                }
                if (ptr == c64memrom_basic64_read) {
                    int mapping = c64dtvmem_memmapper[1];
                    paddr = ((mapping & 0x1f) << 16) + (paddr & ~0x3fff) - (addr & 0xc000);
                    *base = ((mapping & 0xc0) ? mem_ram : c64dtvflash_mem) + paddr;
                    *limit = (addr & 0xe000) | 0x1ffd;
                    *start = addr & 0xe000;
                    return;
                }
                if (ptr == chargen_read) { /* not likely but anyway */
                    *base = c64dtvflash_mem + (paddr & ~0x3fff) - (addr & 0xc000);
                    *limit = (addr & 0xf000) | 0x0ffd;
                    *start = addr & 0xf000;
                    return;
                }
            }
            *base = NULL;
            *limit = 0;
            *start = 0;
        }
    } else {
        *base = &mem_ram[paddr] - (addr & 0xc000);
        *limit = (addr & 0xc000) | 0x3ffd;
        *start = addr & 0xc000;
    }
#endif
}

/* ------------------------------------------------------------------------- */

/* Initialize RAM for power-up.  */
void mem_powerup(void)
{
    ram_init(mem_ram, C64_RAM_SIZE);
}

/* ------------------------------------------------------------------------- */

/* Change the current video bank.  Call this routine only when the vbank
   has really changed.  */
void mem_set_vbank(int new_vbank)
{
    vbank = new_vbank;
    _mem_write_tab_ptr = mem_write_tab[new_vbank][mem_config];
    vicii_set_vbank(new_vbank);
}

/* ------------------------------------------------------------------------- */

/* FIXME: this part needs to be checked.  */

void mem_get_basic_text(uint16_t *start, uint16_t *end)
{
    if (start != NULL) {
        *start = mem_ram[0x2b] | (mem_ram[0x2c] << 8);
    }
    if (end != NULL) {
        *end = mem_ram[0x2d] | (mem_ram[0x2e] << 8);
    }
}

void mem_set_basic_text(uint16_t start, uint16_t end)
{
    mem_ram[0x2b] = mem_ram[0xac] = start & 0xff;
    mem_ram[0x2c] = mem_ram[0xad] = start >> 8;
    mem_ram[0x2d] = mem_ram[0x2f] = mem_ram[0x31] = mem_ram[0xae] = end & 0xff;
    mem_ram[0x2e] = mem_ram[0x30] = mem_ram[0x32] = mem_ram[0xaf] = end >> 8;
}

/* this function should always read from the screen currently used by the kernal
   for output, normally this does just return system ram - except when the 
   videoram is not memory mapped.
   used by autostart to "read" the kernal messages
*/
uint8_t mem_read_screen(uint16_t addr)
{
    return ram_read(addr);
}

void mem_inject(uint32_t addr, uint8_t value)
{
    mem_ram[addr & 0x1fffff] = value;
}

/* In banked memory architectures this will always write to the bank that
   contains the keyboard buffer and "number of keys in buffer", regardless of
   what the CPU "sees" currently.
   In all other cases this just writes to the first 64kb block, usually by
   wrapping to mem_inject().
*/
void mem_inject_key(uint16_t addr, uint8_t value)
{
    mem_inject(addr, value);
}

/* ------------------------------------------------------------------------- */

int mem_rom_trap_allowed(uint16_t addr)
{
    if (addr >= 0xe000) {
        switch (mem_config) {
            case 2:
            case 3:
            case 6:
            case 7:
                return 1;
            default:
                return 0;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

/* Banked memory access functions for the monitor.  */

/* FIXME: peek, cartridge support */

void store_bank_io(uint16_t addr, uint8_t byte)
{
    switch (addr & 0xff00) {
        case 0xd000:
        case 0xd100:
        case 0xd200:
        case 0xd300:
            vicii_store(addr, byte);
            break;
        case 0xd400:
        case 0xd500:
        case 0xd600:
        case 0xd700:
            sid_store(addr, byte);
            debugcart_store(addr, byte);
            break;
        case 0xd800:
        case 0xd900:
        case 0xda00:
        case 0xdb00:
            colorram_store(addr, byte);
            break;
        case 0xdc00:
            cia1_store(addr, byte);
            break;
        case 0xdd00:
            cia2_store(addr, byte);
            break;
        case 0xde00:
            c64io1_store(addr, byte);
            break;
        case 0xdf00:
            c64io2_store(addr, byte);
            break;
    }
    return;
}

uint8_t read_bank_io(uint16_t addr)
{
    switch (addr & 0xff00) {
        case 0xd000:
        case 0xd100:
        case 0xd200:
        case 0xd300:
            return vicii_read(addr);
        case 0xd400:
        case 0xd500:
        case 0xd600:
        case 0xd700:
            return sid_read(addr);
        case 0xd800:
        case 0xd900:
        case 0xda00:
        case 0xdb00:
            return colorram_read(addr);
        case 0xdc00:
            return cia1_read(addr);
        case 0xdd00:
            return cia2_read(addr);
        case 0xde00:
            return c64io1_read(addr);
        case 0xdf00:
            return c64io2_read(addr);
    }
    return 0xff;
}

static uint8_t peek_bank_io(uint16_t addr)
{
    switch (addr & 0xff00) {
        case 0xd000:
        case 0xd100:
        case 0xd200:
        case 0xd300:
            return vicii_peek(addr);
        case 0xd400:
        case 0xd500:
        case 0xd600:
        case 0xd700:
            return sid_read(addr);
        case 0xd800:
        case 0xd900:
        case 0xda00:
        case 0xdb00:
            return colorram_read(addr);
        case 0xdc00:
            return cia1_peek(addr);
        case 0xdd00:
            return cia2_peek(addr);
        case 0xde00:
            return c64io1_read(addr); /* FIXME */
        case 0xdf00:
            return c64io2_read(addr); /* FIXME */
    }
    return 0xff;
}

/* ------------------------------------------------------------------------- */

/* Exported banked memory access functions for the monitor.  */

static int mem_dump_io(void *context, uint16_t addr)
{
    if ((addr >= 0xd000) && (addr <= 0xd04f)) {
        return vicii_dump();
    } else if ((addr >= 0xd400) && (addr <= 0xd41f)) {
        /* return sidcore_dump(machine_context.sid); */ /* FIXME */
    } else if ((addr >= 0xdc00) && (addr <= 0xdc3f)) {
        return ciacore_dump(machine_context.cia1);
    } else if ((addr >= 0xdd00) && (addr <= 0xdd3f)) {
        return ciacore_dump(machine_context.cia2);
    }
    return -1;
}

mem_ioreg_list_t *mem_ioreg_list_get(void *context)
{
    mem_ioreg_list_t *mem_ioreg_list = NULL;

    mon_ioreg_add_list(&mem_ioreg_list, "VIC-II", 0xd000, 0xd04f, mem_dump_io, NULL, IO_MIRROR_NONE);
    /* TODO blitter, DMA... */
    mon_ioreg_add_list(&mem_ioreg_list, "SID", 0xd400, 0xd41f, mem_dump_io, NULL, IO_MIRROR_NONE);
    mon_ioreg_add_list(&mem_ioreg_list, "CIA1", 0xdc00, 0xdc0f, mem_dump_io, NULL, IO_MIRROR_NONE);
    mon_ioreg_add_list(&mem_ioreg_list, "CIA2", 0xdd00, 0xdd0f, mem_dump_io, NULL, IO_MIRROR_NONE);

    return mem_ioreg_list;
}

void mem_get_screen_parameter(uint16_t *base, uint8_t *rows, uint8_t *columns, int *bank)
{
    *base = ((vicii_peek(0xd018) & 0xf0) << 6)
            | ((~cia2_peek(0xdd00) & 0x03) << 14);
    *rows = 25;
    *columns = 40;
    *bank = 0;
}

/* used by autostart to locate and "read" kernal output on the current screen
 * this function should return whatever the kernal currently uses, regardless
 * what is currently visible/active in the UI 
 */
void mem_get_cursor_parameter(uint16_t *screen_addr, uint8_t *cursor_column, uint8_t *line_length, int *blinking)
{
    /* Cursor Blink enable: 1 = Flash Cursor, 0 = Cursor disabled, -1 = n/a */
    *blinking = mem_ram[0xcc] ? 0 : 1;
    *screen_addr = mem_ram[0xd1] + mem_ram[0xd2] * 256; /* Current Screen Line Address */
    *cursor_column = mem_ram[0xd3];    /* Cursor Column on Current Line */
    *line_length = mem_ram[0xd5] + 1;  /* Physical Screen Line Length */
}

/* end of c64dtvmem_main.c */

static log_t c64dtvmem_log = LOG_ERR;

/* I/O of the memory mapper ($D100/$D101) */
uint8_t c64dtvmem_memmapper[0x2];

/* The memory banking mechanism/virtual memory is visible to the CPU only. */
/* VICII, DMA Engine, Blitter have access to physical memory. */
/* Kernal/Basic ROM is visible in physical memory only since DTV address */
/* translation is done before determining access type (C64 MMU). */
/* Note that zeropage/stack mapping (reg10/11) is done before this - */
/* see c64dtvcpu. */

static inline int addr_to_paddr(uint16_t addr)
{
    int bank = addr >> 14;
    /* DTV register 12-15 - Remap 16k memory banks */
    return ((((int) dtv_registers[12 + bank]) << 14) + (addr & 0x3fff)) & (C64_RAM_SIZE - 1);
}

static inline int access_rom(uint16_t addr)
{
    int bank = addr >> 14;
    return(((dtv_registers[8] >> (bank * 2)) & 0x03) == 0x00);
    /* TODO 00 is ROM, 01 is RAM, what about 10 and 11? */
}

/* ------------------------------------------------------------------------- */
/* Replacements for c64mem.c code */

void mem_store(uint16_t addr, uint8_t value)
{
#ifdef FEATURE_CPUMEMHISTORY
    store_func_ptr_t rptr;
#endif

    int paddr = addr_to_paddr(addr);
/* if (addr != paddr) printf("Store to adress %x mapped to %x - %d %d %d %d\n", addr, paddr, dtv_registers[12], dtv_registers[13], dtv_registers[14], dtv_registers[15]); */ /* DEBUG */
    if (access_rom(addr)) {
#ifdef FEATURE_CPUMEMHISTORY
        monitor_memmap_store(paddr, MEMMAP_ROM_W);
#endif
        c64dtvflash_store(paddr, value);
        return;
    }
    if (paddr <= 0xffff) {
#ifdef FEATURE_CPUMEMHISTORY
        rptr = _mem_write_tab_ptr[paddr >> 8];
        if ((rptr == ram_store)
            || (rptr == ram_hi_store)
            || (rptr == vicii_mem_vbank_store)
            || (rptr == zero_store)) {
            monitor_memmap_store(paddr, MEMMAP_RAM_W);
        } else {
            monitor_memmap_store(paddr, MEMMAP_I_O_W);
        }
#endif
        /* disable dummy write if skip cycle */
        if (dtv_registers[9] & 1) {
            maincpu_rmw_flag = 0;
        }
        _mem_write_tab_ptr[paddr >> 8]((uint16_t)paddr, value);
    } else {
#ifdef FEATURE_CPUMEMHISTORY
        monitor_memmap_store(paddr, MEMMAP_RAM_W);
#endif
        mem_ram[paddr] = value;
    }
}

uint8_t mem_read(uint16_t addr)
{
#ifdef FEATURE_CPUMEMHISTORY
    read_func_ptr_t rptr;
#endif

    int paddr = addr_to_paddr(addr);
/* if (addr != paddr) printf("Read from adress %x mapped to %x - %d %d %d %d\n", addr, paddr, dtv_registers[12], dtv_registers[13], dtv_registers[14], dtv_registers[15]); */ /* DEBUG */
    if (access_rom(addr)) {
#ifdef FEATURE_CPUMEMHISTORY
        monitor_memmap_store(paddr, (memmap_state & MEMMAP_STATE_OPCODE) ? MEMMAP_ROM_X : (memmap_state & MEMMAP_STATE_INSTR) ? 0 : MEMMAP_ROM_R);
        memmap_state &= ~(MEMMAP_STATE_OPCODE);
#endif
        return c64dtvflash_read(paddr);
    }
    if (paddr <= 0xffff) {
#ifdef FEATURE_CPUMEMHISTORY
        rptr = _mem_read_tab_ptr[paddr >> 8];
        if ((rptr == ram_read)
            || (rptr == zero_read)) {
            monitor_memmap_store(paddr, (memmap_state & MEMMAP_STATE_OPCODE) ? MEMMAP_RAM_X : (memmap_state & MEMMAP_STATE_INSTR) ? 0 : MEMMAP_RAM_R);
        } else if ((rptr == c64memrom_basic64_read)
                   || (rptr == c64memrom_kernal64_read)) {
            monitor_memmap_store(paddr, (memmap_state & MEMMAP_STATE_OPCODE) ? MEMMAP_ROM_X : (memmap_state & MEMMAP_STATE_INSTR) ? 0 : MEMMAP_ROM_R);
        } else {
            monitor_memmap_store(paddr, MEMMAP_I_O_R);
        }
        memmap_state &= ~(MEMMAP_STATE_OPCODE);
#endif
        return _mem_read_tab_ptr[paddr >> 8]((uint16_t)paddr);
    } else {
#ifdef FEATURE_CPUMEMHISTORY
        monitor_memmap_store(paddr, (memmap_state & MEMMAP_STATE_OPCODE) ? MEMMAP_RAM_X : (memmap_state & MEMMAP_STATE_INSTR) ? 0 : MEMMAP_RAM_R);
        memmap_state &= ~(MEMMAP_STATE_OPCODE);
#endif
        return mem_ram[paddr];
    }
}

void colorram_store(uint16_t addr, uint8_t value)
{
    mem_color_ram_cpu[addr & 0x3ff] = value;
}

uint8_t colorram_read(uint16_t addr)
{
    return mem_color_ram_cpu[addr & 0x3ff];
}


/* ------------------------------------------------------------------------- */

#define NUM_TRAP_DEVICES 9  /* FIXME: is there a better constant ? */
static int trapfl[NUM_TRAP_DEVICES];
static int trapdevices[NUM_TRAP_DEVICES + 1] = { 1, 4, 5, 6, 7, 8, 9, 10, 11, -1 };

static void get_trapflags(void)
{
    int i;
    for(i = 0; trapdevices[i] != -1; i++) {
        resources_get_int_sprintf("VirtualDevice%d", &trapfl[i], trapdevices[i]);
    }
}

static void clear_trapflags(void)
{
    int i;
    for(i = 0; trapdevices[i] != -1; i++) {
        resources_set_int_sprintf("VirtualDevice%d", 0, trapdevices[i]);
    }
}

static int restore_trapflags(void)
{
    int i, flags = 0;
    for(i = 0; trapdevices[i] != -1; i++) {
        resources_set_int_sprintf("VirtualDevice%d", trapfl[i], trapdevices[i]);
        flags |= trapfl[i];
    }
    return flags;
}

void c64dtv_init(void)
{
    if (c64dtvmem_log == LOG_ERR) {
        c64dtvmem_log = log_open("C64DTVMEM");
    }

    hummeradc_init();
    c64dtvblitter_init();
    c64dtvdma_init();
    c64dtvflash_init();
    DBG("installing floppy traps");
    /* TODO disable copying by command line parameter */
    /* Make sure serial code traps are in place.  */
    get_trapflags();
    clear_trapflags();
    restore_trapflags();
    /* TODO chargen ROM support */

    DBG("END init");
}

/* init C64DTV memory table changes */
void c64dtvmem_init_config(void)
{
    int i, j, k;

    /* install DMA engine handlers */
    DBG("install mem_read/mem_write handlers");
    for (i = 0; i < NUM_CONFIGS; i++)
    {
        for (j = 1; j <= 0xff; j++)
        {
            for (k = 0; k < NUM_VBANKS; k++)
            {
                if (mem_write_tab[k][i][j] == vicii_store && j == 0xd3) {
                    mem_write_tab[k][i][j] = c64dtv_dmablit_store;
                }
                if (mem_write_tab[k][i][j] == vicii_store && j == 0xd1) {
                    mem_write_tab[k][i][j] = c64dtv_mapper_store;
                }
                if (mem_write_tab[k][i][j] == vicii_store && j == 0xd2) {
                    mem_write_tab[k][i][j] = c64dtv_palette_store;
                }
            }
            if (mem_read_tab[i][j] == vicii_read && j == 0xd3) {
                mem_read_tab[i][j] = c64dtv_dmablit_read;
            }
            if (mem_read_tab[i][j] == vicii_read && j == 0xd1) {
                mem_read_tab[i][j] = c64dtv_mapper_read;
            }
            if (mem_read_tab[i][j] == vicii_read && j == 0xd2) {
                mem_read_tab[i][j] = c64dtv_palette_read;
            }
        }
    }
    DBG("END init_config");
}


void c64dtvmem_shutdown(void)
{
    hummeradc_shutdown();
    c64dtvblitter_shutdown();
    c64dtvdma_shutdown();
    /* work around for non transparent kernal traps.
       Disable serial traps when shutting down c64dtvflash, which
       saves the contents if enabled */
    get_trapflags();
    clear_trapflags();
    c64dtvflash_shutdown();
    restore_trapflags();

    DBG("END shutdown");
}

void c64dtvmem_reset(void)
{
    DBG("reset");

    /* Disable serial traps when resetting mem mapper */
    get_trapflags();
    clear_trapflags();
    c64dtvmem_memmapper[0x00] = 0; /* KERNAL ROM segment (0x10000 byte segments) */
    c64dtvmem_memmapper[0x01] = 0; /* BASIC ROM segment (0x10000 byte segments) */
    restore_trapflags();

    /* TODO move register file initialization somewhere else? */
    dtv_registers[8] = 0x55; /* RAM/ROM access mode */
    dtv_registers[9] = 0; /* skip cycle and burst mode */
    dtv_registers[10] = 0; /* zero page (0x100 byte segments) */
    dtv_registers[11] = 1; /* stack page (0x100 byte segments) */
    dtv_registers[12] = 0; /* bank 0 (0x4000 byte segments) */
    dtv_registers[13] = 1; /* bank 1 */
    dtv_registers[14] = 2; /* bank 2 */
    dtv_registers[15] = 3; /* bank 3 */
    ps2mouse_reset();
    hummeradc_reset();
    c64dtvblitter_reset();
    c64dtvdma_reset();
    c64dtvflash_reset();
}

/* ------------------------------------------------------------------------- */

/* These are the $D100/$D101 memory mapper register handlers */

uint8_t c64dtv_mapper_read(uint16_t addr)
{
    if (!vicii_extended_regs()) {
        return vicii_read(addr);
    }

    return mem_ram[addr];
}

void c64dtv_mapper_store(uint16_t addr, uint8_t value)
{
    if (!vicii_extended_regs()) {
        vicii_store(addr, value);
        return;
    }

    /* always write through to $d100 (this is a hardware bug) */
    mem_ram[addr] = value;

    /* handle aliasing */
    addr &= 0x0f;

    /* DBG("Wrote %d to %x", value, addr); */

    switch (addr) {
        case 0x00:
            /* Deinstall serial traps, change KERNAL segment, reinstall traps */
            get_trapflags();
            clear_trapflags();
            c64dtvmem_memmapper[0] = value;
            maincpu_resync_limits();
            if (restore_trapflags()) {
                log_message(c64dtvmem_log, "Changed KERNAL segment - disable VirtualDevices if you encounter problems");
            }
            break;
        case 0x01:
            c64dtvmem_memmapper[1] = value;
            maincpu_resync_limits();
            break;
        default:
            break;
    }
}


uint8_t c64io1_read(uint16_t addr)
{
    return 0x00;
}

void c64io1_store(uint16_t addr, uint8_t value)
{
}

uint8_t c64io2_read(uint16_t addr)
{
    return 0x00;
}

void c64io2_store(uint16_t addr, uint8_t value)
{
}

/* ------------------------------------------------------------------------- */

/* These are the $D200 palette register handlers */

uint8_t c64dtv_palette_read(uint16_t addr)
{
    if (!vicii_extended_regs()) {
        return vicii_read(addr);
    }

    return vicii_palette_read(addr);
}

void c64dtv_palette_store(uint16_t addr, uint8_t value)
{
    if (!vicii_extended_regs()) {
        vicii_store(addr, value);
        return;
    }

    vicii_palette_store(addr, value);
    return;
}


/* ------------------------------------------------------------------------- */

/* These are the $D300 DMA and blitter register handlers */

uint8_t c64dtv_dmablit_read(uint16_t addr)
{
    if (!vicii_extended_regs()) {
        return vicii_read(addr);
    }

    addr &= 0x3f;

    if (addr & 0x20) {
        return c64dtv_blitter_read((uint16_t)(addr & 0x1f));
    } else {
        return c64dtv_dma_read(addr);
    }
}

void c64dtv_dmablit_store(uint16_t addr, uint8_t value)
{
    if (!vicii_extended_regs()) {
        vicii_store(addr, value);
        return;
    }

    addr &= 0x3f;

    if (addr & 0x20) {
        c64dtv_blitter_store((uint16_t)(addr & 0x1f), value);
    } else {
        c64dtv_dma_store(addr, value);
    }
}



/* ------------------------------------------------------------------------- */

/* Exported banked memory access functions for the monitor.  */
#define MAXBANKS (6 + 0x20 + 0x20)

static const char *banknames[MAXBANKS + 1] =
{
    "default", "cpu", "ram", "rom", "io", "cart",
    /* by convention, a "bank array" has a 2-hex-digit bank index appended */
    "ram00", "ram01", "ram02", "ram03", "ram04", "ram05", "ram06", "ram07",
    "ram08", "ram09", "ram0a", "ram0b", "ram0c", "ram0d", "ram0e", "ram0f",
    "ram10", "ram11", "ram12", "ram13", "ram14", "ram15", "ram16", "ram17",
    "ram18", "ram19", "ram1a", "ram1b", "ram1c", "ram1d", "ram1e", "ram1f",
    "rom00", "rom01", "rom02", "rom03", "rom04", "rom05", "rom06", "rom07",
    "rom08", "rom09", "rom0a", "rom0b", "rom0c", "rom0d", "rom0e", "rom0f",
    "rom10", "rom11", "rom12", "rom13", "rom14", "rom15", "rom16", "rom17",
    "rom18", "rom19", "rom1a", "rom1b", "rom1c", "rom1d", "rom1e", "rom1f",
    NULL
};

static const int banknums[MAXBANKS + 1] =
{
    0, 0, 1, 2, 3, 4,
    5, 6, 7, 8, 9, 10, 11, 12,
    13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28,
    29, 30, 31, 32, 33, 34, 35, 36,
    37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 52,
    53, 54, 55, 56, 57, 58, 59, 60,
    61, 62, 63, 64, 65, 66, 67, 68,
    -1
};

static const int bankindex[MAXBANKS + 1] =
{
    -1, -1, -1, -1, -1, -1,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    -1
};

static const int bankflags[MAXBANKS + 1] =
{
    0, 0, 0, 0, 0, 0,
    MEM_BANK_ISARRAY | MEM_BANK_ISARRAYFIRST, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY | MEM_BANK_ISARRAYLAST,
    MEM_BANK_ISARRAY | MEM_BANK_ISARRAYFIRST, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY,
    MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY, MEM_BANK_ISARRAY | MEM_BANK_ISARRAYLAST,
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
    int paddr;

    if ((bank >= 5) && (bank <= 36)) {
        return mem_ram[((bank - 5) << 16) + addr]; /* ram00..1f */
    }
    if ((bank >= 37) && (bank <= 68)) {
        return c64dtvflash_mem[((bank - 37) << 16) + addr]; /* rom00..1f */
    }

    /* TODO: is restoring r8, r10..15 needed? */
    dtv_registers[8] = MOS6510DTV_REGS_GET_R8(&maincpu_regs);
    dtv_registers[10] = MOS6510DTV_REGS_GET_R10(&maincpu_regs);
    dtv_registers[11] = MOS6510DTV_REGS_GET_R11(&maincpu_regs);
    dtv_registers[12] = MOS6510DTV_REGS_GET_R12(&maincpu_regs);
    dtv_registers[13] = MOS6510DTV_REGS_GET_R13(&maincpu_regs);
    dtv_registers[14] = MOS6510DTV_REGS_GET_R14(&maincpu_regs);
    dtv_registers[15] = MOS6510DTV_REGS_GET_R15(&maincpu_regs);

    paddr = addr_to_paddr(addr);
    switch (bank) {
        case 0:                 /* current */
            return mem_read(addr);
        case 3:                 /* io */
            if (paddr >= 0xd000 && paddr < 0xe000) {
                return read_bank_io((uint16_t)paddr);
            }
        case 4:                 /* cart */
            break;
        case 2:                 /* rom */
            if (paddr >= 0xa000 && paddr <= 0xbfff) {
                return c64memrom_basic64_read((uint16_t)paddr);
            }
            if (paddr >= 0xd000 && paddr <= 0xdfff) {
                return chargen_read((uint16_t)paddr);
            }
            if (paddr >= 0xe000) {
                return c64memrom_kernal64_read((uint16_t)paddr);
            }
        case 1:                 /* ram */
            break; /* yes, this could be flash as well */
    }
    return access_rom(addr) ? c64dtvflash_read(paddr) : mem_ram[paddr];
}

/* used by monitor if sfx off */
uint8_t mem_bank_peek(int bank, uint16_t addr, void *context)
{
    int paddr;
    if (bank >= 5) {
        return mem_bank_read(bank, addr, context);
    }
    /* Commented out to see if that fixes bug #1266. Any memory peek code
     * should not alter the state of the machine.  -- compyx
     */
#if 0
    /* TODO: is restoring r8, r10..15 needed? */
    dtv_registers[8] = MOS6510DTV_REGS_GET_R8(&maincpu_regs);
    dtv_registers[10] = MOS6510DTV_REGS_GET_R10(&maincpu_regs);
    dtv_registers[11] = MOS6510DTV_REGS_GET_R11(&maincpu_regs);
    dtv_registers[12] = MOS6510DTV_REGS_GET_R12(&maincpu_regs);
    dtv_registers[13] = MOS6510DTV_REGS_GET_R13(&maincpu_regs);
    dtv_registers[14] = MOS6510DTV_REGS_GET_R14(&maincpu_regs);
    dtv_registers[15] = MOS6510DTV_REGS_GET_R15(&maincpu_regs);
#endif
    paddr = addr_to_paddr(addr);
    switch (bank) {
        case 0:                 /* current */
            if (access_rom(addr)) {
                return c64dtvflash_mem[paddr];
            }
            if (paddr <= 0xffff) {
                if (c64dtvmeminit_io_config[mem_config]) {
                    if ((paddr >= 0xd000) && (paddr < 0xe000)) {
                        return peek_bank_io((uint16_t)paddr);
                    }
                }
                if (_mem_read_tab_ptr[paddr >> 8] == c64memrom_basic64_read) {
                    int mapping = c64dtvmem_memmapper[1];
                    paddr |= (mapping & 0x1f) << 16;
                    return (mapping & 0xc0) ? mem_ram[paddr] : c64dtvflash_mem[paddr];
                }
                if (_mem_read_tab_ptr[paddr >> 8] == chargen_read) {
                    return c64dtvflash_mem[paddr];
                }
                if (_mem_read_tab_ptr[paddr >> 8] == c64memrom_kernal64_read) {
                    int mapping = c64dtvmem_memmapper[0];
                    paddr |= (mapping & 0x1f) << 16;
                    return (mapping & 0xc0) ? mem_ram[paddr] : c64dtvflash_mem[paddr];
                } /* no side effects on the rest */
                return _mem_read_tab_ptr[paddr >> 8]((uint16_t)paddr);
            }
            return mem_ram[paddr];
        case 3:                 /* io */
            if (paddr >= 0xd000 && paddr < 0xe000) {
                return peek_bank_io((uint16_t)paddr);
            }
        case 4:                 /* cart */
            break;
        case 2:                 /* rom */
            if (paddr >= 0xa000 && paddr <= 0xbfff) {
                int mapping = c64dtvmem_memmapper[1];
                paddr += ((mapping & 0x1f) << 16);
                return ((mapping >> 6) == 0) ? c64dtvflash_mem[paddr] : mem_ram[paddr];
            }
            if (paddr >= 0xd000 && paddr <= 0xdfff) {
                return c64dtvflash_mem[paddr];
            }
            if (paddr >= 0xe000) {
                int mapping = c64dtvmem_memmapper[0];
                paddr += ((mapping & 0x1f) << 16);
                return ((mapping >> 6) == 0) ? c64dtvflash_mem[paddr] : mem_ram[paddr];
            }
        case 1:                 /* ram */
            break; /* yes, this could be flash as well */
    }
    return access_rom(addr) ? c64dtvflash_mem[paddr] : mem_ram[paddr];
}

void mem_bank_write(int bank, uint16_t addr, uint8_t byte, void *context)
{
    int paddr;

    if ((bank >= 5) && (bank <= 36)) { /* ram00..1f */
        mem_ram[((bank - 5) << 16) + addr] = byte;
        return;
    }

    if ((bank >= 37) && (bank <= 68)) {
        c64dtvflash_mem[((bank - 37) << 16) + addr] = byte; /* rom00..1f */
        return;
    }

    /* TODO: is restoring r8, r10..15 needed? */
    dtv_registers[8] = MOS6510DTV_REGS_GET_R8(&maincpu_regs);
    dtv_registers[10] = MOS6510DTV_REGS_GET_R10(&maincpu_regs);
    dtv_registers[11] = MOS6510DTV_REGS_GET_R11(&maincpu_regs);
    dtv_registers[12] = MOS6510DTV_REGS_GET_R12(&maincpu_regs);
    dtv_registers[13] = MOS6510DTV_REGS_GET_R13(&maincpu_regs);
    dtv_registers[14] = MOS6510DTV_REGS_GET_R14(&maincpu_regs);
    dtv_registers[15] = MOS6510DTV_REGS_GET_R15(&maincpu_regs);

    paddr = addr_to_paddr(addr);
    switch (bank) {
        case 0:                 /* current */
            mem_store(addr, byte);
            return;
        case 3:                 /* io */
            if (paddr >= 0xd000 && paddr < 0xe000) {
                store_bank_io((uint16_t)paddr, byte);
                return;
            }
        case 4:                 /* cart */
            break;
        case 2:                 /* rom */
            if (paddr >= 0xa000 && paddr <= 0xbfff) {
                return;
            }
            if (paddr >= 0xd000 && paddr <= 0xdfff) {
                return;
            }
            if (paddr >= 0xe000) {
                return;
            }
        case 1:                 /* ram */
            break; /* yes, this could be flash as well */
    }
    if (access_rom(addr)) {
        c64dtvflash_mem[paddr] = byte;
    } else {
        mem_ram[paddr] = byte;
    }
}

/* used by monitor if sfx off */
void mem_bank_poke(int bank, uint16_t addr, uint8_t byte, void *context)
{
    mem_bank_write(bank, addr, byte, context);
}

/* ------------------------------------------------------------------------- */

int c64dtvmem_resources_init(void)
{
    return c64dtvblitter_resources_init() < 0 ||
           c64dtvdma_resources_init() < 0 ||
           c64dtvflash_resources_init() < 0;
}

void c64dtvmem_resources_shutdown(void)
{
    c64dtvblitter_resources_shutdown();
    c64dtvdma_resources_shutdown();
    c64dtvflash_resources_shutdown();
}


int c64dtvmem_cmdline_options_init(void)
{
    return c64dtvblitter_cmdline_options_init() < 0 ||
           c64dtvdma_cmdline_options_init() < 0 ||
           c64dtvflash_cmdline_options_init() < 0;
}
