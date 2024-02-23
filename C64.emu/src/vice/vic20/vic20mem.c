/*
 * vic20mem.c -- VIC20 memory handling.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Daniel Kahlin <daniel@kahlin.net>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Multiple memory configuration support originally by
 *  Alexander Lehmann <alex@mathematik.th-darmstadt.de>
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

#include "cartio.h"
#include "cartridge.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "ram.h"
#include "resources.h"
#include "sid-resources.h"
#include "sid.h"
#include "types.h"
#include "uiapi.h"
#include "via.h"
#include "vic.h"
#include "vic-mem.h"
#include "vic20.h"
#include "vic20-resources.h"
#include "vic20cartmem.h"
#include "vic20ieeevia.h"
#include "vic20mem.h"
#include "vic20memrom.h"
#include "vic20rom.h"
#include "vic20via.h"

/* ------------------------------------------------------------------------- */

/* The VIC20 memory. */
uint8_t mem_ram[VIC20_RAM_SIZE];

uint8_t vfli_ram[0x4000]; /* for mikes vfli modification */

/* Last data read/write by the cpu, this value lingers on the C(PU)-bus and
   gets used when the CPU reads from unconnected space on the C(PU)-bus */
uint8_t vic20_cpu_last_data;
/* Last read data on V-bus (VD0-VD7) */
uint8_t vic20_v_bus_last_data;
/* Last read data on V-bus (VD8-VD11) */
uint8_t vic20_v_bus_last_high;

/* Memory read and write tables.  */
static uint8_t *_mem_read_base_tab[0x101];
static int mem_read_limit_tab[0x101];

/* These ones are used when watchpoints are turned on.  */
static read_func_ptr_t _mem_read_tab_watch[0x101];
static store_func_ptr_t _mem_write_tab_watch[0x101];

static read_func_ptr_t _mem_read_tab_nowatch[0x101];
static store_func_ptr_t _mem_write_tab_nowatch[0x101];
static read_func_ptr_t _mem_peek_tab[0x101];

read_func_ptr_t *_mem_read_tab_ptr;
store_func_ptr_t *_mem_write_tab_ptr;
read_func_ptr_t *_mem_read_tab_ptr_dummy;
store_func_ptr_t *_mem_write_tab_ptr_dummy;
static uint8_t **_mem_read_base_tab_ptr;
static int *mem_read_limit_tab_ptr;

/* Current watchpoint state.
          0 = no watchpoints
    bit0; 1 = watchpoints active
    bit1; 2 = watchpoints trigger on dummy accesses
*/
static int watchpoints_active = 0;

/* ------------------------------------------------------------------------- */

uint8_t zero_read(uint16_t addr)
{
    vic20_cpu_last_data = mem_ram[addr & 0xff];
    vic20_mem_v_bus_read(addr);
    return vic20_cpu_last_data;
}

void zero_store(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
    vic20_mem_v_bus_store(addr);
    mem_ram[addr & 0xff] = value;
}

static uint8_t ram_read(uint16_t addr)
{
    vic20_cpu_last_data = mem_ram[addr];
    return vic20_cpu_last_data;
}

static uint8_t ram_read_v_bus(uint16_t addr)
{
    vic20_cpu_last_data = mem_ram[addr];
    vic20_mem_v_bus_read(addr);
    return vic20_cpu_last_data;
}

static void ram_store(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
    mem_ram[addr & (VIC20_RAM_SIZE - 1)] = value;
}

static void ram_store_v_bus(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
    vic20_mem_v_bus_store(addr);
    mem_ram[addr & (VIC20_RAM_SIZE - 1)] = value;
}

static uint8_t ram_peek(uint16_t addr)
{
    return mem_ram[addr];
}

/* ------------------------------------------------------------------------- */

static uint8_t colorram_read(uint16_t addr)
{
    if (vflimod_enabled) {
        addr = (addr & 0x3ff) | (vic20_vflihack_userport << 10);
        vic20_cpu_last_data = vfli_ram[addr] | (vic20_v_bus_last_data & 0xf0);
    } else {
        vic20_cpu_last_data = mem_ram[addr] | (vic20_v_bus_last_data & 0xf0);
    }
    vic20_v_bus_last_data = vic20_cpu_last_data; /* TODO verify this */
    return vic20_cpu_last_data;
}

static void colorram_store(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
    vic20_v_bus_last_data = vic20_cpu_last_data; /* TODO verify this */
    if (vflimod_enabled) {
        addr = (addr & 0x3ff) | (vic20_vflihack_userport << 10);
        vfli_ram[addr] = value & 0xf;
    } else {
        mem_ram[addr & (VIC20_RAM_SIZE - 1)] = value & 0xf;
    }
}

static uint8_t colorram_peek(uint16_t addr)
{
    return mem_ram[addr] | (vic20_v_bus_last_data & 0xf0);
}

/* ------------------------------------------------------------------------- */

static uint8_t io3_peek(uint16_t addr)
{
    return vic20io3_peek(addr);
}

static uint8_t io2_peek(uint16_t addr)
{
    return vic20io2_peek(addr);
}

static uint8_t io0_peek(uint16_t addr)
{
    return vic20io0_peek(addr);
}

/*-------------------------------------------------------------------*/

static uint8_t chargen_read(uint16_t addr)
{
    vic20_cpu_last_data = vic20memrom_chargen_read(addr);
    vic20_mem_v_bus_read(addr);
    return vic20_cpu_last_data;
}

static uint8_t chargen_peek(uint16_t addr)
{
    return vic20memrom_chargen_read(addr);
}

/*-------------------------------------------------------------------*/

static uint8_t read_unconnected_v_bus(uint16_t addr)
{
    vic20_cpu_last_data = vic20_v_bus_last_data;
    vic20_mem_v_bus_read(addr);
    return vic20_cpu_last_data;
}

static uint8_t read_unconnected_c_bus(uint16_t addr)
{
    return vic20_cpu_last_data;
}

static void store_dummy_v_bus(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
    vic20_mem_v_bus_store(addr);
}

static void store_dummy_c_bus(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
}

static uint8_t peek_unconnected_v_bus(uint16_t addr)
{
    return vic20_v_bus_last_data;
}

static uint8_t peek_unconnected_c_bus(uint16_t addr)
{
    return vic20_cpu_last_data;
}

/*-------------------------------------------------------------------*/
/* Watchpoint functions */

static uint8_t zero_read_watch(uint16_t addr)
{
    addr &= 0xff;
    monitor_watch_push_load_addr(addr, e_comp_space);
    return _mem_read_tab_nowatch[0](addr);
}

static void zero_store_watch(uint16_t addr, uint8_t value)
{
    addr &= 0xff;
    monitor_watch_push_store_addr(addr, e_comp_space);
    _mem_write_tab_nowatch[0](addr, value);
}

static uint8_t read_watch(uint16_t addr)
{
    monitor_watch_push_load_addr(addr, e_comp_space);
    return _mem_read_tab_nowatch[addr >> 8](addr);
}

static void store_watch(uint16_t addr, uint8_t value)
{
    monitor_watch_push_store_addr(addr, e_comp_space);
    _mem_write_tab_nowatch[addr >> 8](addr, value);
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

static uint8_t mem_peek(uint16_t addr)
{
    return _mem_peek_tab[addr >> 8](addr);
}

/* ------------------------------------------------------------------------- */

static void set_mem(int start_page, int end_page,
                    read_func_ptr_t read_func,
                    store_func_ptr_t store_func,
                    read_func_ptr_t peek_func,
                    uint8_t *read_base, int base_mask)
{
    int i;

    if (read_base != NULL) {
        for (i = start_page; i <= end_page; i++) {
            _mem_read_tab_nowatch[i] = read_func;
            _mem_write_tab_nowatch[i] = store_func;
            _mem_peek_tab[i] = peek_func;
            _mem_read_base_tab[i] = read_base + ((i << 8) & base_mask);
            mem_read_limit_tab[i] = (end_page << 8) + 0xfd;
        }
    } else {
        for (i = start_page; i <= end_page; i++) {
            _mem_read_tab_nowatch[i] = read_func;
            _mem_write_tab_nowatch[i] = store_func;
            _mem_peek_tab[i] = peek_func;
            _mem_read_base_tab[i] = NULL;
            mem_read_limit_tab[i] = 0;
        }
    }
}

int vic20_mem_enable_ram_block(int num)
{
    if (num == 0) {
        set_mem(0x04, 0x0f,
                ram_read_v_bus, ram_store_v_bus, ram_peek,
                NULL, 0);
        return 0;
    } else {
        if (num > 0 && num != 4 && num <= 5) {
            set_mem(num * 0x20, num * 0x20 + 0x1f,
                    ram_read, ram_store, ram_peek,
                    NULL, 0);
            return 0;
        }
    }
    return -1;
}

int vic20_mem_disable_ram_block(int num)
{
    if (num == 0) {
        set_mem(0x04, 0x0f,
                read_unconnected_v_bus, store_dummy_v_bus, peek_unconnected_v_bus,
                NULL, 0);
        return 0;
    } else {
        if (num > 0 && num != 4 && num <= 5) {
            set_mem(num * 0x20, num * 0x20 + 0x1f,
                    read_unconnected_c_bus, store_dummy_c_bus, peek_unconnected_c_bus,
                    NULL, 0);
            return 0;
        }
    }
    return -1;
}

void mem_initialize_memory(void)
{
    int i;

    /* Setup zero page at $0000-$00FF. */
    set_mem(0x00, 0x00,
            zero_read, zero_store, ram_peek,
            NULL, 0);

    /* Setup low standard RAM at $0100-$03FF. */
    set_mem(0x01, 0x03,
            ram_read_v_bus, ram_store_v_bus, ram_peek,
            NULL, 0);

    /* Setup more low RAM at $1000-$1FFF.  */
    set_mem(0x10, 0x1f,
            ram_read_v_bus, ram_store_v_bus, ram_peek,
            NULL, 0);

    if (mem_cart_blocks & VIC_CART_RAM123) {
        /* a cartridge is selected, map everything to cart/vic20cartmem.c */
        set_mem(0x04, 0x0f,
                cartridge_read_ram123, cartridge_store_ram123, cartridge_peek_ram123,
                NULL, 0);
    } else {
        /* Setup RAM at $0400-$0FFF.  */
        if (ram_block_0_enabled) {
            vic20_mem_enable_ram_block(0);
        } else {
            vic20_mem_disable_ram_block(0);
        }
    }

    if (mem_cart_blocks & VIC_CART_BLK1) {
        /* a cartridge is selected, map everything to cart/vic20cartmem.c */
        set_mem(0x20, 0x3f,
                cartridge_read_blk1, cartridge_store_blk1, cartridge_peek_blk1,
                NULL, 0);
    } else {
        /* Setup RAM at $2000-$3FFF.  */
        if (ram_block_1_enabled) {
            vic20_mem_enable_ram_block(1);
        } else {
            vic20_mem_disable_ram_block(1);
        }
    }

    if (mem_cart_blocks & VIC_CART_BLK2) {
        /* a cartridge is selected, map everything to cart/vic20cartmem.c */
        set_mem(0x40, 0x5f,
                cartridge_read_blk2, cartridge_store_blk2, cartridge_peek_blk2,
                NULL, 0);
    } else {
        /* Setup RAM at $4000-$5FFF.  */
        if (ram_block_2_enabled) {
            vic20_mem_enable_ram_block(2);
        } else {
            vic20_mem_disable_ram_block(2);
        }
    }

    if (mem_cart_blocks & VIC_CART_BLK3) {
        /* a cartridge is selected, map everything to cart/vic20cartmem.c */
        set_mem(0x60, 0x7f,
                cartridge_read_blk3, cartridge_store_blk3, cartridge_peek_blk3,
                NULL, 0);
    } else {
        /* Setup RAM at $6000-$7FFF.  */
        if (ram_block_3_enabled) {
            vic20_mem_enable_ram_block(3);
        } else {
            vic20_mem_disable_ram_block(3);
        }
    }

    if (mem_cart_blocks & VIC_CART_BLK5) {
        /* a cartridge is selected, map everything to cart/vic20cartmem.c */
        set_mem(0xa0, 0xbf,
                cartridge_read_blk5, cartridge_store_blk5, cartridge_peek_blk5,
                NULL, 0);
    } else {
        /* Setup RAM at $A000-$BFFF.  */
        if (ram_block_5_enabled) {
            vic20_mem_enable_ram_block(5);
        } else {
            vic20_mem_disable_ram_block(5);
        }
    }

    /* Setup character generator ROM at $8000-$8FFF. */
    set_mem(0x80, 0x8f,
            chargen_read, store_dummy_v_bus, chargen_peek,
            NULL, 0);

    /* Setup I/O0 */
    set_mem(0x90, 0x93,
            vic20io0_read, vic20io0_store, io0_peek,
            NULL, 0);

    /* Setup color memory at $9400-$97FF.
       Warning: we use a kludge here.  Instead of mapping the color memory
       separately, we map it directly in the corresponding RAM address
       space. */
    set_mem(0x94, 0x97,
            colorram_read, colorram_store, colorram_peek,
            NULL, 0);

    /* Setup I/O2 at the expansion port */
    set_mem(0x98, 0x9b,
            vic20io2_read, vic20io2_store, io2_peek,
            NULL, 0);

    /* Setup I/O3 at the expansion port */
    set_mem(0x9c, 0x9f,
            vic20io3_read, vic20io3_store, io3_peek,
            NULL, 0);

    /* Setup BASIC ROM at $C000-$DFFF. */
    set_mem(0xc0, 0xdf,
            vic20memrom_basic_read, store_dummy_c_bus, vic20memrom_basic_read,
            NULL, 0);

    /* Setup Kernal ROM at $E000-$FFFF. */
    set_mem(0xe0, 0xff,
            vic20memrom_kernal_read, store_dummy_c_bus, vic20memrom_kernal_read,
            vic20memrom_kernal_trap_rom, 0x1fff);

    _mem_read_tab_nowatch[0x100] = _mem_read_tab_nowatch[0];
    _mem_write_tab_nowatch[0x100] = _mem_write_tab_nowatch[0];
    _mem_peek_tab[0x100] = _mem_peek_tab[0];
    _mem_read_base_tab[0x100] = _mem_read_base_tab[0];
    mem_read_limit_tab[0x100] = 0;

    _mem_read_base_tab_ptr = _mem_read_base_tab;
    mem_read_limit_tab_ptr = mem_read_limit_tab;

    /* setup watchpoint tables */
    _mem_read_tab_watch[0] = zero_read_watch;
    _mem_write_tab_watch[0] = zero_store_watch;
    for (i = 1; i <= 0x100; i++) {
        _mem_read_tab_watch[i] = read_watch;
        _mem_write_tab_watch[i] = store_watch;
    }

    mem_toggle_watchpoints(watchpoints_active, NULL);
    maincpu_resync_limits();
}

void mem_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
    uint8_t *p = _mem_read_base_tab_ptr[addr >> 8];

    *base = (p == NULL) ? NULL : (p - (addr & 0xff00));
    *start = addr; /* TODO */
    *limit = mem_read_limit_tab_ptr[addr >> 8];
}

void mem_toggle_watchpoints(int flag, void *context)
{
    if (flag) {
        _mem_read_tab_ptr = _mem_read_tab_watch;
        _mem_write_tab_ptr = _mem_write_tab_watch;
        if (flag > 1) {
            /* enable watchpoints on dummy accesses */
            _mem_read_tab_ptr_dummy = _mem_read_tab_watch;
            _mem_write_tab_ptr_dummy = _mem_write_tab_watch;
        } else {
            _mem_read_tab_ptr_dummy = _mem_read_tab_nowatch;
            _mem_write_tab_ptr_dummy = _mem_write_tab_nowatch;
        }
    } else {
        /* all watchpoints disabled */
        _mem_read_tab_ptr = _mem_read_tab_nowatch;
        _mem_write_tab_ptr = _mem_write_tab_nowatch;
        _mem_read_tab_ptr_dummy = _mem_read_tab_nowatch;
        _mem_write_tab_ptr_dummy = _mem_write_tab_nowatch;
    }
    watchpoints_active = flag;
}
/* ------------------------------------------------------------------------- */

/* Initialize RAM for power-up.  */
void mem_powerup(void)
{
    ram_init(mem_ram, 0x8000);
    ram_init(vfli_ram, 0x4000);
    memset(mem_ram + 0x8000, 0, 0x8000);
}

/* ------------------------------------------------------------------------- */

/* FIXME: this part needs to be checked. */

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

/* ------------------------------------------------------------------------- */

int mem_rom_trap_allowed(uint16_t addr)
{
    return addr >= 0xe000;
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
    /* just call mem_store(), otherwise expansions might fail */
    mem_store((uint16_t)(addr & 0xffff), value);
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

/* Banked memory access functions for the monitor */

/* Exported banked memory access functions for the monitor */
#define MAXBANKS (2)

/* by convention, a "bank array" has a 2-hex-digit bank index appended */
static const char *banknames[MAXBANKS + 1] = { "default", "cpu", NULL };

static const int banknums[MAXBANKS + 1] = { 0, 0, -1 };
static const int bankindex[MAXBANKS + 1] = { -1, -1, -1 };
static const int bankflags[MAXBANKS + 1] = { 0, 0, -1 };

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
    return mem_read(addr);
}

/* used by monitor if sfx off */
uint8_t mem_bank_peek(int bank, uint16_t addr, void *context)
{
    return mem_peek(addr);
}

int mem_get_current_bank_config(void) {
    return 0;
}

uint8_t mem_peek_with_config(int config, uint16_t addr, void *context) {
    return mem_bank_peek(0 /* current */, addr, context);
}

void mem_bank_write(int bank, uint16_t addr, uint8_t byte, void *context)
{
    mem_store(addr, byte);
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
    *base = ((vic_peek(0x9005) & 0x80) ? 0 : 0x8000) + ((vic_peek(0x9005) & 0x70) << 6) + ((vic_peek(0x9002) & 0x80) << 2);
    *rows = (vic_peek(0x9003) & 0x7e) >> 1;
    *columns = vic_peek(0x9002) & 0x7f;
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
