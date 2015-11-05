/*
 * vicii-mem.c - Memory interface for the MOS6569 (VIC-II) emulation.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *  Daniel Kahlin <daniel@kahlin.net>
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

#include "debug.h"
#include "types.h"
#include "vicii-chip-model.h"
#include "vicii-fetch.h"
#include "vicii-irq.h"
#include "vicii-resources.h"
#include "vicii-mem.h"
#include "vicii.h"
#include "viciitypes.h"


/* Unused bits in VIC-II registers: these are always 1 when read.  */
static int unused_bits_in_registers[0x40] =
{
    0x00 /* $D000 */, 0x00 /* $D001 */, 0x00 /* $D002 */, 0x00 /* $D003 */,
    0x00 /* $D004 */, 0x00 /* $D005 */, 0x00 /* $D006 */, 0x00 /* $D007 */,
    0x00 /* $D008 */, 0x00 /* $D009 */, 0x00 /* $D00A */, 0x00 /* $D00B */,
    0x00 /* $D00C */, 0x00 /* $D00D */, 0x00 /* $D00E */, 0x00 /* $D00F */,
    0x00 /* $D010 */, 0x00 /* $D011 */, 0x00 /* $D012 */, 0x00 /* $D013 */,
    0x00 /* $D014 */, 0x00 /* $D015 */, 0xc0 /* $D016 */, 0x00 /* $D017 */,
    0x01 /* $D018 */, 0x70 /* $D019 */, 0xf0 /* $D01A */, 0x00 /* $D01B */,
    0x00 /* $D01C */, 0x00 /* $D01D */, 0x00 /* $D01E */, 0x00 /* $D01F */,
    0xf0 /* $D020 */, 0xf0 /* $D021 */, 0xf0 /* $D022 */, 0xf0 /* $D023 */,
    0xf0 /* $D024 */, 0xf0 /* $D025 */, 0xf0 /* $D026 */, 0xf0 /* $D027 */,
    0xf0 /* $D028 */, 0xf0 /* $D029 */, 0xf0 /* $D02A */, 0xf0 /* $D02B */,
    0xf0 /* $D02C */, 0xf0 /* $D02D */, 0xf0 /* $D02E */, 0xff /* $D02F */,
    0xff /* $D030 */, 0xff /* $D031 */, 0xff /* $D032 */, 0xff /* $D033 */,
    0xff /* $D034 */, 0xff /* $D035 */, 0xff /* $D036 */, 0xff /* $D037 */,
    0xff /* $D038 */, 0xff /* $D039 */, 0xff /* $D03A */, 0xff /* $D03B */,
    0xff /* $D03C */, 0xff /* $D03D */, 0xff /* $D03E */, 0xff    /* $D03F */
};


/* FIXME plus60k/256k needs these for now */
inline static void vicii_local_store_vbank(WORD addr, BYTE value)
{
    vicii.ram_base_phi2[addr] = value;
}

void vicii_mem_vbank_store(WORD addr, BYTE value)
{
    vicii_local_store_vbank(addr, value);
}

void vicii_mem_vbank_39xx_store(WORD addr, BYTE value)
{
    vicii_local_store_vbank(addr, value);
}

void vicii_mem_vbank_3fxx_store(WORD addr, BYTE value)
{
    vicii_local_store_vbank(addr, value);
}


inline static void store_sprite_x_position_lsb(const WORD addr, BYTE value)
{
    int n;

    if (value == vicii.regs[addr]) {
        return;
    }

    vicii.regs[addr] = value;
    n = addr >> 1;                /* Number of changed sprite.  */

    VICII_DEBUG_REGISTER(("Sprite #%d X position LSB: $%02X", n, value));

    vicii.sprite[n].x = (value | (vicii.regs[0x10] & (1 << n) ? 0x100 : 0));
}

inline static void store_sprite_y_position(const WORD addr, BYTE value)
{
    vicii.regs[addr] = value;
}

static inline void store_sprite_x_position_msb(const WORD addr, BYTE value)
{
    int i;
    BYTE b;

    VICII_DEBUG_REGISTER(("Sprite X position MSBs: $%02X", value));

    if (value == vicii.regs[addr]) {
        return;
    }

    vicii.regs[addr] = value;

    /* Recalculate the sprite X coordinates.  */
    for (i = 0, b = 0x01; i < 8; b <<= 1, i++) {
        vicii.sprite[i].x = (vicii.regs[2 * i] | (value & b ? 0x100 : 0));
    }
}

inline static void update_raster_line(void)
{
    unsigned int new_line;

    new_line = vicii.regs[0x12];
    new_line |= (vicii.regs[0x11] & 0x80) << 1;

    vicii.raster_irq_line = new_line;

    VICII_DEBUG_REGISTER(("Raster interrupt line set to $%04X",
                          vicii.raster_irq_line));
}

inline static void d011_store(BYTE value)
{
    VICII_DEBUG_REGISTER(("Control register: $%02X", value));
    VICII_DEBUG_REGISTER(("$D011 tricks at cycle %d, line $%04X, "
                          "value $%02X", vicii.raster_cycle, vicii.raster_line, value));

    vicii.ysmooth = value & 0x7;

    vicii.regs[0x11] = value;

    update_raster_line();
}

inline static void d012_store(BYTE value)
{
    VICII_DEBUG_REGISTER(("Raster compare register: $%02X", value));

    if (value == vicii.regs[0x12]) {
        return;
    }

    vicii.regs[0x12] = value;

    update_raster_line();
}

inline static void d015_store(const BYTE value)
{
    vicii.regs[0x15] = value;
}

inline static void d016_store(const BYTE value)
{
    VICII_DEBUG_REGISTER(("Control register: $%02X", value));

    vicii.regs[0x16] = value;
}

inline static void d017_store(const BYTE value)
{
    int i;
    BYTE b;

    VICII_DEBUG_REGISTER(("Sprite Y Expand register: $%02X", value));

    if (value == vicii.regs[0x17]) {
        return;
    }

    for (i = 0, b = 0x01; i < 8; b <<= 1, i++) {
        if (!(value & b) && !vicii.sprite[i].exp_flop) {
            /* sprite crunch */
            /* if (cycle == VICII_PAL_CYCLE(15))) { */
            if (cycle_is_check_spr_crunch(vicii.cycle_flags)) {
                BYTE mc = vicii.sprite[i].mc;
                BYTE mcbase = vicii.sprite[i].mcbase;

                /* 0x2a = 0b101010
                   0x15 = 0b010101 */
                vicii.sprite[i].mc = (0x2a & (mcbase & mc)) | (0x15 & (mcbase | mc));

                /* mcbase is set from mc on the following vicii_cycle() call */
            }

            vicii.sprite[i].exp_flop = 1;
        }
    }

    vicii.regs[0x17] = value;
}

inline static void d018_store(const BYTE value)
{
    VICII_DEBUG_REGISTER(("Memory register: $%02X", value));

    if (vicii.regs[0x18] == value) {
        return;
    }

    vicii.regs[0x18] = value;
}

inline static void d019_store(const BYTE value)
{
    vicii.irq_status &= ~((value & 0xf) | 0x80);
    vicii_irq_set_line();

    VICII_DEBUG_REGISTER(("IRQ flag register: $%02X", vicii.irq_status));
}

inline static void d01a_store(const BYTE value)
{
    vicii.regs[0x1a] = value & 0xf;

    vicii_irq_set_line();

    VICII_DEBUG_REGISTER(("IRQ mask register: $%02X", vicii.regs[0x1a]));
}

inline static void d01b_store(const BYTE value)
{
    VICII_DEBUG_REGISTER(("Sprite priority register: $%02X", value));

    vicii.regs[0x1b] = value;
}

inline static void d01c_store(const BYTE value)
{
    VICII_DEBUG_REGISTER(("Sprite Multicolor Enable register: $%02X", value));

    vicii.regs[0x1c] = value;
}

inline static void d01d_store(const BYTE value)
{
    VICII_DEBUG_REGISTER(("Sprite X Expand register: $%02X", value));

    vicii.regs[0x1d] = value;
}

inline static void collision_store(const WORD addr, const BYTE value)
{
    VICII_DEBUG_REGISTER(("(collision register, Read Only)"));
}

inline static void color_reg_store(WORD addr, BYTE value)
{
    vicii.regs[addr] = value;
    vicii.last_color_reg = (BYTE)(addr);
    vicii.last_color_value = value;
}

inline static void d020_store(BYTE value)
{
    VICII_DEBUG_REGISTER(("Border color register: $%02X", value));

    value &= 0x0f;

    color_reg_store(0x20, value);
}

inline static void d021_store(BYTE value)
{
    VICII_DEBUG_REGISTER(("Background #0 color register: $%02X", value));

    value &= 0x0f;

    color_reg_store(0x21, value);
}

inline static void ext_background_store(WORD addr, BYTE value)
{
    value &= 0x0f;

    VICII_DEBUG_REGISTER(("Background color #%d register: $%02X",
                          addr - 0x21, value));

    color_reg_store(addr, value);
}

inline static void d025_store(BYTE value)
{
    value &= 0xf;

    VICII_DEBUG_REGISTER(("Sprite multicolor register #0: $%02X", value));

    color_reg_store(0x25, value);
}

inline static void d026_store(BYTE value)
{
    value &= 0xf;

    VICII_DEBUG_REGISTER(("Sprite multicolor register #1: $%02X", value));

    color_reg_store(0x26, value);
}

inline static void sprite_color_store(WORD addr, BYTE value)
{
    value &= 0xf;

    VICII_DEBUG_REGISTER(("Sprite #%d color register: $%02X",
                          addr - 0x27, value));

    color_reg_store(addr, value);
}

/* Store a value in a VIC-II register.  */
void vicii_store(WORD addr, BYTE value)
{
    addr &= 0x3f;

    vicii.last_bus_phi2 = value;

    VICII_DEBUG_REGISTER(("WRITE $D0%02X at cycle %d of current_line $%04X",
                          addr, vicii.raster_cycle, vicii.raster_line));

    switch (addr) {
        case 0x0:                 /* $D000: Sprite #0 X position LSB */
        case 0x2:                 /* $D002: Sprite #1 X position LSB */
        case 0x4:                 /* $D004: Sprite #2 X position LSB */
        case 0x6:                 /* $D006: Sprite #3 X position LSB */
        case 0x8:                 /* $D008: Sprite #4 X position LSB */
        case 0xa:                 /* $D00a: Sprite #5 X position LSB */
        case 0xc:                 /* $D00c: Sprite #6 X position LSB */
        case 0xe:                 /* $D00e: Sprite #7 X position LSB */
            store_sprite_x_position_lsb(addr, value);
            break;

        case 0x1:                 /* $D001: Sprite #0 Y position */
        case 0x3:                 /* $D003: Sprite #1 Y position */
        case 0x5:                 /* $D005: Sprite #2 Y position */
        case 0x7:                 /* $D007: Sprite #3 Y position */
        case 0x9:                 /* $D009: Sprite #4 Y position */
        case 0xb:                 /* $D00B: Sprite #5 Y position */
        case 0xd:                 /* $D00D: Sprite #6 Y position */
        case 0xf:                 /* $D00F: Sprite #7 Y position */
            store_sprite_y_position(addr, value);
            break;

        case 0x10:                /* $D010: Sprite X position MSB */
            store_sprite_x_position_msb(addr, value);
            break;

        case 0x11:                /* $D011: video mode, Y scroll, 24/25 line
                                     mode and raster MSB */
            d011_store(value);
            break;

        case 0x12:                /* $D012: Raster line compare */
            d012_store(value);
            break;

        case 0x13:                /* $D013: Light Pen X */
        case 0x14:                /* $D014: Light Pen Y */
            break;

        case 0x15:                /* $D015: Sprite Enable */
            d015_store(value);
            break;

        case 0x16:                /* $D016 */
            d016_store(value);
            break;

        case 0x17:                /* $D017: Sprite Y-expand */
            d017_store(value);
            break;

        case 0x18:                /* $D018: Video and char matrix base
                                     address */
            d018_store(value);
            break;

        case 0x19:                /* $D019: IRQ flag register */
            d019_store(value);
            break;

        case 0x1a:                /* $D01A: IRQ mask register */
            d01a_store(value);
            break;

        case 0x1b:                /* $D01B: Sprite priority */
            d01b_store(value);
            break;

        case 0x1c:                /* $D01C: Sprite Multicolor select */
            d01c_store(value);
            break;

        case 0x1d:                /* $D01D: Sprite X-expand */
            d01d_store(value);
            break;

        case 0x1e:                /* $D01E: Sprite-sprite collision */
        case 0x1f:                /* $D01F: Sprite-background collision */
            collision_store(addr, value);
            break;

        case 0x20:                /* $D020: Border color */
            d020_store(value);
            break;

        case 0x21:                /* $D021: Background #0 color */
            d021_store(value);
            break;

        case 0x22:                /* $D022: Background #1 color */
        case 0x23:                /* $D023: Background #2 color */
        case 0x24:                /* $D024: Background #3 color */
            ext_background_store(addr, value);
            break;

        case 0x25:                /* $D025: Sprite multicolor register #0 */
            d025_store(value);
            break;

        case 0x26:                /* $D026: Sprite multicolor register #1 */
            d026_store(value);
            break;

        case 0x27:                /* $D027: Sprite #0 color */
        case 0x28:                /* $D028: Sprite #1 color */
        case 0x29:                /* $D029: Sprite #2 color */
        case 0x2a:                /* $D02A: Sprite #3 color */
        case 0x2b:                /* $D02B: Sprite #4 color */
        case 0x2c:                /* $D02C: Sprite #5 color */
        case 0x2d:                /* $D02D: Sprite #6 color */
        case 0x2e:                /* $D02E: Sprite #7 color */
            sprite_color_store(addr, value);
            break;

        case 0x2f:                /* $D02F: Unused */
        case 0x30:                /* $D030: Unused */
        case 0x31:                /* $D031: Unused */
        case 0x32:                /* $D032: Unused */
        case 0x33:                /* $D033: Unused */
        case 0x34:                /* $D034: Unused */
        case 0x35:                /* $D035: Unused */
        case 0x36:                /* $D036: Unused */
        case 0x37:                /* $D037: Unused */
        case 0x38:                /* $D038: Unused */
        case 0x39:                /* $D039: Unused */
        case 0x3a:                /* $D03A: Unused */
        case 0x3b:                /* $D03B: Unused */
        case 0x3c:                /* $D03C: Unused */
        case 0x3d:                /* $D03D: Unused */
        case 0x3e:                /* $D03E: Unused */
        case 0x3f:                /* $D03F: Unused */
            VICII_DEBUG_REGISTER(("(unused)"));
            break;
    }
}


/* Helper function for reading from $D011/$D012.  */
inline static unsigned int read_raster_y(void)
{
    int raster_y;

    raster_y = vicii.raster_line;

    return raster_y;
}

inline static BYTE d01112_read(WORD addr)
{
    unsigned int tmp = read_raster_y();

    VICII_DEBUG_REGISTER(("Raster Line register %svalue = $%04X",
                          (addr == 0x11 ? "(highest bit) " : ""), tmp));
    if (addr == 0x11) {
        return (vicii.regs[addr] & 0x7f) | ((tmp & 0x100) >> 1);
    } else {
        return tmp & 0xff;
    }
}


inline static BYTE d019_read(void)
{
    return vicii.irq_status | 0x70;
}

inline static BYTE d01e_read(void)
{
    if (!vicii_resources.sprite_sprite_collisions_enabled) {
        VICII_DEBUG_REGISTER(("Sprite-sprite collision mask: $00 "
                              "(emulation disabled)"));
        vicii.sprite_sprite_collisions = 0;
        return 0;
    }

    vicii.regs[0x1e] = vicii.sprite_sprite_collisions;
    vicii.clear_collisions = 0x1e;
    VICII_DEBUG_REGISTER(("Sprite-sprite collision mask: $%02X",
                          vicii.regs[0x1e]));

    return vicii.regs[0x1e];
}

inline static BYTE d01f_read(void)
{
    if (!vicii_resources.sprite_background_collisions_enabled) {
        VICII_DEBUG_REGISTER(("Sprite-background collision mask: $00 "
                              "(emulation disabled)"));
        vicii.sprite_background_collisions = 0;
        return 0;
    }

    vicii.regs[0x1f] = vicii.sprite_background_collisions;
    vicii.clear_collisions = 0x1f;
    VICII_DEBUG_REGISTER(("Sprite-background collision mask: $%02X",
                          vicii.regs[0x1f]));

#if defined (VICII_DEBUG_SB_COLLISIONS)
    log_message(vicii.log,
                "vicii.sprite_background_collisions reset by $D01F "
                "read at line 0x%X.",
                VICII_RASTER_Y(clk));
#endif

    return vicii.regs[0x1f];
}

/* Read a value from a VIC-II register.  */
BYTE vicii_read(WORD addr)
{
    BYTE value;
    addr &= 0x3f;

    VICII_DEBUG_REGISTER(("READ $D0%02X at cycle %d of current_line $%04X:",
                          addr, vicii.raster_cycle, vicii.raster_line));

    /* Note: we use hardcoded values instead of `unused_bits_in_registers[]'
       here because this is a little bit faster.  */
    switch (addr) {
        case 0x0:                 /* $D000: Sprite #0 X position LSB */
        case 0x2:                 /* $D002: Sprite #1 X position LSB */
        case 0x4:                 /* $D004: Sprite #2 X position LSB */
        case 0x6:                 /* $D006: Sprite #3 X position LSB */
        case 0x8:                 /* $D008: Sprite #4 X position LSB */
        case 0xa:                 /* $D00a: Sprite #5 X position LSB */
        case 0xc:                 /* $D00c: Sprite #6 X position LSB */
        case 0xe:                 /* $D00e: Sprite #7 X position LSB */
            VICII_DEBUG_REGISTER(("Sprite #%d X position LSB: $%02X",
                                  addr >> 1, vicii.regs[addr]));
            value = vicii.regs[addr];
            break;

        case 0x1:                 /* $D001: Sprite #0 Y position */
        case 0x3:                 /* $D003: Sprite #1 Y position */
        case 0x5:                 /* $D005: Sprite #2 Y position */
        case 0x7:                 /* $D007: Sprite #3 Y position */
        case 0x9:                 /* $D009: Sprite #4 Y position */
        case 0xb:                 /* $D00B: Sprite #5 Y position */
        case 0xd:                 /* $D00D: Sprite #6 Y position */
        case 0xf:                 /* $D00F: Sprite #7 Y position */
            VICII_DEBUG_REGISTER(("Sprite #%d Y position: $%02X",
                                  addr >> 1, vicii.regs[addr]));
            value = vicii.regs[addr];
            break;

        case 0x10:                /* $D010: Sprite X position MSB */
            VICII_DEBUG_REGISTER(("Sprite X position MSB: $%02X",
                                  vicii.regs[addr]));
            value = vicii.regs[addr];
            break;

        case 0x11:              /* $D011: video mode, Y scroll, 24/25 line mode
                                   and raster MSB */
        case 0x12:              /* $D012: Raster line compare */
            value = d01112_read(addr);
            break;

        case 0x13:                /* $D013: Light Pen X */
            VICII_DEBUG_REGISTER(("Light pen X: %d", vicii.light_pen.x));
            value = vicii.light_pen.x;
            break;

        case 0x14:                /* $D014: Light Pen Y */
            VICII_DEBUG_REGISTER(("Light pen Y: %d", vicii.light_pen.y));
            value = vicii.light_pen.y;
            break;

        case 0x15:                /* $D015: Sprite Enable */
            VICII_DEBUG_REGISTER(("Sprite Enable register: $%02X",
                                  vicii.regs[addr]));
            value = vicii.regs[addr];
            break;

        case 0x16:                /* $D016 */
            VICII_DEBUG_REGISTER(("$D016 Control register read: $%02X",
                                  vicii.regs[addr]));
            value = vicii.regs[addr] | 0xc0;
            break;

        case 0x17:                /* $D017: Sprite Y-expand */
            VICII_DEBUG_REGISTER(("Sprite Y Expand register: $%02X",
                                  vicii.regs[addr]));
            value = vicii.regs[addr];
            break;

        case 0x18:              /* $D018: Video and char matrix base address */
            VICII_DEBUG_REGISTER(("Video memory address register: $%02X",
                                  vicii.regs[addr]));
            value = vicii.regs[addr] | 0x1;
            break;

        case 0x19:                /* $D019: IRQ flag register */
            value = d019_read();
            VICII_DEBUG_REGISTER(("Interrupt register: $%02X", value));
            break;

        case 0x1a:                /* $D01A: IRQ mask register  */
            value = vicii.regs[addr] | 0xf0;
            VICII_DEBUG_REGISTER(("Mask register: $%02X", value));
            break;

        case 0x1b:                /* $D01B: Sprite priority */
            VICII_DEBUG_REGISTER(("Sprite Priority register: $%02X",
                                  vicii.regs[addr]));
            value = vicii.regs[addr];
            break;

        case 0x1c:                /* $D01C: Sprite Multicolor select */
            VICII_DEBUG_REGISTER(("Sprite Multicolor Enable register: $%02X",
                                  vicii.regs[addr]));
            value = vicii.regs[addr];
            break;

        case 0x1d:                /* $D01D: Sprite X-expand */
            VICII_DEBUG_REGISTER(("Sprite X Expand register: $%02X",
                                  vicii.regs[addr]));
            value = vicii.regs[addr];
            break;

        case 0x1e:                /* $D01E: Sprite-sprite collision */
            value = d01e_read();
            break;

        case 0x1f:                /* $D01F: Sprite-background collision */
            value = d01f_read();
            break;

        case 0x20:                /* $D020: Border color */
            VICII_DEBUG_REGISTER(("Border Color register: $%02X",
                                  vicii.regs[addr]));
            value = vicii.regs[addr] | 0xf0;
            break;

        case 0x21:                /* $D021: Background #0 color */
        case 0x22:                /* $D022: Background #1 color */
        case 0x23:                /* $D023: Background #2 color */
        case 0x24:                /* $D024: Background #3 color */
            VICII_DEBUG_REGISTER(("Background Color #%d register: $%02X",
                                  addr - 0x21, vicii.regs[addr]));
            value = vicii.regs[addr] | 0xf0;
            break;

        case 0x25:                /* $D025: Sprite multicolor register #0 */
        case 0x26:                /* $D026: Sprite multicolor register #1 */
            VICII_DEBUG_REGISTER(("Multicolor register #%d: $%02X",
                                  addr - 0x22, vicii.regs[addr]));
            value = vicii.regs[addr] | 0xf0;
            break;

        case 0x27:                /* $D027: Sprite #0 color */
        case 0x28:                /* $D028: Sprite #1 color */
        case 0x29:                /* $D029: Sprite #2 color */
        case 0x2a:                /* $D02A: Sprite #3 color */
        case 0x2b:                /* $D02B: Sprite #4 color */
        case 0x2c:                /* $D02C: Sprite #5 color */
        case 0x2d:                /* $D02D: Sprite #6 color */
        case 0x2e:                /* $D02E: Sprite #7 color */
            VICII_DEBUG_REGISTER(("Sprite #%d color: $%02X",
                                  addr - 0x22, vicii.regs[addr]));
            value = vicii.regs[addr] | 0xf0;
            break;

        case 0x2f:                /* $D02F: Unused */
        case 0x30:                /* $D030: Unused */
        case 0x31:                /* $D031: Unused */
        case 0x32:                /* $D032: Unused */
        case 0x33:                /* $D033: Unused */
        case 0x34:                /* $D034: Unused */
        case 0x35:                /* $D035: Unused */
        case 0x36:                /* $D036: Unused */
        case 0x37:                /* $D037: Unused */
        case 0x38:                /* $D038: Unused */
        case 0x39:                /* $D039: Unused */
        case 0x3a:                /* $D03A: Unused */
        case 0x3b:                /* $D03B: Unused */
        case 0x3c:                /* $D03C: Unused */
        case 0x3d:                /* $D03D: Unused */
        case 0x3e:                /* $D03E: Unused */
        case 0x3f:                /* $D03F: Unused */
        default:
            value = 0xff;
            break;
    }

    vicii.last_bus_phi2 = value;
    return value;
}

inline static BYTE d019_peek(void)
{
    return vicii.irq_status | 0x70;
}

BYTE vicii_peek(WORD addr)
{
    addr &= 0x3f;

    switch (addr) {
        case 0x11:            /* $D011: video mode, Y scroll, 24/25 line mode
                                 and raster MSB */
            return (vicii.regs[addr] & 0x7f) | ((read_raster_y () & 0x100) >> 1);
        case 0x12:            /* $D012: Raster line LSB */
            return read_raster_y() & 0xff;
        case 0x13:            /* $D013: Light Pen X */
            return vicii.light_pen.x;
        case 0x14:            /* $D014: Light Pen Y */
            return vicii.light_pen.y;
        case 0x19:
            return d019_peek();
        case 0x1e:            /* $D01E: Sprite-sprite collision */
            return vicii.sprite_sprite_collisions;
        case 0x1f:            /* $D01F: Sprite-background collision */
            return vicii.sprite_background_collisions;
        default:
            return vicii.regs[addr] | unused_bits_in_registers[addr];
    }
}
