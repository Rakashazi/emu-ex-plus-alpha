/** \file   c128mem.c
 * \brief   Memory handling for the C128 emulator
 *
 * \author  Andreas Boose <viceteam@t-online.de>
 * \author  Ettore Perazzoli <ettore@comm2000.it>
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Based on the original work in VICE 0.11.0 by
 *  Jouko Valta <jopi@stekt.oulu.fi>
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

#include "c128-resources.h"
#include "c128.h"
#include "c128mem.h"
#include "c128meminit.h"
#include "c128memlimit.h"
#include "c128memrom.h"
#include "c128mmu.h"
#include "c64cart.h"
#include "c64cia.h"
#include "c64meminit.h"
#include "c64memrom.h"
#include "c64pla.h"
#include "cartio.h"
#include "cartridge.h"
#include "cia.h"
#include "functionrom.h"
#include "georam.h"
#include "keyboard.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "monitor.h"
#include "ram.h"
#include "reu.h"
#include "sid.h"
#include "sid-resources.h"
#include "types.h"
#include "vdc-mem.h"
#include "vdc.h"
#include "vdctypes.h"
#include "vicii-mem.h"
#include "vicii-phi1.h"
#include "vicii.h"
#include "viciitypes.h"
#include "z80mem.h"
#include "video.h"

/* #define DEBUG_MMU */

#ifdef DEBUG_MMU
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x)
#endif

/* ------------------------------------------------------------------------- */

/* Number of possible video banks (16K each).  */
#define NUM_VBANKS 4

/* The C128 memory.  */
uint8_t mem_ram[C128_RAM_SIZE];
uint8_t mem_chargen_rom[C128_CHARGEN_ROM_SIZE];

/* Internal color memory.  */
static uint8_t mem_color_ram[0x800];
uint8_t *mem_color_ram_cpu, *mem_color_ram_vicii;

/* Pointer to the chargen ROM.  */
uint8_t *mem_chargen_rom_ptr;

/* Currently selected RAM bank.  */
uint8_t *ram_bank;

/* Shared memory.  */
static uint16_t top_shared_limit, bottom_shared_limit;

/* Pointers to pages 0 and 1 (which can be physically placed anywhere).  */
uint8_t *mem_page_zero, *mem_page_one;

/* Pointers to the currently used memory read and write tables.  */
read_func_ptr_t *_mem_read_tab_ptr;
store_func_ptr_t *_mem_write_tab_ptr;
read_func_ptr_t *_mem_read_tab_ptr_dummy;
store_func_ptr_t *_mem_write_tab_ptr_dummy;
static uint8_t **_mem_read_base_tab_ptr;
static int *mem_read_limit_tab_ptr;

#define NUM_CONFIGS 256

/* Memory read and write tables.  */
static store_func_ptr_t mem_write_tab[NUM_VBANKS][NUM_CONFIGS][0x101];
static read_func_ptr_t mem_read_tab[NUM_CONFIGS][0x101];
static uint8_t *mem_read_base_tab[NUM_CONFIGS][0x101];
static int mem_read_limit_tab[NUM_CONFIGS][0x101];

static store_func_ptr_t mem_write_tab_watch[0x101];
static read_func_ptr_t mem_read_tab_watch[0x101];

/* Current video bank (0, 1, 2 or 3 in the first bank,
   4, 5, 6 or 7 in the second bank).  */
static int vbank = 0;

/* Tape sense status: 1 = some button pressed, 0 = no buttons pressed.  */
static int tape_sense = 0;

static int tape_write_in = 0;
static int tape_motor_in = 0;

/* Current memory configuration.  */
static int mem_config;

/* Current watchpoint state. 
          0 = no watchpoints
    bit0; 1 = watchpoints active
    bit1; 2 = watchpoints trigger on dummy accesses
*/
static int watchpoints_active = 0;

/* Current machine type.  */
static unsigned int mem_machine_type;

/* Logging goes here.  */
static log_t c128_mem_log = LOG_DEFAULT;

/* Status of the CAPS key (ASCII/DIN).  */
static int caps_sense = 1;

/* ------------------------------------------------------------------------- */

static uint8_t watch_read(uint16_t addr)
{
    monitor_watch_push_load_addr(addr, e_comp_space);
    return mem_read_tab[mem_config][addr >> 8](addr);
}

static void watch_store(uint16_t addr, uint8_t value)
{
    monitor_watch_push_store_addr(addr, e_comp_space);
    mem_write_tab[vbank][mem_config][addr >> 8](addr, value);
}

/* called by mem_update_config(), mem_toggle_watchpoints() */
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

static void mem_update_chargen(unsigned int chargen_high)
{
    uint8_t *old_chargen_rom_ptr;

    old_chargen_rom_ptr = mem_chargen_rom_ptr;

    /* invert line on international version, fixes bug #781 */
    if (mem_machine_type == C128_MACHINE_INT) {
        chargen_high = chargen_high ? 0 : 1;
    }

    if (chargen_high) {
        mem_chargen_rom_ptr = mem_chargen_rom;
    } else {
        mem_chargen_rom_ptr = &mem_chargen_rom[0x1000];
    }
    if (old_chargen_rom_ptr != mem_chargen_rom_ptr) {
        machine_update_memory_ptrs();
    }
}

void mem_update_config(int config)
{
    mem_config = config;

    mem_update_tab_ptrs(watchpoints_active);

    _mem_read_base_tab_ptr = mem_read_base_tab[config];
    mem_read_limit_tab_ptr = mem_read_limit_tab[config];

    maincpu_resync_limits();

    if (config >= 0x80) {
        if (mem_machine_type == C128_MACHINE_INT) {
            mem_update_chargen(0);
        }
        mem_color_ram_cpu = mem_color_ram;
        mem_color_ram_vicii = mem_color_ram;
        vicii_set_chargen_addr_options(0x7000, 0x1000);
    } else {
        if (mem_machine_type == C128_MACHINE_INT) {
            mem_update_chargen(1);
        }
        if (pport.data_read & 1) {
            mem_color_ram_cpu = mem_color_ram;
        } else {
            mem_color_ram_cpu = &mem_color_ram[0x400];
        }
        if (pport.data_read & 2) {
            mem_color_ram_vicii = mem_color_ram;
        } else {
            mem_color_ram_vicii = &mem_color_ram[0x400];
        }
        if (pport.data_read & 4) {
            vicii_set_chargen_addr_options(0xffff, 0xffff);
        } else {
            vicii_set_chargen_addr_options(0x3000, 0x1000);
        }
    }
}

void mem_set_machine_type(unsigned type)
{
    mem_machine_type = type;
    caps_sense = 1;
    mem_pla_config_changed();
}

/* Change the current video bank.  Call this routine only when the vbank
   has really changed.  */
void mem_set_vbank(int new_vbank)
{
    vbank = (vbank & ~3) | new_vbank;
    vicii_set_vbank(new_vbank & 3);
}

void mem_set_ram_config(uint8_t value)
{
    unsigned int shared_size;

    /* XXX: We only support 128K here.  */
    vicii_set_ram_base(mem_ram + ((value & 0x40) << 10));

    DEBUG_PRINT(("MMU: Store RCR = $%02x\n", value));
    DEBUG_PRINT(("MMU: VIC-II base at $%05X\n", ((value & 0xc0) << 2)));

    if ((value & 0x3) == 0) {
        shared_size = 1024;
    } else {
        shared_size = 0x1000 << ((value & 0x3) - 1);
    }

    /* Share high memory?  */
    if (value & 0x8) {
        top_shared_limit = 0xffff - shared_size;
        DEBUG_PRINT(("MMU: Sharing high RAM from $%04X\n", top_shared_limit + 1));
    } else {
        top_shared_limit = 0xffff;
        DEBUG_PRINT(("MMU: No high shared RAM\n"));
    }

    /* Share low memory?  */
    if (value & 0x4) {
        bottom_shared_limit = shared_size;
        DEBUG_PRINT(("MMU: Sharing low RAM up to $%04X\n", bottom_shared_limit - 1));
    } else {
        bottom_shared_limit = 0;
        DEBUG_PRINT(("MMU: No low shared RAM\n"));
    }
}

/* ------------------------------------------------------------------------- */

void mem_pla_config_changed(void)
{
    /* NOTE: on the international/US version of the hardware, there is no DIN/ASCII key -
             instead this key is caps lock */
    c64pla_config_changed(tape_sense, tape_write_in, tape_motor_in, caps_sense, 0x57);

    mmu_set_config64(((~pport.dir | pport.data) & 0x7) | (export.exrom << 3) | (export.game << 4));

    /* on the international version, A8 of the chargen comes from the c64/c128 mode, not
       from the status of the DIN/ASCII (capslock) key */
    if (mem_machine_type != C128_MACHINE_INT) {
        mem_update_chargen(pport.data_read & 0x40);
    }
}

static void mem_toggle_caps_key(void)
{
    caps_sense = (caps_sense) ? 0 : 1;
    mem_pla_config_changed();
    log_message(c128_mem_log, "CAPS key (ASCII/DIN) %s.", (caps_sense) ? "released" : "pressed");
}

static int mem_get_caps_key(void)
{
    return caps_sense;
}

/* ------------------------------------------------------------------------- */

/* $00/$01 unused bits emulation

   - There are 2 different unused bits, 1) the output bits, 2) the input bits
   - The output bits can be (re)set when the data-direction is set to output
     for those bits and the output bits will not drop-off to 0.
   - When the data-direction for the unused bits is set to output then the
     unused input bits can be (re)set by writing to them, when set to 1 the
     drop-off timer will start which will cause the unused input bits to drop
     down to 0 in a certain amount of time.
   - When an unused input bit already had the drop-off timer running, and is
     set to 1 again, the drop-off timer will restart.
   - when an unused bit changes from output to input, and the current output
     bit is 1, the drop-off timer will restart again

    see testprogs/CPU/cpuport for details and tests
*/

uint8_t zero_read(uint16_t addr)
{
    addr &= 0xff;

    switch ((uint8_t)addr) {
        case 0:
            vicii.last_cpu_val = pport.dir_read;
            break;
        case 1:
            vicii.last_cpu_val = pport.data_read;

            /* discharge the "capacitor" */

            /* set real value of read bit 7 */
            if (pport.data_falloff_bit7 && (pport.data_set_clk_bit7 < maincpu_clk)) {
                pport.data_falloff_bit7 = 0;
                pport.data_set_bit7 = 0;
            }

            /* for unused bits in input mode, the value comes from the "capacitor" */

            /* set real value of bit 7 */
            if (!(pport.dir_read & 0x80)) {
               vicii.last_cpu_val &= ~0x80;
               vicii.last_cpu_val |= pport.data_set_bit7;
            }
            break;
        default:
            vicii.last_cpu_val = mem_page_zero[addr];
    }

    return vicii.last_cpu_val;
}

void zero_store(uint16_t addr, uint8_t value)
{
    addr &= 0xff;

    vicii.last_cpu_val = value;

    switch ((uint8_t)addr) {
        case 0:
#if 0
            if (vbank == 0) {
                vicii_mem_vbank_store((uint16_t)0, vicii_read_phi1_lowlevel());
            } else {
#endif
            mem_page_zero[0] = vicii_read_phi1_lowlevel();
            machine_handle_pending_alarms(maincpu_rmw_flag + 1);
#if 0
    }
#endif
            /* when switching an unused bit from output (where it contained a
               stable value) to input mode (where the input is floating), some
               of the charge is transferred to the floating input */

            /* check if bit 7 has flipped */
            if ((pport.dir & 0x80)) {
                if ((pport.dir ^ value) & 0x80) {
                    pport.data_set_clk_bit7 = maincpu_clk + C128_CPU8502_DATA_PORT_FALL_OFF_CYCLES;
                    pport.data_set_bit7 = pport.data & 0x80;
                    pport.data_falloff_bit7 = 1;
                }
            }

            if (pport.dir != value) {
                pport.dir = value;
                mem_pla_config_changed();
            }
            break;
        case 1:
#if 0
            if (vbank == 0) {
                vicii_mem_vbank_store((uint16_t)1, vicii_read_phi1_lowlevel());
            } else {
#endif
            mem_page_zero[1] = vicii_read_phi1_lowlevel();
            machine_handle_pending_alarms(maincpu_rmw_flag + 1);
#if 0
    }
#endif
            /* when writing to an unused bit that is output, charge the "capacitor",
               otherwise don't touch it */
            if (pport.dir & 0x80) {
                pport.data_set_bit7 = value & 0x80;
                pport.data_set_clk_bit7 = maincpu_clk + C128_CPU8502_DATA_PORT_FALL_OFF_CYCLES;
                pport.data_falloff_bit7 = 1;
            }

            if (pport.data != value) {
                pport.data = value;
                mem_pla_config_changed();
            }
            break;
        default:
#if 0
            if (vbank == 0) {
                vicii_mem_vbank_store(addr, value);
            } else {
#endif
            mem_page_zero[addr] = value;
#if 0
    }
#endif
    }
}

/* ------------------------------------------------------------------------- */

uint8_t one_read(uint16_t addr)
{
    return mem_page_one[addr - 0x100];
}

void one_store(uint16_t addr, uint8_t value)
{
    mem_page_one[addr - 0x100] = value;
}

/* ------------------------------------------------------------------------- */

/* External memory access functions.  */

uint8_t chargen_read(uint16_t addr)
{
    vicii.last_cpu_val = mem_chargen_rom_ptr[addr & 0x0fff];
    return vicii.last_cpu_val;
}

void chargen_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    mem_chargen_rom_ptr[addr & 0x0fff] = value;
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

void mem_store_without_ultimax(uint16_t addr, uint8_t value)
{
    store_func_ptr_t *write_tab_ptr;

    write_tab_ptr = mem_write_tab[vbank][mem_config & 7];

    write_tab_ptr[addr >> 8](addr, value);
}

uint8_t mem_read_without_ultimax(uint16_t addr)
{
    read_func_ptr_t *read_tab_ptr;

    read_tab_ptr = mem_read_tab[mem_config & 7];

    return read_tab_ptr[addr >> 8](addr);
}

void mem_store_without_romlh(uint16_t addr, uint8_t value)
{
    store_func_ptr_t *write_tab_ptr;

    write_tab_ptr = mem_write_tab[vbank][0];

    write_tab_ptr[addr >> 8](addr, value);
}

/* ------------------------------------------------------------------------- */

/* CPU Memory interface.  */

/* The MMU can basically do the following:

   - select one of the two (four) memory banks as the standard
   (non-shared) memory;

   - turn ROM and I/O on and off;

   - enable/disable top/bottom shared RAM (from 1K to 16K, so bottom
   shared RAM cannot go beyond $3FFF and top shared RAM cannot go
   under $C000);

   - move pages 0 and 1 to any physical address.  */

#define READ_TOP_SHARED(addr) ((addr) > top_shared_limit ? mem_ram[(addr)] : ram_bank[(addr)])

#define STORE_TOP_SHARED(addr, value) ((addr) > top_shared_limit ? (mem_ram[(addr)] = (value)) : (ram_bank[(addr)] = (value)))

#define READ_BOTTOM_SHARED(addr) ((addr) < bottom_shared_limit ? mem_ram[(addr)] : ram_bank[(addr)])

#define STORE_BOTTOM_SHARED(addr, value) ((addr) < bottom_shared_limit ? (mem_ram[(addr)] = (value)) : (ram_bank[(addr)] = (value)))

/* $0200 - $3FFF: RAM (normal or shared).  */
uint8_t lo_read(uint16_t addr)
{
    vicii.last_cpu_val = READ_BOTTOM_SHARED(addr);

    return vicii.last_cpu_val;
}

void lo_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    STORE_BOTTOM_SHARED(addr, value);
}

uint8_t ram_read(uint16_t addr)
{
    vicii.last_cpu_val = ram_bank[addr];

    return vicii.last_cpu_val;
}

void ram_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;

    ram_bank[addr] = value;
}

void ram_hi_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;

    if (vbank == 3) {
        vicii_mem_vbank_3fxx_store(addr, value);
    } else {
        ram_bank[addr] = value;
    }

    if (addr == 0xff00) {
        reu_dma(-1);
    }
}

/* $4000 - $7FFF: RAM or low BASIC ROM.  */
uint8_t basic_lo_read(uint16_t addr)
{
    vicii.last_cpu_val = c128memrom_basic_rom[addr - 0x4000];

    return vicii.last_cpu_val;
}

void basic_lo_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;

    ram_bank[addr] = value;
}

/* $8000 - $BFFF: RAM or high BASIC ROM.  */
uint8_t basic_hi_read(uint16_t addr)
{
    vicii.last_cpu_val = c128memrom_basic_rom[addr - 0x4000];
    return vicii.last_cpu_val;
}

void basic_hi_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    ram_bank[addr] = value;
}

/* $C000 - $CFFF: RAM (normal or shared) or Editor ROM.  */
uint8_t editor_read(uint16_t addr)
{
    vicii.last_cpu_val = c128memrom_basic_rom[addr - 0x4000];
    return vicii.last_cpu_val;
}

void editor_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    STORE_TOP_SHARED(addr, value);
}

static uint8_t d5xx_read(uint16_t addr)
{
    return vicii_read_phi1();
}

static void d5xx_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
}

uint8_t d7xx_read(uint16_t addr)
{
    if (sid_stereo >= 1 && addr >= sid2_address_start && addr < sid2_address_end) {
        return sid2_read(addr);
    }
    if (sid_stereo >= 2 && addr >= sid3_address_start && addr < sid3_address_end) {
        return sid3_read(addr);
    }
    if (sid_stereo >= 3 && addr >= sid4_address_start && addr < sid4_address_end) {
        return sid4_read(addr);
    }
    if (sid_stereo >= 4 && addr >= sid5_address_start && addr < sid5_address_end) {
        return sid5_read(addr);
    }
    if (sid_stereo >= 5 && addr >= sid6_address_start && addr < sid6_address_end) {
        return sid6_read(addr);
    }
    if (sid_stereo >= 6 && addr >= sid7_address_start && addr < sid7_address_end) {
        return sid7_read(addr);
    }
    if (sid_stereo >= 7 && addr >= sid8_address_start && addr < sid8_address_end) {
        return sid8_read(addr);
    }
    return vicii_read_phi1();
}

void d7xx_store(uint16_t addr, uint8_t value)
{
    if (sid_stereo >= 1 && addr >= sid2_address_start && addr < sid2_address_end) {
        sid2_store(addr, value);
    }
    if (sid_stereo >= 2 && addr >= sid3_address_start && addr < sid3_address_end) {
        sid3_store(addr, value);
    }
    if (sid_stereo >= 3 && addr >= sid4_address_start && addr < sid4_address_end) {
        sid4_store(addr, value);
    }
    if (sid_stereo >= 4 && addr >= sid5_address_start && addr < sid5_address_end) {
        sid5_store(addr, value);
    }
    if (sid_stereo >= 5 && addr >= sid6_address_start && addr < sid6_address_end) {
        sid6_store(addr, value);
    }
    if (sid_stereo >= 6 && addr >= sid7_address_start && addr < sid7_address_end) {
        sid7_store(addr, value);
    }
    if (sid_stereo >= 7 && addr >= sid8_address_start && addr < sid8_address_end) {
        sid8_store(addr, value);
    }
    vicii.last_cpu_val = value;
}

/* $E000 - $FFFF: RAM or Kernal.  */
uint8_t hi_read(uint16_t addr)
{
    vicii.last_cpu_val = c128memrom_kernal_rom[addr & 0x1fff];
    return vicii.last_cpu_val;
}

void hi_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    STORE_TOP_SHARED(addr, value);
}

uint8_t top_shared_read(uint16_t addr)
{
    vicii.last_cpu_val = READ_TOP_SHARED(addr);
    return vicii.last_cpu_val;
}

void top_shared_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    STORE_TOP_SHARED(addr, value);
}

/* ------------------------------------------------------------------------- */

void colorram_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    mem_color_ram_cpu[addr & 0x3ff] = value & 0xf;
}

uint8_t colorram_read(uint16_t addr)
{
    vicii.last_cpu_val = mem_color_ram_cpu[addr & 0x3ff] | (vicii_read_phi1() & 0xf0);
    return vicii.last_cpu_val;
}

/* ------------------------------------------------------------------------- */

void mem_set_write_hook(int config, int page, store_func_t *f)
{
    int i;

    for (i = 0; i < NUM_VBANKS; i++) {
        mem_write_tab[i][config][page] = f;
    }
}

void mem_read_tab_set(unsigned int base, unsigned int index, read_func_ptr_t read_func)
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

    mem_chargen_rom_ptr = mem_chargen_rom;
    mem_color_ram_cpu = mem_color_ram;
    mem_color_ram_vicii = mem_color_ram;

    mem_limit_init(mem_read_limit_tab);

    for (i = 0; i <= 0x100; i++) {
        mem_read_tab_watch[i] = watch_read;
        mem_write_tab_watch[i] = watch_store;
    }

    c128meminit();

    /* C64 mode configuration.  */

    for (i = 0; i < 32; i++) {
        mem_set_write_hook(128 + i, 0, zero_store);
        mem_read_tab[128 + i][0] = zero_read;
        mem_read_base_tab[128 + i][0] = mem_ram;
        mem_set_write_hook(128 + i, 1, one_store);
        mem_read_tab[128 + i][1] = one_read;
        mem_read_base_tab[128 + i][1] = mem_ram;
        for (j = 2; j <= 0xfe; j++) {
            mem_read_tab[128 + i][j] = ram_read;
            mem_read_base_tab[128 + i][j] = mem_ram;
            for (k = 0; k < NUM_VBANKS; k++) {
                if ((j & 0xc0) == (k << 6)) {
                    switch (j & 0x3f) {
                        case 0x39:
                            mem_write_tab[k][128 + i][j] = vicii_mem_vbank_39xx_store;
                            break;
                        case 0x3f:
                            mem_write_tab[k][128 + i][j] = vicii_mem_vbank_3fxx_store;
                            break;
                        default:
                            mem_write_tab[k][128 + i][j] = vicii_mem_vbank_store;
                    }
                } else {
                    mem_write_tab[k][128 + i][j] = ram_store;
                }
            }
        }
        mem_read_tab[128 + i][0xff] = ram_read;
        mem_read_base_tab[128 + i][0xff] = mem_ram;

        /* vbank access is handled within `ram_hi_store()'.  */
        mem_set_write_hook(128 + i, 0xff, ram_hi_store);
    }

    /* Setup character generator ROM at $D000-$DFFF (memory configs 1, 2,
       3, 9, 10, 11, 25, 26, 27).  */
    for (i = 0xd0; i <= 0xdf; i++) {
        mem_read_tab[128 + 1][i] = chargen_read;
        mem_read_tab[128 + 2][i] = chargen_read;
        mem_read_tab[128 + 3][i] = chargen_read;
        mem_read_tab[128 + 9][i] = chargen_read;
        mem_read_tab[128 + 10][i] = chargen_read;
        mem_read_tab[128 + 11][i] = chargen_read;
        /*mem_read_tab[128 + 25][i] = chargen_read;*/
        mem_read_tab[128 + 26][i] = chargen_read;
        mem_read_tab[128 + 27][i] = chargen_read;
        mem_read_base_tab[128 + 1][i] = NULL;
        mem_read_base_tab[128 + 2][i] = NULL;
        mem_read_base_tab[128 + 3][i] = NULL;
        mem_read_base_tab[128 + 9][i] = NULL;
        mem_read_base_tab[128 + 10][i] = NULL;
        mem_read_base_tab[128 + 11][i] = NULL;
        /*mem_read_base_tab[128 + 25][i] = NULL;*/
        mem_read_base_tab[128 + 26][i] = NULL;
        mem_read_base_tab[128 + 27][i] = NULL;
    }

    c64meminit(128);

    /* Setup C128 specific I/O at $D000-$DFFF.  */
    for (j = 0; j < 32; j++) {
        if (c64meminit_io_config[j]) {
            mem_read_tab[128 + j][0xd0] = c128_c64io_d000_read;
            mem_set_write_hook(128 + j, 0xd0, c128_c64io_d000_store);
            mem_read_tab[128 + j][0xd1] = c128_c64io_d100_read;
            mem_set_write_hook(128 + j, 0xd1, c128_c64io_d100_store);
            mem_read_tab[128 + j][0xd2] = c128_c64io_d200_read;
            mem_set_write_hook(128 + j, 0xd2, c128_c64io_d200_store);
            mem_read_tab[128 + j][0xd3] = c128_c64io_d300_read;
            mem_set_write_hook(128 + j, 0xd3, c128_c64io_d300_store);
            mem_read_tab[128 + j][0xd4] = c128_c64io_d400_read;
            mem_set_write_hook(128 + j, 0xd4, c128_c64io_d400_store);
            mem_read_tab[128 + j][0xd5] = c128_d5xx_read;
            mem_set_write_hook(128 + j, 0xd5, c128_d5xx_store);
            mem_read_tab[128 + j][0xd6] = c128_vdc_read;
            mem_set_write_hook(128 + j, 0xd6, c128_vdc_store);
            mem_read_tab[128 + j][0xd7] = c128_c64io_d700_read;
            mem_set_write_hook(128 + j, 0xd7, c128_c64io_d700_store);
            mem_read_tab[128 + j][0xd8] = c128_colorram_read;
            mem_set_write_hook(128 + j, 0xd8, c128_colorram_store);
            mem_read_tab[128 + j][0xd9] = c128_colorram_read;
            mem_set_write_hook(128 + j, 0xd9, c128_colorram_store);
            mem_read_tab[128 + j][0xda] = c128_colorram_read;
            mem_set_write_hook(128 + j, 0xda, c128_colorram_store);
            mem_read_tab[128 + j][0xdb] = c128_colorram_read;
            mem_set_write_hook(128 + j, 0xdb, c128_colorram_store);
            mem_read_tab[128 + j][0xdc] = c128_cia1_read;
            mem_set_write_hook(128 + j, 0xdc, c128_cia1_store);
            mem_read_tab[128 + j][0xdd] = c128_cia2_read;
            mem_set_write_hook(128 + j, 0xdd, c128_cia2_store);
            mem_read_tab[128 + j][0xde] = c128_c64io_de00_read;
            mem_set_write_hook(128 + j, 0xde, c128_c64io_de00_store);
            mem_read_tab[128 + j][0xdf] = c128_c64io_df00_read;
            mem_set_write_hook(128 + j, 0xdf, c128_c64io_df00_store);
        }
    }

    for (i = 128; i < 128 + 32; i++) {
        mem_read_tab[i][0x100] = mem_read_tab[i][0];
        for (j = 0; j < NUM_VBANKS; j++) {
            mem_write_tab[j][i][0x100] = mem_write_tab[j][i][0];
        }
        mem_read_base_tab[i][0x100] = mem_read_base_tab[i][0];
    }

    vicii_set_chargen_addr_options(0xffff, 0xffff);

    mmu_reset();

    keyboard_register_caps_key(mem_toggle_caps_key);
    keyboard_register_get_caps_key(mem_get_caps_key);

    top_shared_limit = 0xffff;
    bottom_shared_limit = 0x0000;
    ram_bank = mem_ram;
    mem_page_zero = mem_ram;
    mem_page_one = mem_ram + 0x100;

    _mem_read_tab_ptr = mem_read_tab[3];
    _mem_write_tab_ptr = mem_write_tab[vbank][3];
    _mem_read_base_tab_ptr = mem_read_base_tab[3];
    mem_read_limit_tab_ptr = mem_read_limit_tab[3];

    c64pla_pport_reset();

    cartridge_init_config();
}


void mem_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
    uint8_t *p = _mem_read_base_tab_ptr[addr >> 8];

    *base = (p == NULL) ? NULL : p;
    *start = addr; /* TODO */
    *limit = mem_read_limit_tab_ptr[addr >> 8];
}

/* ------------------------------------------------------------------------- */

/* Initialize RAM for power-up.  */
void mem_powerup(void)
{
    ram_init(mem_ram, C128_RAM_SIZE);
}

/* ------------------------------------------------------------------------- */

/* Set the tape sense status.  */
void mem_set_tape_sense(int sense)
{
    tape_sense = sense;
    mem_pla_config_changed();
}

/* Set the tape write in. */
void mem_set_tape_write_in(int val)
{
    tape_write_in = val;
    mem_pla_config_changed();
}

/* Set the tape motor in. */
void mem_set_tape_motor_in(int val)
{
    tape_motor_in = val;
    mem_pla_config_changed();
}

/* ------------------------------------------------------------------------- */

void mem_get_basic_text(uint16_t *start, uint16_t *end)
{
    if (start != NULL) {
        *start = mem_ram[0x2b] | (mem_ram[0x2c] << 8);
    }
    if (mmu_is_c64config()) {
        if (end != NULL) {
            *end = mem_ram[0x2d] | (mem_ram[0x2e] << 8);
        }
    } else {
        if (end != NULL) {
            *end = mem_ram[0x1210] | (mem_ram[0x1211] << 8);
        }
    }
}

void mem_set_basic_text(uint16_t start, uint16_t end)
{
    mem_ram[0x2b] = mem_ram[0xac] = start & 0xff;
    mem_ram[0x2c] = mem_ram[0xad] = start >> 8;
    if (mmu_is_c64config()) {
        mem_ram[0x2d] = mem_ram[0x2f] = mem_ram[0x31] = mem_ram[0xae] = end & 0xff;
        mem_ram[0x2e] = mem_ram[0x30] = mem_ram[0x32] = mem_ram[0xaf] = end >> 8;
    } else {
        mem_ram[0x1210] = end & 0xff;
        mem_ram[0x1211] = end >> 8;
    }
}

/* this function should always read from the screen currently used by the kernal
   for output, normally this does just return system ram - except when the 
   videoram is not memory mapped.
   used by autostart to "read" the kernal messages
*/
uint8_t mem_read_screen(uint16_t addr)
{
    /* we assume in C64 mode the kernal never uses the VDC :) */
    if (mmu_is_c64config()) {
        /* directly read the memory without going through the mmu - it may
           point to the upper 64k block and then we read the wrong memory */
        /* return ram_read(addr); */
        return mem_ram[addr];
    }
    if (!(mem_ram[215] & 0x80)) {
        /* directly read the memory without going through the mmu - it may
           point to the upper 64k block and then we read the wrong memory */
        /* return ram_read(addr); */
        return mem_ram[addr];
    }
    return vdc_ram_read(addr);
}

void mem_inject(uint32_t addr, uint8_t value)
{
    /* this could be altered to handle more that 64 Kb in some
       useful way */
    mem_ram[addr & 0xffff] = value;
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
        if (mem_config >= 128) {
            switch (mem_config - 128) {
                case 2:
                case 3:
                case 6:
                case 7:
                case 10:
                case 11:
                case 14:
                case 15:
                case 26:
                case 27:
                case 30:
                case 31:
                    return 1;
                default:
                    return 0;
            }
        }
        return 1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

/* Banked memory access functions for the monitor */

/* FIXME: peek, cartridge support */

void store_bank_io(uint16_t addr, uint8_t byte)
{
    switch (addr & 0xff00) {
        case 0xd000:
            c64io_d000_store(addr, byte);
            break;
        case 0xd100:
            c64io_d100_store(addr, byte);
            break;
        case 0xd200:
            c64io_d200_store(addr, byte);
            break;
        case 0xd300:
            c64io_d300_store(addr, byte);
            break;
        case 0xd400:
            c64io_d400_store(addr, byte);
            break;
        case 0xd500:
            mmu_store(addr, byte);
            break;
        case 0xd600:
            vdc_store(addr, byte);
            break;
        case 0xd700:
            c64io_d700_store(addr, byte);
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
            c64io_de00_store(addr, byte);
            break;
        case 0xdf00:
            c64io_df00_store(addr, byte);
            break;
    }
    return;
}

uint8_t read_bank_io(uint16_t addr)
{
    switch (addr & 0xff00) {
        case 0xd000:
            return c64io_d000_read(addr);
        case 0xd100:
            return c64io_d100_read(addr);
        case 0xd200:
            return c64io_d200_read(addr);
        case 0xd300:
            return c64io_d300_read(addr);
        case 0xd400:
            return c64io_d400_read(addr);
        case 0xd500:
            return mmu_read(addr);
        case 0xd600:
            return vdc_read(addr);
        case 0xd700:
            return c64io_d700_read(addr);
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
            return c64io_de00_read(addr);
        case 0xdf00:
            return c64io_df00_read(addr);
    }
    return 0xff;
}

static uint8_t peek_bank_io(uint16_t addr)
{
    switch (addr & 0xff00) {
        case 0xd000:
            return c64io_d000_peek(addr);
        case 0xd100:
            return c64io_d100_peek(addr);
        case 0xd200:
            return c64io_d200_peek(addr);
        case 0xd300:
            return c64io_d300_peek(addr);
        case 0xd400:
            return c64io_d400_peek(addr);
        case 0xd500:
            return mmu_peek(addr);
        case 0xd600:
            return vdc_peek(addr);
        case 0xd700:
            return c64io_d700_peek(addr);
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
            return c64io_de00_peek(addr);
        case 0xdf00:
            return c64io_df00_peek(addr);
    }
    return 0xff;
}

/* Exported banked memory access functions for the monitor.  */
#define MAXBANKS (5 + 2 + 5)

/* FIXME: add ram00 bank, make 'ram' bank always show selected ram bank, ram00
 * and ram01 always physical ram bank */

static const char *banknames[MAXBANKS + 1] = {
    "default",
    "cpu",
    "ram",
    "rom",
    "io",
    /* by convention, a "bank array" has a 2-hex-digit bank index appended */
    "ram00",
    "ram01",
    "intfunc",
    "extfunc",
    "cart",
    "c64rom",
    "vdc",
    NULL
};

enum {
    bank_cpu = 0,
    bank_ram,
    bank_rom,
    bank_io,
    bank_ram00,
    bank_ram01,
    bank_intfunc,
    bank_extfunc,
    bank_cart,
    bank_c64rom,
    bank_vdc
};

static const int banknums[MAXBANKS + 1] = {
    bank_cpu, /* default */
    bank_cpu,
    bank_ram,
    bank_rom,
    bank_io,
    bank_ram00,
    bank_ram01,
    bank_intfunc,
    bank_extfunc,
    bank_cart,
    bank_c64rom,
    bank_vdc,
    -1
};

static const int bankindex[MAXBANKS + 1] = {
    -1,
    -1,
    -1,
    -1,
    -1,
    0,
    1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1
};

static const int bankflags[MAXBANKS + 1] = {
    0,
    0,
    0,
    0,
    0,
    MEM_BANK_ISARRAY | MEM_BANK_ISARRAYFIRST,
    MEM_BANK_ISARRAY | MEM_BANK_ISARRAYLAST,
    0,
    0,
    0,
    0,
    0,
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
        case bank_cpu:                   /* current */
            return mem_read(addr);
        case bank_ram00:                   /* ram0 */
            return mem_ram[addr];
        case bank_ram01:                   /* ram1 */
            return mem_ram[addr + 0x10000];
        case bank_io:                   /* io */
            if (addr >= 0xd000 && addr < 0xe000) {
                return read_bank_io(addr);
            }
            /* FALL THROUGH */
        case bank_rom:                   /* rom */
            if (addr <= 0x0fff) {
                return bios_read(addr);
            }
            if (addr >= 0x4000 && addr <= 0xcfff) {
                return c128memrom_basic_rom[addr - 0x4000];
            }
            if (addr >= 0xd000 && addr <= 0xdfff) {
                return mem_chargen_rom[addr & 0x0fff];
            }
            if (addr >= 0xe000) {
                return c128memrom_kernal_rom[addr & 0x1fff];
            }
            /* FALL THROUGH */
        case bank_ram:                   /* ram */
            break;
        case bank_intfunc:
            if (addr >= 0x8000) {
                return int_function_rom[addr & 0x7fff];
            }
            break;
        case bank_extfunc:
            if (addr >= 0x8000) {
                return ext_function_rom[addr & 0x7fff];
            }
            break;
        case bank_cart:
            return cartridge_peek_mem(addr);
        case bank_c64rom:
            if (addr >= 0xa000 && addr <= 0xbfff) {
                return c64memrom_basic64_rom[addr & 0x1fff];
            }
            if (addr >= 0xd000 && addr <= 0xdfff) {
                return mem_chargen_rom[addr & 0x0fff];
            }
            if (addr >= 0xe000) {
                return c64memrom_kernal64_rom[addr & 0x1fff];
            }
            break;
        case bank_vdc:
            return vdc_ram_read(addr);
    }
    return mem_ram[addr];
}

/* used by monitor if sfx off */
uint8_t mem_bank_peek(int bank, uint16_t addr, void *context)
{
    switch (bank) {
        case bank_cpu:                   /* current */
            /* FIXME: we must check for which bank is currently active, and only use peek_bank_io
                      when needed. doing this without checking is wrong, but we do it anyways to
                      avoid side effects
           */
            if (addr >= 0xd000 && addr < 0xe000) {
                return peek_bank_io(addr);
            }
            return mem_read(addr);
            break;
        case bank_io:                   /* io */
            if (addr >= 0xd000 && addr < 0xe000) {
                return peek_bank_io(addr);
            }
            break;
        case bank_cart:
            return cartridge_peek_mem(addr);
    }
    return mem_bank_read(bank, addr, context);
}

void mem_bank_write(int bank, uint16_t addr, uint8_t byte, void *context)
{
    switch (bank) {
        case bank_cpu:                   /* current */
            mem_store(addr, byte);
            return;
        case bank_ram00:                   /* ram0 */
            mem_ram[addr] = byte;
            return;
        case bank_ram01:                   /* ram1 */
            mem_ram[addr + 0x10000] = byte;
            return;
        case bank_io:                   /* io */
            if (addr >= 0xd000 && addr < 0xe000) {
                store_bank_io(addr, byte);
                return;
            }
            /* FALL THROUGH */
        case bank_rom:                   /* rom */
            if (addr >= 0x4000 && addr <= 0xcfff) {
                return;
            }
            if (addr >= 0xe000) {
                return;
            }
            /* FALL THROUGH */
        case bank_ram:                   /* ram */
            break;
        case bank_intfunc:
            if (addr >= 0x8000) {
                return;
            }
            break;
        case bank_extfunc:
            if (addr >= 0x8000 && addr <= 0xbfff) {
                return;
            }
            break;
        case bank_cart:
            if (addr >= 0x8000 && addr <= 0x9fff) {
                return;
            }
            if (addr >= 0xa000 && addr <= 0xbfff) {
                return;
            }
            /* FALL THROUGH */
        case bank_c64rom:
            if (addr >= 0xa000 && addr <= 0xbfff) {
                return;
            }
            if (addr >= 0xd000 && addr <= 0xdfff) {
                return;
            }
            if (addr >= 0xe000) {
                return;
            }
            break;
        case bank_vdc:
            vdc_ram_store(addr, byte);
            break;
    }
    mem_ram[addr] = byte;
}

/* used by monitor if sfx off */
void mem_bank_poke(int bank, uint16_t addr, uint8_t byte, void *context)
{
    mem_bank_write(bank, addr, byte, context);
}

static int mem_dump_io(void *context, uint16_t addr)
{
    if ((addr >= 0xdc00) && (addr <= 0xdc3f)) {
        return ciacore_dump(machine_context.cia1);
    } else if ((addr >= 0xdd00) && (addr <= 0xdd3f)) {
        return ciacore_dump(machine_context.cia2);
    }
    return -1;
}

mem_ioreg_list_t *mem_ioreg_list_get(void *context)
{
    mem_ioreg_list_t *mem_ioreg_list = NULL;

    io_source_ioreg_add_list(&mem_ioreg_list);  /* VIC-IIe, SID first so it's in address order */

    mon_ioreg_add_list(&mem_ioreg_list, "MMU", 0xd500, 0xd50b, mmu_dump, NULL, IO_MIRROR_NONE);
    mon_ioreg_add_list(&mem_ioreg_list, "VDC", 0xd600, 0xd601, vdc_dump, NULL, IO_MIRROR_NONE);
    mon_ioreg_add_list(&mem_ioreg_list, "CIA1", 0xdc00, 0xdc0f, mem_dump_io, NULL, IO_MIRROR_NONE);
    mon_ioreg_add_list(&mem_ioreg_list, "CIA2", 0xdd00, 0xdd0f, mem_dump_io, NULL, IO_MIRROR_NONE);

    return mem_ioreg_list;
}

void mem_get_screen_parameter(uint16_t *base, uint8_t *rows, uint8_t *columns, int *bank)
{
    int chip_idx = video_arch_get_active_chip();

    /* Check the 40/80 DISPLAY switch state */
    switch (chip_idx) {
        case VIDEO_CHIP_VDC:
            *base = (vdc.regs[12] << 8) | vdc.regs[13];
            *rows = vdc.regs[6];
            *columns = vdc.regs[1];
            *bank = bank_vdc;
            break;

        case VIDEO_CHIP_VICII:
        default:
            *base = ((vicii_peek(0xd018) & 0xf0) << 6) | ((~cia2_peek(0xdd00) & 0x03) << 14);
            *rows = 25;
            *columns = 40;
            *bank = 0;
            break;
    }

/*    printf("mem_get_screen_parameter (%s) base:%04x rows: %d colums: %d bank: %d\n",
           mem_ram[215] & 0x80 ? "vdc" : "vicii", *base, *rows, *columns, *bank); */
}

/* used by autostart to locate and "read" kernal output on the current screen
 * this function should return whatever the kernal currently uses, regardless
 * what is currently visible/active in the UI 
 */
void mem_get_cursor_parameter(uint16_t *screen_addr, uint8_t *cursor_column, uint8_t *line_length, int *blinking)
{
    if (mmu_is_c64config()) {
        /* CAUTION: this function can be called at any time when the emulation (KERNAL)
                    is in the middle of a screen update. we must make sure that all
                    values are being looked up in an "atomic" way so we dont use a low-
                    and high- byte from before and after an update, leading to invalid
                    values */
        int screen_base = (mem_ram[0xd1] + (mem_ram[0xd2] * 256)) & ~0x3ff; /* the upper bits will not change */

        /* Cursor Blink enable: 1 = Cursor in Blink Phase (visible), 0 = Cursor disabled, -1 = n/a */
        *blinking = mem_ram[0xcc] ? 0 : 1;
        /* Current Screen Line Address */
        *screen_addr = screen_base + (mem_ram[0xd6] * 40);
        /* Cursor Column on Current Line */
        *cursor_column = mem_ram[0xd3];
        while (*cursor_column >= 40) {
            *cursor_column -= 40;
            *screen_addr += 40;
        }
        /* Physical Screen Line Length */
        *line_length = 40;
    } else {
        if (!(mem_ram[215] & 0x80)) {
            /* VICII */
            int screen_base = (mem_ram[0xe0] + (mem_ram[0xe1] * 256)) & ~0x3ff; /* the upper bits will not change */
            *screen_addr = screen_base + (mem_ram[0xeb] * 40);
            *cursor_column = mem_ram[0xec];
            *line_length = 40;
            *blinking = mem_ram[0xa27] ? 0 : 1;
        } else { 
            /* VDC */
            int screen_base = ((vdc.regs[12] << 8) + vdc.regs[13]) & vdc.vdc_address_mask;
            int cursor_pos = ((vdc.regs[14] << 8) + vdc.regs[15]) & vdc.vdc_address_mask;
            *line_length = vdc.regs[1];
            *cursor_column = (cursor_pos - screen_base) % *line_length;
            *screen_addr = screen_base + (((cursor_pos - screen_base) / *line_length) * *line_length);
            *blinking = ((vdc.regs[10] & 0x60) == 0x20) ? 0 : 1;
        }
    }
/*   printf("mem_get_cursor_parameter (%s) screen_addr:%04x cursor_column: %d line_length: %d blinking: %d\n",
           mem_ram[215] & 0x80 ? "vdc" : "vicii", *screen_addr, *cursor_column, (int)*line_length, *blinking); */
}

/* ------------------------------------------------------------------------- */

void mem_color_ram_to_snapshot(uint8_t *color_ram)
{
    unsigned int i;

    for (i = 0; i < 0x400; i++) {
        color_ram[i] = (mem_color_ram[i] & 15) | (mem_color_ram[i + 0x400] << 4);
    }
}

void mem_color_ram_from_snapshot(uint8_t *color_ram)
{
    unsigned int i;

    for (i = 0; i < 0x400; i++) {
        mem_color_ram[i] = color_ram[i] & 15;
        mem_color_ram[i + 0x400] = color_ram[i] >> 4;
    }
}

/* ------------------------------------------------------------------------- */

/* 8502 specific I/O function wrappers for 2mhz mode cycle stretching */

uint8_t c128_c64io_d000_read(uint16_t addr)
{
    vicii.last_cpu_val = c64io_d000_read(addr);
    vicii_clock_read_stretch();
    return vicii.last_cpu_val;
}

void c128_c64io_d000_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    vicii_clock_write_stretch();
    c64io_d000_store(addr, value);
}

uint8_t c128_c64io_d100_read(uint16_t addr)
{
    vicii.last_cpu_val = c64io_d100_read(addr);
    vicii_clock_read_stretch();
    return vicii.last_cpu_val;
}

void c128_c64io_d100_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    vicii_clock_write_stretch();
    c64io_d100_store(addr, value);
}

uint8_t c128_c64io_d200_read(uint16_t addr)
{
    vicii.last_cpu_val = c64io_d200_read(addr);
    vicii_clock_read_stretch();
    return vicii.last_cpu_val;
}

void c128_c64io_d200_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    vicii_clock_write_stretch();
    c64io_d200_store(addr, value);
}

uint8_t c128_c64io_d300_read(uint16_t addr)
{
    vicii.last_cpu_val = c64io_d300_read(addr);
    vicii_clock_read_stretch();
    return vicii.last_cpu_val;
}

void c128_c64io_d300_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    vicii_clock_write_stretch();
    c64io_d300_store(addr, value);
}

uint8_t c128_c64io_d400_read(uint16_t addr)
{
    vicii.last_cpu_val = c64io_d400_read(addr);
    vicii_clock_read_stretch();
    return vicii.last_cpu_val;
}

void c128_c64io_d400_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    vicii_clock_write_stretch();
    c64io_d400_store(addr, value);
}

uint8_t c128_mmu_read(uint16_t addr)
{
    vicii.last_cpu_val = mmu_read(addr);
    vicii_clock_read_stretch();
    return vicii.last_cpu_val;
}

void c128_mmu_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    vicii_clock_write_stretch();
    mmu_store(addr, value);
}

uint8_t c128_d5xx_read(uint16_t addr)
{
    vicii.last_cpu_val = d5xx_read(addr);
    vicii_clock_read_stretch();
    return vicii.last_cpu_val;
}

void c128_d5xx_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    vicii_clock_write_stretch();
    d5xx_store(addr, value);
}

uint8_t c128_vdc_read(uint16_t addr)
{
    vicii.last_cpu_val = vdc_read(addr);
    vicii_clock_read_stretch();
    return vicii.last_cpu_val;
}

void c128_vdc_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    vicii_clock_write_stretch();
    vdc_store(addr, value);
}

uint8_t c128_c64io_d700_read(uint16_t addr)
{
    vicii.last_cpu_val = c64io_d700_read(addr);
    vicii_clock_read_stretch();
    return vicii.last_cpu_val;
}

void c128_c64io_d700_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    vicii_clock_write_stretch();
    c64io_d700_store(addr, value);
}

uint8_t c128_colorram_read(uint16_t addr)
{
    vicii.last_cpu_val = colorram_read(addr);
    vicii_clock_read_stretch();
    return vicii.last_cpu_val;
}

void c128_colorram_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    vicii_clock_write_stretch();
    colorram_store(addr, value);
}

uint8_t c128_cia1_read(uint16_t addr)
{
    vicii.last_cpu_val = cia1_read(addr);
    vicii_clock_read_stretch();
    return vicii.last_cpu_val;
}

void c128_cia1_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    vicii_clock_write_stretch();
    cia1_store(addr, value);
}

uint8_t c128_cia2_read(uint16_t addr)
{
    vicii.last_cpu_val = cia2_read(addr);
    vicii_clock_read_stretch();
    return vicii.last_cpu_val;
}

void c128_cia2_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    vicii_clock_write_stretch();
    cia2_store(addr, value);
}

uint8_t c128_c64io_de00_read(uint16_t addr)
{
    vicii.last_cpu_val = c64io_de00_read(addr);
    vicii_clock_read_stretch();
    return vicii.last_cpu_val;
}

void c128_c64io_de00_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    vicii_clock_write_stretch();
    c64io_de00_store(addr, value);
}

uint8_t c128_c64io_df00_read(uint16_t addr)
{
    vicii.last_cpu_val = c64io_df00_read(addr);
    vicii_clock_read_stretch();
    return vicii.last_cpu_val;
}

void c128_c64io_df00_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    vicii_clock_write_stretch();
    c64io_df00_store(addr, value);
}
