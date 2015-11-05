/*
 * vic20mem.c -- VIC20 memory handling.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Daniel Kahlin <daniel@kahlin.net>
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
BYTE mem_ram[VIC20_RAM_SIZE];

/* Last data read/write by the cpu, this value lingers on the C(PU)-bus and
   gets used when the CPU reads from unconnected space on the C(PU)-bus */
BYTE vic20_cpu_last_data;
/* Last read data on V-bus (VD0-VD7) */
BYTE vic20_v_bus_last_data;
/* Last read data on V-bus (VD8-VD11) */
BYTE vic20_v_bus_last_high;

/* Memory read and write tables.  */
static BYTE *_mem_read_base_tab[0x101];
static int mem_read_limit_tab[0x101];

/* These ones are used when watchpoints are turned on.  */
static read_func_ptr_t _mem_read_tab_watch[0x101];
static store_func_ptr_t _mem_write_tab_watch[0x101];

static read_func_ptr_t _mem_read_tab_nowatch[0x101];
static store_func_ptr_t _mem_write_tab_nowatch[0x101];
static read_func_ptr_t _mem_peek_tab[0x101];

read_func_ptr_t *_mem_read_tab_ptr;
store_func_ptr_t *_mem_write_tab_ptr;
static BYTE **_mem_read_base_tab_ptr;
static int *mem_read_limit_tab_ptr;

/* Current watchpoint state. 1 = watchpoints active, 0 = no watchpoints */
static int watchpoints_active = 0;

/* ------------------------------------------------------------------------- */

BYTE zero_read(WORD addr)
{
    vic20_cpu_last_data = mem_ram[addr & 0xff];
    vic20_mem_v_bus_read(addr);
    return vic20_cpu_last_data;
}

void zero_store(WORD addr, BYTE value)
{
    vic20_cpu_last_data = value;
    vic20_mem_v_bus_store(addr);
    mem_ram[addr & 0xff] = value;
}

static BYTE ram_read(WORD addr)
{
    vic20_cpu_last_data = mem_ram[addr];
    return vic20_cpu_last_data;
}

static BYTE ram_read_v_bus(WORD addr)
{
    vic20_cpu_last_data = mem_ram[addr];
    vic20_mem_v_bus_read(addr);
    return vic20_cpu_last_data;
}

static void ram_store(WORD addr, BYTE value)
{
    vic20_cpu_last_data = value;
    mem_ram[addr & (VIC20_RAM_SIZE - 1)] = value;
}

static void ram_store_v_bus(WORD addr, BYTE value)
{
    vic20_cpu_last_data = value;
    vic20_mem_v_bus_store(addr);
    mem_ram[addr & (VIC20_RAM_SIZE - 1)] = value;
}

static BYTE ram_peek(WORD addr)
{
    return mem_ram[addr];
}

/* ------------------------------------------------------------------------- */

static BYTE colorram_read(WORD addr)
{
    vic20_cpu_last_data = mem_ram[addr] | (vic20_v_bus_last_data & 0xf0);
    vic20_v_bus_last_data = vic20_cpu_last_data; /* TODO verify this */
    return vic20_cpu_last_data;
}

static void colorram_store(WORD addr, BYTE value)
{
    vic20_cpu_last_data = value;
    vic20_v_bus_last_data = vic20_cpu_last_data; /* TODO verify this */
    mem_ram[addr & (VIC20_RAM_SIZE - 1)] = value & 0xf;
}

static BYTE colorram_peek(WORD addr)
{
    return mem_ram[addr] | (vic20_v_bus_last_data & 0xf0);
}

/* ------------------------------------------------------------------------- */

static void via_store(WORD addr, BYTE value)
{
    vic20_cpu_last_data = value;

    if (addr & 0x10) {          /* $911x (VIA2) */
        via2_store(addr, value);
    }
    if (addr & 0x20) {          /* $912x (VIA1) */
        via1_store(addr, value);
    }
    vic20_mem_v_bus_store(addr);
}

static BYTE via_read(WORD addr)
{
    if ((addr & 0x30) == 0x00) {    /* $910x (unconnected V-bus) */
        vic20_cpu_last_data = vic20_v_bus_last_data;
    } else {
        BYTE temp_bus = 0xff;

        if (addr & 0x10) {          /* $911x (VIA2) */
            temp_bus &= via2_read(addr);
        }
        if (addr & 0x20) {          /* $912x (VIA1) */
            temp_bus &= via1_read(addr);
        }
        vic20_cpu_last_data = temp_bus;
    }
    vic20_mem_v_bus_read(addr);
    return vic20_cpu_last_data;
}

static BYTE via_peek(WORD addr)
{
    if ((addr & 0x30) == 0x00) {  /* $910x (unconnected V-bus) */
        return vic20_v_bus_last_data;
    } else {
        BYTE temp_bus = 0xff;

        if (addr & 0x10) {          /* $911x (VIA2) */
            temp_bus &= via2_read(addr);
        }
        if (addr & 0x20) {          /* $912x (VIA1) */
            temp_bus &= via1_read(addr);
        }
        return temp_bus;
    }
}

/*-------------------------------------------------------------------*/

static BYTE io3_peek(WORD addr)
{
#if 0
    /* TODO */
    if (sidcart_enabled && sidcart_address == 1 && addr >= 0x9c00 && addr <= 0x9c1f) {
        return sid_peek(addr);
    }
#endif

#ifdef HAVE_MIDI
#if 0
    /* TODO */
    if (midi_enabled && (addr & 0xff00) == 0x9c00) {
        if (midi_test_peek((WORD)(addr & 0xff))) {
            return midi_peek((WORD)(addr & 0xff));
        }
    }
#endif
#endif

    return vic20_v_bus_last_data;
}

static BYTE io2_peek(WORD addr)
{
#if 0
    /* TODO */
    if (sidcart_enabled && sidcart_address == 0 && addr >= 0x9800 && addr <= 0x981f) {
        return sid_peek(addr);
    }
#endif

    return vic20_v_bus_last_data;
}

/*-------------------------------------------------------------------*/

static BYTE chargen_read(WORD addr)
{
    vic20_cpu_last_data = vic20memrom_chargen_read(addr);
    vic20_mem_v_bus_read(addr);
    return vic20_cpu_last_data;
}

static BYTE chargen_peek(WORD addr)
{
    return vic20memrom_chargen_read(addr);
}

/*-------------------------------------------------------------------*/

static BYTE read_unconnected_v_bus(WORD addr)
{
    vic20_cpu_last_data = vic20_v_bus_last_data;
    vic20_mem_v_bus_read(addr);
    return vic20_cpu_last_data;
}

static BYTE read_unconnected_c_bus(WORD addr)
{
    return vic20_cpu_last_data;
}

static void store_dummy_v_bus(WORD addr, BYTE value)
{
    vic20_cpu_last_data = value;
    vic20_mem_v_bus_store(addr);
}

static void store_dummy_c_bus(WORD addr, BYTE value)
{
    vic20_cpu_last_data = value;
}

static BYTE peek_unconnected_v_bus(WORD addr)
{
    return vic20_v_bus_last_data;
}

static BYTE peek_unconnected_c_bus(WORD addr)
{
    return vic20_cpu_last_data;
}

/*-------------------------------------------------------------------*/
/* Watchpoint functions */

static BYTE zero_read_watch(WORD addr)
{
    addr &= 0xff;
    monitor_watch_push_load_addr(addr, e_comp_space);
    return _mem_read_tab_nowatch[0](addr);
}

static void zero_store_watch(WORD addr, BYTE value)
{
    addr &= 0xff;
    monitor_watch_push_store_addr(addr, e_comp_space);
    _mem_write_tab_nowatch[0](addr, value);
}

static BYTE read_watch(WORD addr)
{
    monitor_watch_push_load_addr(addr, e_comp_space);
    return _mem_read_tab_nowatch[addr >> 8](addr);
}

static void store_watch(WORD addr, BYTE value)
{
    monitor_watch_push_store_addr(addr, e_comp_space);
    _mem_write_tab_nowatch[addr >> 8](addr, value);
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

BYTE mem_peek(WORD addr)
{
    return _mem_peek_tab[addr >> 8](addr);
}

/* ------------------------------------------------------------------------- */

static void set_mem(int start_page, int end_page,
                    read_func_ptr_t read_func,
                    store_func_ptr_t store_func,
                    read_func_ptr_t peek_func,
                    BYTE *read_base, int base_mask)
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

    /* Setup VIC-I at $9000-$90FF. */
    set_mem(0x90, 0x90,
            vic_read, vic_store, vic_peek,
            NULL, 0);

    /* Setup VIAs at $9100-$93FF. */
    set_mem(0x91, 0x93,
            via_read, via_store, via_peek,
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

    /* Setup I/O3 at the expansion port (includes emulator ID) */
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

void mem_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
{
    BYTE *p = _mem_read_base_tab_ptr[addr >> 8];

    *base = (p == NULL) ? NULL : (p - (addr & 0xff00));
    *start = addr; /* TODO */
    *limit = mem_read_limit_tab_ptr[addr >> 8];
}

void mem_toggle_watchpoints(int flag, void *context)
{
    if (flag) {
        _mem_read_tab_ptr = _mem_read_tab_watch;
        _mem_write_tab_ptr = _mem_write_tab_watch;
    } else {
        _mem_read_tab_ptr = _mem_read_tab_nowatch;
        _mem_write_tab_ptr = _mem_write_tab_nowatch;
    }
    watchpoints_active = flag;
}

/* ------------------------------------------------------------------------- */

/* Initialize RAM for power-up.  */
void mem_powerup(void)
{
    ram_init(mem_ram, 0x8000);
    memset(mem_ram + 0x8000, 0, 0x8000);
}

/* ------------------------------------------------------------------------- */

/* FIXME: this part needs to be checked. */

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

/* ------------------------------------------------------------------------- */

int mem_rom_trap_allowed(WORD addr)
{
    return addr >= 0xe000;
}

void mem_inject(DWORD addr, BYTE value)
{
    /* just call mem_store(), otherwise expansions might fail */
    mem_store((WORD)(addr & 0xffff), value);
}

/* ------------------------------------------------------------------------- */

/* Banked memory access functions for the monitor */

/* Exported banked memory access functions for the monitor */

static const char *banknames[] = { "default", "cpu", NULL };

static const int banknums[] = { 0, 0 };

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
    return mem_read(addr);
}

BYTE mem_bank_peek(int bank, WORD addr, void *context)
{
    return mem_peek(addr);
}

void mem_bank_write(int bank, WORD addr, BYTE byte, void *context)
{
    mem_store(addr, byte);
}

/* FIXME: add other i/o extensions here */
static int mem_dump_io(WORD addr)
{
    if ((addr >= 0x9000) && (addr <= 0x900f)) {
        return vic_dump();
    } else if ((addr >= 0x9120) && (addr <= 0x912f)) {
        return viacore_dump(machine_context.via1);
    } else if ((addr >= 0x9110) && (addr <= 0x911f)) {
        return viacore_dump(machine_context.via2);
    }
    return -1;
}

mem_ioreg_list_t *mem_ioreg_list_get(void *context)
{
    mem_ioreg_list_t *mem_ioreg_list = NULL;

    mon_ioreg_add_list(&mem_ioreg_list, "VIC", 0x9000, 0x900f, mem_dump_io);
    mon_ioreg_add_list(&mem_ioreg_list, "VIA1", 0x9120, 0x912f, mem_dump_io);
    mon_ioreg_add_list(&mem_ioreg_list, "VIA2", 0x9110, 0x911f, mem_dump_io);

    io_source_ioreg_add_list(&mem_ioreg_list);

    return mem_ioreg_list;
}

void mem_get_screen_parameter(WORD *base, BYTE *rows, BYTE *columns, int *bank)
{
    *base = ((vic_peek(0x9005) & 0x80) ? 0 : 0x8000) + ((vic_peek(0x9005) & 0x70) << 6) + ((vic_peek(0x9002) & 0x80) << 2);
    *rows = (vic_peek(0x9003) & 0x7e) >> 1;
    *columns = vic_peek(0x9002) & 0x7f;
    *bank = 0;
}

