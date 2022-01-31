/** \file   c128mmu.c
 * \brief   C128 memory management uint
 *
 * \author  Andreas Boose <viceteam@t-online.de>
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 */

/*
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

#include "c128.h"
#include "c128-resources.h"
#include "c128fastiec.h"
#include "c128mem.h"
#include "c128memrom.h"
#include "c128mmu.h"
#include "c64cart.h"
#include "cmdline.h"
#include "functionrom.h"
#include "interrupt.h"
#include "keyboard.h"
#include "log.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "resources.h"
#include "reu.h"
#include "types.h"
#include "vdc.h"
#include "vicii.h"
#include "viciitypes.h"
#include "z80.h"
#include "z80mem.h"

/* #define MMU_DEBUG */

/* MMU register.  */
static uint8_t mmu[12];

/* latches for P0H and P1H */
static uint8_t p0h_latch, p1h_latch;

/* State of the 40/80 column key.  */
static int mmu_column4080_key = 1;

static int force_c64_mode_res = 0;
static int force_c64_mode = 0;

static int mmu_config64 = 0;

/* Logging goes here.  */
static log_t mmu_log = LOG_ERR;

/* ------------------------------------------------------------------------- */

static int set_column4080_key(int val, void *param)
{
    mmu_column4080_key = val ? 1 : 0;

#ifdef HAS_SINGLE_CANVAS
    vdc_set_canvas_refresh(mmu_column4080_key ? 0 : 1);
    vicii_set_canvas_refresh(mmu_column4080_key ? 1 : 0);
#endif
    return 0;
}

static int set_force_c64_mode(int val, void *param)
{
    force_c64_mode_res = val ? 1 : 0;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "C128ColumnKey", 1, RES_EVENT_SAME, NULL,
      &mmu_column4080_key, set_column4080_key, NULL },
    { "Go64Mode", 0, RES_EVENT_SAME, NULL,
      &force_c64_mode_res, set_force_c64_mode, NULL },
    RESOURCE_INT_LIST_END
};

int mmu_resources_init(void)
{
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-40col", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "C128ColumnKey", (resource_value_t) 1,
      NULL, "Activate 40 column mode" },
    { "-80col", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "C128ColumnKey", (resource_value_t) 0,
      NULL, "Activate 80 column mode" },
    { "-go64", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Go64Mode", (resource_value_t) 1,
      NULL, "Always switch to C64 mode on reset" },
    { "+go64", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Go64Mode", (resource_value_t) 0,
      NULL, "Always switch to C128 mode on reset" },
    CMDLINE_LIST_END
};

int mmu_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

static int mmu_is_in_shared_ram(uint16_t address)
{
    unsigned int shared_size;
    if (mmu[6] & 3) {
        shared_size = 2048U << (mmu[6] & 3);
    } else {
        shared_size = 1024;
    }

    if ((mmu[6] & 4) && address < shared_size) {
        return 1;
    }

    if ((mmu[6] & 8) && address >= shared_size) {
        return 1;
    }

    return 0;
}

static uint8_t mmu_is_valid_ram(uint8_t page, uint8_t bank, uint8_t current_bank)
{
    uint8_t hi_ram = (mmu[0] & 0x30) >> 4;
    uint8_t mid_ram = (mmu[0] & 0x0c) >> 2;
    uint8_t lo_ram = (mmu[0] & 0x02) >> 1;
    uint8_t io_ram = (mmu[0] & 0x01);

    /* check if the needed bank is the current ram bank */
    if (bank != current_bank) {
        /* needed bank is not current ram bank, so return not-valid */
        return 0;
    }

    /* check if page is $00-$3f */
    if (page <= 0x3f) {
        /* always ram, return valid */
        return 1;
    }

    /* check if page is $40-$7f */
    if (page >= 0x40 && page <= 0x7f) {
        if (lo_ram == 1) {
            return 1;
        }
    }

    /* check if page is $80-$bf */
    if (page >= 0x80 && page <= 0xbf) {
        if (mid_ram == 3) {
            return 1;
        }
    }

    /* check if page is $c0-$cf or $e0-$fe */
    if ((page >= 0xc0 && page <= 0xcf) || (page >= 0xe0 && page <= 0xfe)) {
        if (hi_ram == 3) {
            return 1;
        }
    }

    /* check if page is $d0-$df */
    if (page >= 0xd0 && page <= 0xdf) {
        if (io_ram == 1 && hi_ram == 3) {
            return 1;
        }
    }

    /* rom area, so return not-valid */
    return 0;
}

static void mmu_toggle_column4080_key(void)
{
    mmu_column4080_key = !mmu_column4080_key;
    resources_set_int("C128ColumnKey", mmu_column4080_key);
    log_message(mmu_log, "40/80 column key %s.", (mmu_column4080_key) ? "released" : "pressed");
}

static void mmu_switch_cpu(int value)
{
    if (value) {
#ifdef MMU_DEBUG
        log_message(mmu_log, "Switching to 8502 CPU.");
#endif
        z80_trigger_dma();
    } else {
#ifdef MMU_DEBUG
        log_message(mmu_log, "Switching to Z80 CPU.");
#endif
        interrupt_trigger_dma(maincpu_int_status, maincpu_clk);
    }
}

static void mmu_set_ram_bank(uint8_t value)
{
    if (c128_full_banks) {
        ram_bank = mem_ram + (((long)value & 0xc0) << 10);
    } else {
        ram_bank = mem_ram + (((long)value & 0x40) << 10);
    }
#ifdef MMU_DEBUG
    if (c128_full_banks) {
        log_message(mmu_log, "Set RAM bank %i.", (value & 0xc0) >> 6);
    } else {
        log_message(mmu_log, "Set RAM bank %i.", (value & 0x40) >> 6);
    }
#endif
}

static void mmu_update_page01_pointers(void)
{
    /* update pointers for page 0/1 in case they or the shared RAM settings changed */
    /* (shared window has priority over P0H/P1H) */
    uint8_t page_zero_bank, page_one_bank, current_bank;

    if (c128_full_banks) {
        page_zero_bank = (mmu[0x8] & 0x3);
        page_one_bank  = (mmu[0xa] & 0x3);
        current_bank = ((mmu[0] >> 6) & 0x3);
    } else {
        page_zero_bank = (mmu[0x8] & 0x1);
        page_one_bank  = (mmu[0xa] & 0x1);
        current_bank = ((mmu[0] >> 6) & 0x1);
    }

    c128_cpu_set_mmu_page_0_target_ram(mmu_is_valid_ram(mmu[0x7], page_zero_bank, current_bank));
    c128_cpu_set_mmu_page_1_target_ram(mmu_is_valid_ram(mmu[0x9], page_one_bank, current_bank));

    c128_cpu_set_mmu_page_0_bank(page_zero_bank);
    c128_cpu_set_mmu_page_1_bank(page_one_bank);

    c128_cpu_set_mmu_zp_sp_shared(mmu_is_in_shared_ram(0));

    if (mmu_is_in_shared_ram(mmu[0x7] << 8)) {
        page_zero_bank = 0;
    }
    if (mmu_is_in_shared_ram(mmu[0x9] << 8)) {
        page_one_bank = 0;
    }
    mem_page_zero = mem_ram + (page_zero_bank << 16) + (mmu[0x7] << 8);
    mem_page_one  = mem_ram + (page_one_bank << 16) + (mmu[0x9] << 8);
}

/* returns 1 if MMU is in C64 mode */
int mmu_is_c64config(void)
{
    return (mmu[5] & 0x40) ? 1 : 0; /* FIXME: is this correct? */
}

static void mmu_switch_to_c64mode(void)
{
#ifdef MMU_DEBUG
    log_message(mmu_log, "mmu_switch_to_c64mode\n");
#endif
    if (force_c64_mode) {
#ifdef MMU_DEBUG
        log_message(mmu_log, "mmu_switch_to_c64mode: force_c64_mode\n");
#endif
        mmu_config64 = 0x07;
        /* force c64-compatible register values */
        /* Note: Don't use mmu_store here, it calls this function. */
        mmu[0] = 0x3e;
        mmu[5] = 0xf7;
        /* force standard addresses for stack and zeropage */
        mmu[7] = 0;
        c128_cpu_set_mmu_page_0(0);
        mmu[8] = 0;
        mmu[9] = 1;
        c128_cpu_set_mmu_page_1(1);
        mmu[10] = 0;
        mmu_update_page01_pointers();
    }
    machine_tape_init_c64();
    mem_update_config(0x80 + mmu_config64);
    keyboard_alternative_set(1);
    machine_kbdbuf_reset_c64();
    machine_autostart_reset_c64();
    force_c64_mode = 0;
}

static void mmu_switch_to_c128mode(void)
{
#ifdef MMU_DEBUG
    log_message(mmu_log, "mmu_switch_to_c128mode\n");
#endif
    machine_tape_init_c128();
    mem_update_config(((mmu[0] & 0x2) ? 0 : 1) |
                      ((mmu[0] & 0x0c) >> 1) |
                      ((mmu[0] & 0x30) >> 1) |
                      ((mmu[0] & 0x40) ? 32 : 0) |
                      ((mmu[0] & 0x1) ? 0 : 64));
    z80mem_update_config((((mmu[0] & 0x1)) ? 0 : 1) | ((mmu[0] & 0x40) ? 2 : 0) | ((mmu[0] & 0x80) ? 4 : 0));
    keyboard_alternative_set(0);
    machine_kbdbuf_reset_c128();
    machine_autostart_reset_c128();
}

/* FIXME: for some reason it is not enough to call the sub functions only when
          the mode actually changes, as a result various init functions are now
          excessively called for mostly no reason */
static void mmu_update_config(void)
{
#ifdef MMU_DEBUG
    log_message(mmu_log, "MMU5 %02x, MMU0 %02x, MMUC %02x\n", mmu[5] & 0x40, mmu[0], mmu_config64);
#endif

    if (mmu[5] & 0x40) {
        mmu_switch_to_c64mode();
    } else {
        mmu_switch_to_c128mode();
    }
}

void mmu_set_config64(int config)
{
    mmu_config64 = config;
    mmu_update_config();
}

/* ------------------------------------------------------------------------- */

uint8_t mmu_peek(uint16_t addr)
{
    addr &= 0xff;

#ifdef MMU_DEBUG
    log_message(mmu_log, "MMU READ $%x.", addr);
#endif

    if (addr < 0xc) {
        switch (addr) {
        case 5: {
            uint8_t exrom = export.exrom;

            if (force_c64_mode) {
                exrom = 1;
            }

            /* 0x80 = 40/80 key released.  */
            return (mmu[5] & 0x0f) | (mmu_column4080_key ? 0x80 : 0) | ((export.game ^ 1) << 4) | ((exrom ^ 1) << 5);
        }

        case 8:
        case 10:
            /* P0H/P1H upper four bits are unused and always return 1 */
            return mmu[addr] | 0xf0;

        case 11:
            /* always return 0x20 unless someone confirms it would ever return
               0x40 in any of the bank2+3 expansions */
            /* return ((c128_full_banks) ? 0x40 : 0x20); */
            return 0x20;

        default:
            return mmu[addr];
        }
    } else {
        return 0xff;
    }
}

uint8_t mmu_read(uint16_t addr)
{
    vicii_handle_pending_alarms_external(0);

    return mmu_peek(addr);
}

void mmu_store(uint16_t address, uint8_t value)
{
    vicii_handle_pending_alarms_external_write();

    address &= 0xff;

#ifdef MMU_DEBUG
    log_message(mmu_log, "MMU STORE $%x <- #$%x.", address, value);
#endif

    if (address < 0xb) {
        uint8_t oldvalue;

        oldvalue = mmu[address];
        mmu[address] = value;

        switch (address) {
            case 0: /* Configuration register (CR).  */
                mmu_set_ram_bank(value);
#ifdef MMU_DEBUG
                log_message(mmu_log,
                            "IO: %s BASLO: %s BASHI: %s KERNAL %s FUNCLO %s.",
                            !(value & 0x1) ? "on" : "off",
                            !(value & 0x2) ? "on" : "off",
                            !(value & 0xc) ? "on" : "off",
                            !(value & 0x30) ? "on" : "off",
                            ((value & 0xc) == 0x4) ? "on" : "off");
#endif
                break;
            case 5: /* Mode configuration register (MCR).  */
                value = (value & 0x7f) | 0x30;
                if ((value & 1) ^ (oldvalue & 1)) {
                    mmu_switch_cpu(value & 1);
                }
                c128fastiec_fast_cpu_direction(value & 8);
                break;
            case 6: /* RAM configuration register (RCR).  */
                mem_set_ram_config(value);
                break;
            case 8:
                /* do not commit yet, update happens on write to p0l */
                mmu[address] = oldvalue;
                p0h_latch = value;
                break;
            case 10:
                /* do not commit yet, update happens on write to p1l */
                mmu[address] = oldvalue;
                p1h_latch = value;
                break;
            case 7:
                mmu[8] = p0h_latch;
                c128_cpu_set_mmu_page_0(mmu[7]);
#ifdef MMU_DEBUG
                log_message(mmu_log, "PAGE ZERO %05x PAGE ONE %05x",
                            (mmu[0x8] & 0x1 ? 0x10000 : 0x00000) + (mmu[0x7] << 8),
                            (mmu[0xa] & 0x1 ? 0x10000 : 0x00000) + (mmu[0x9] << 8));
#endif
                break;
            case 9:
                mmu[10] = p1h_latch;
                c128_cpu_set_mmu_page_1(mmu[9]);
#ifdef MMU_DEBUG
                log_message(mmu_log, "PAGE ZERO %05x PAGE ONE %05x",
                            (mmu[0x8] & 0x1 ? 0x10000 : 0x00000) + (mmu[0x7] << 8),
                            (mmu[0xa] & 0x1 ? 0x10000 : 0x00000) + (mmu[0x9] << 8));
#endif
                break;
        }

        mmu_update_page01_pointers();

        mmu_update_config();
    }
}

/* $FF00 - $FFFF: RAM, Kernal or internal function ROM, with MMU at
   $FF00 - $FF04.  */
uint8_t mmu_ffxx_read(uint16_t addr)
{
    if (addr >= 0xff00 && addr <= 0xff04) {
        vicii.last_cpu_val = mmu[addr & 0xf];
    } else if ((mmu[0] & 0x30) == 0x00) {
        vicii.last_cpu_val = c128memrom_kernal_read(addr);
    } else if ((mmu[0] & 0x30) == 0x10) {
        vicii.last_cpu_val = internal_function_rom_read(addr);
    } else if ((mmu[0] & 0x30) == 0x20) {
        vicii.last_cpu_val = external_function_rom_read(addr);
    } else {
        vicii.last_cpu_val = top_shared_read(addr);
    }
    return vicii.last_cpu_val;
}

uint8_t mmu_ffxx_read_z80(uint16_t addr)
{
    if (addr >= 0xff00 && addr <= 0xff04) {
        return mmu[addr & 0xf];
    }

    return top_shared_read(addr);
}

void mmu_ffxx_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    if (addr == 0xff00) {
        mmu_store(0, value);
        /* FIXME? [SRT] does reu_dma(-1) work here, or should
        it be deferred until later? */
        reu_dma(-1);
    } else {
        if (addr <= 0xff04) {
            mmu_store(0, mmu[addr & 0xf]);
        } else {
            top_shared_store(addr, value);
        }
    }
}

int mmu_dump(void *context, uint16_t addr)
{
    mon_out("CR: bank: %d, $4000-$7FFF: %s, $8000-$BFFF: %s, $C000-$CFFF: %s, $D000-$DFFF: %s, $E000-$FFFF: %s\n",
            (mmu[0] & 0xc0) >> 6,
            (mmu[0] & 2) ? "RAM" : "BASIC ROM low",
            (mmu[0] & 8) ? ((mmu[0] & 4) ? "RAM" : "External Function ROM") : ((mmu[0] & 4) ? "Internal Function ROM" : "BASIC ROM high"),
            (mmu[0] & 0x20) ? ((mmu[0] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[0] & 0x10) ? "Internal Function ROM" : "Kernal ROM"),
            (mmu[0] & 1) ? ((mmu[0] & 0x20) ? ((mmu[0] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[0] & 0x10) ? "Internal Function ROM" : "Kernal ROM")) : "I/O",
            (mmu[0] & 0x20) ? ((mmu[0] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[0] & 0x10) ? "Internal Function ROM" : "Kernal ROM"));

    mon_out("PCRA: bank: %d, $4000-$7FFF: %s, $8000-$BFFF: %s, $C000-$CFFF: %s, $D000-$DFFF: %s, $E000-$FFFF: %s\n",
            (mmu[1] & 0xc0) >> 6,
            (mmu[1] & 2) ? "RAM" : "BASIC ROM low",
            (mmu[1] & 8) ? ((mmu[1] & 4) ? "RAM" : "External Function ROM") : ((mmu[1] & 4) ? "Internal Function ROM" : "BASIC ROM high"),
            (mmu[1] & 0x20) ? ((mmu[1] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[1] & 0x10) ? "Internal Function ROM" : "Kernal ROM"),
            (mmu[1] & 1) ? ((mmu[1] & 0x20) ? ((mmu[1] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[1] & 0x10) ? "Internal Function ROM" : "Kernal ROM")) : "I/O",
            (mmu[1] & 0x20) ? ((mmu[1] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[1] & 0x10) ? "Internal Function ROM" : "Kernal ROM"));

    mon_out("PCRB: bank: %d, $4000-$7FFF: %s, $8000-$BFFF: %s, $C000-$CFFF: %s, $D000-$DFFF: %s, $E000-$FFFF: %s\n",
            (mmu[2] & 0xc0) >> 6,
            (mmu[2] & 2) ? "RAM" : "BASIC ROM low",
            (mmu[2] & 8) ? ((mmu[2] & 4) ? "RAM" : "External Function ROM") : ((mmu[2] & 4) ? "Internal Function ROM" : "BASIC ROM high"),
            (mmu[2] & 0x20) ? ((mmu[2] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[2] & 0x10) ? "Internal Function ROM" : "Kernal ROM"),
            (mmu[2] & 1) ? ((mmu[2] & 0x20) ? ((mmu[2] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[2] & 0x10) ? "Internal Function ROM" : "Kernal ROM")) : "I/O",
            (mmu[2] & 0x20) ? ((mmu[2] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[2] & 0x10) ? "Internal Function ROM" : "Kernal ROM"));

    mon_out("PCRC: bank: %d, $4000-$7FFF: %s, $8000-$BFFF: %s, $C000-$CFFF: %s, $D000-$DFFF: %s, $E000-$FFFF: %s\n",
            (mmu[3] & 0xc0) >> 6,
            (mmu[3] & 2) ? "RAM" : "BASIC ROM low",
            (mmu[3] & 8) ? ((mmu[3] & 4) ? "RAM" : "External Function ROM") : ((mmu[3] & 4) ? "Internal Function ROM" : "BASIC ROM high"),
            (mmu[3] & 0x20) ? ((mmu[3] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[3] & 0x10) ? "Internal Function ROM" : "Kernal ROM"),
            (mmu[3] & 1) ? ((mmu[3] & 0x20) ? ((mmu[3] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[3] & 0x10) ? "Internal Function ROM" : "Kernal ROM")) : "I/O",
            (mmu[3] & 0x20) ? ((mmu[3] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[3] & 0x10) ? "Internal Function ROM" : "Kernal ROM"));

    mon_out("PCRD: bank: %d, $4000-$7FFF: %s, $8000-$BFFF: %s, $C000-$CFFF: %s, $D000-$DFFF: %s, $E000-$FFFF: %s\n",
            (mmu[4] & 0xc0) >> 6,
            (mmu[4] & 2) ? "RAM" : "BASIC ROM low",
            (mmu[4] & 8) ? ((mmu[4] & 4) ? "RAM" : "External Function ROM") : ((mmu[4] & 4) ? "Internal Function ROM" : "BASIC ROM high"),
            (mmu[4] & 0x20) ? ((mmu[1] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[4] & 0x10) ? "Internal Function ROM" : "Kernal ROM"),
            (mmu[4] & 1) ? ((mmu[4] & 0x20) ? ((mmu[4] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[4] & 0x10) ? "Internal Function ROM" : "Kernal ROM")) : "I/O",
            (mmu[4] & 0x20) ? ((mmu[4] & 0x10) ? "RAM" : "External Function ROM") : ((mmu[4] & 0x10) ? "Internal Function ROM" : "Kernal ROM"));

    mon_out("MCR: 40/80 key: %s, Operating mode: %s, EXROM line: %d, GAME line: %d, fast serial: %s, current CPU: %s\n",
            (mmu[5] & 0x80) ? "up" : "down",
            (mmu[5] & 0x40) ? "C64 mode" : "C128 mode",
            (mmu[5] & 0x20) >> 5,
            (mmu[5] & 0x10) >> 4,
            (mmu[5] & 8) ? "serial out" : "serial in",
            (mmu[5] & 1) ? "8502" : "Z80");

    mon_out("RCR: VIC-II RAM bank: %d, Shared RAM location: %s, Shared RAM size: %s\n",
            (mmu[6] & 0xc0) >> 6,
            (mmu[6] & 8) ? ((mmu[6] & 4) ? "bottom and top" : "top") : ((mmu[6] & 4) ? "bottom" : "none"),
            (mmu[6] & 2) ? ((mmu[6] & 1) ? "16KiB" : "8KiB") : ((mmu[6] & 1) ? "4KiB" : "1KiB"));

    mon_out("Page 0 pointer: $%05X\n",
            (unsigned int)(((mmu[8] & 1) << 16) | (mmu[7] << 8)));
    mon_out("Page 1 pointer: $%05X\n",
            (unsigned int)(((mmu[10] & 1) << 16) | (mmu[9] << 8)));

    mon_out("MMU version: %d\n", mmu[11] & 0xf);
    mon_out("Amount of 64KiB blocks present: %d\n", (c128_full_banks) ? 4 : 2);
    return 0;
}

/* ------------------------------------------------------------------------- */

void mmu_init(void)
{
    mmu_log = log_open("MMU");

    set_column4080_key(mmu_column4080_key, NULL);

    mmu[5] = 0;
}

void mmu_reset(void)
{
    uint16_t i;

    for (i = 0; i < 0xb; i++) {
        mmu[i] = 0;
    }
    mmu[9] = 1;
    mmu_update_page01_pointers();

    keyboard_register_column4080_key(mmu_toggle_column4080_key);

    force_c64_mode = force_c64_mode_res;
}
