/*
 * viciivsid-mem.c - Memory interface for the MOS6569 (VIC-II) emulation.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
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

#include "alarm.h"
#include "c64cia.h"
#include "debug.h"
#include "maincpu.h"
#include "mem.h"
#include "raster-changes.h"
#include "raster-sprite-status.h"
#include "raster-sprite.h"
#include "types.h"
#include "vicii-badline.h"
#include "vicii-fetch.h"
#include "vicii-irq.h"
#include "vicii-resources.h"
#include "vicii-sprites.h"
#include "vicii-mem.h"
#include "vicii.h"
#include "viciitypes.h"
#include "viewport.h"


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

/* Store a value in the video bank (it is assumed to be in RAM).  */
inline static void vicii_local_store_vbank(WORD addr, BYTE value)
{
    unsigned int f;

    do {
        CLOCK mclk;

        /* WARNING: Assumes `maincpu_rmw_flag' is 0 or 1.  */
        mclk = maincpu_clk - maincpu_rmw_flag - 1;
        f = 0;

        if (mclk >= vicii.fetch_clk) {
            /* If the fetch starts here, the sprite fetch routine should
               get the new value, not the old one.  */
            if (mclk == vicii.fetch_clk) {
                vicii.ram_base_phi2[addr] = value;
            }

            /* The sprite DMA check can be followed by a real fetch.
               Save the location to execute the store if a real
               fetch actually happens.  */
            if (vicii.fetch_idx == VICII_CHECK_SPRITE_DMA) {
                vicii.store_clk = mclk;
                vicii.store_value = value;
                vicii.store_addr = addr;
            }

            vicii_fetch_alarm_handler(maincpu_clk - vicii.fetch_clk, NULL);
            f = 1;
            /* WARNING: Assumes `maincpu_rmw_flag' is 0 or 1.  */
            mclk = maincpu_clk - maincpu_rmw_flag - 1;
            vicii.store_clk = CLOCK_MAX;
        }

        if (mclk >= vicii.draw_clk) {
            vicii_raster_draw_alarm_handler(0, NULL);
            f = 1;
        }
    } while (f);

    vicii.ram_base_phi2[addr] = value;
}

/* Encapsulate inlined function for other modules */
void vicii_mem_vbank_store(WORD addr, BYTE value)
{
    vicii_local_store_vbank(addr, value);
}

/* As `store_vbank()', but for the $3900...$39FF address range.  */
void vicii_mem_vbank_39xx_store(WORD addr, BYTE value)
{
    vicii_local_store_vbank(addr, value);

    if (vicii.idle_data_location == IDLE_39FF && (addr & 0x3fff) == 0x39ff) {
        raster_changes_foreground_add_int
            (&vicii.raster,
            VICII_RASTER_CHAR(VICII_RASTER_CYCLE(maincpu_clk)),
            &vicii.idle_data,
            value);
    }
}

/* As `store_vbank()', but for the $3F00...$3FFF address range.  */
void vicii_mem_vbank_3fxx_store(WORD addr, BYTE value)
{
    vicii_local_store_vbank(addr, value);

    if ((addr & 0x3fff) == 0x3fff) {
        if (vicii.idle_data_location == IDLE_3FFF) {
            raster_changes_foreground_add_int
                (&vicii.raster,
                VICII_RASTER_CHAR(VICII_RASTER_CYCLE(maincpu_clk)),
                &vicii.idle_data,
                value);
        }

        if (vicii.raster.sprite_status->visible_msk != 0
            || vicii.raster.sprite_status->dma_msk != 0) {
            vicii.idle_3fff[vicii.num_idle_3fff].cycle = maincpu_clk;
            vicii.idle_3fff[vicii.num_idle_3fff].value = value;
            vicii.num_idle_3fff++;
        }
    }
}


inline static void store_sprite_x_position_lsb(const WORD addr, BYTE value)
{
    int n;
    int new_x;

    if (value == vicii.regs[addr]) {
        return;
    }

    vicii.regs[addr] = value;

    n = addr >> 1;                /* Number of changed sprite.  */

    VICII_DEBUG_REGISTER(("Sprite #%d X position LSB: $%02X", n, value));

    new_x = (value | (vicii.regs[0x10] & (1 << n) ? 0x100 : 0));
    vicii_sprites_set_x_position(n, new_x, VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk)));
}

inline static void store_sprite_y_position(const WORD addr, BYTE value)
{
    int cycle;

    VICII_DEBUG_REGISTER(("Sprite #%d Y position: $%02X", addr >> 1, value));

    if (vicii.regs[addr] == value) {
        return;
    }

    cycle = VICII_RASTER_CYCLE(maincpu_clk);

    if (cycle == vicii.sprite_fetch_cycle + 1
        && value == (vicii.raster.current_line & 0xff)) {
        vicii.fetch_idx = VICII_CHECK_SPRITE_DMA;
        vicii.fetch_clk = (VICII_LINE_START_CLK(maincpu_clk) + vicii.sprite_fetch_cycle + 1);
        alarm_set(vicii.raster_fetch_alarm, vicii.fetch_clk);
    }

    vicii.raster.sprite_status->sprites[addr >> 1].y = value;
    vicii.regs[addr] = value;
}

static inline void store_sprite_x_position_msb(const WORD addr, BYTE value)
{
    int i;
    BYTE b;
    int raster_x;

    VICII_DEBUG_REGISTER(("Sprite X position MSBs: $%02X", value));

    if (value == vicii.regs[addr]) {
        return;
    }

    raster_x = VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk));

    vicii.regs[addr] = value;

    /* Recalculate the sprite X coordinates.  */
    for (i = 0, b = 0x01; i < 8; b <<= 1, i++) {
        int new_x;

        new_x = (vicii.regs[2 * i] | (value & b ? 0x100 : 0));
        vicii_sprites_set_x_position(i, new_x, raster_x);
    }
}

inline static void check_lower_upper_border(const BYTE value,
                                            unsigned int line, int cycle)
{
    if ((value ^ vicii.regs[0x11]) & 8) {
        if (value & 0x8) {
            /* 24 -> 25 row mode switch.  */

            vicii.raster.display_ystart = vicii.row_25_start_line;
            vicii.raster.display_ystop = vicii.row_25_stop_line;

            if (line == vicii.row_24_stop_line && cycle > 0) {
                /* If on the first line of the 24-line border, we
                   still see the 25-line (lowmost) border because the
                   border flip flop has already been turned on.  */
                vicii.raster.blank_enabled = 1;
            } else {
                if (!vicii.raster.blank && line == vicii.row_24_start_line
                    && cycle > 0) {
                    /* A 24 -> 25 switch somewhere on the first line of
                       the 24-row mode is enough to disable screen
                       blanking.  */
                    vicii.raster.blank_enabled = 0;
                }

                if (line == vicii.raster.display_ystart && cycle > 0
                    && vicii.raster.blank == 0) {
                    /* A 24 -> 25 switch somewhere on the first line of
                       the 25-row mode is enough to disable screen
                       blanking even if blank bit is switched on in the
                       same cycle (and set later in d011_store) */
                    vicii.raster.blank_enabled = 0;
                }
            }
            VICII_DEBUG_REGISTER(("25 line mode enabled"));
        } else {
            /* 25 -> 24 row mode switch.  */

            vicii.raster.display_ystart = vicii.row_24_start_line;
            vicii.raster.display_ystop = vicii.row_24_stop_line;

            /* If on the last line of the 25-line border, we still see the
               24-line (upmost) border because the border flip flop has
               already been turned off.  */
            if (!vicii.raster.blank && line == vicii.row_25_start_line
                && cycle > 0) {
                vicii.raster.blank_enabled = 0;
            } else {
                if (line == vicii.row_25_stop_line && cycle > 0) {
                    vicii.raster.blank_enabled = 1;
                }
            }

            VICII_DEBUG_REGISTER(("24 line mode enabled"));
        }
    }

    /* Check if border is already disabled even if DEN will be cleared */
    if (line == vicii.raster.display_ystart && cycle > 0 && !vicii.raster.blank) {
        vicii.raster.blank_off = 1;
    }
}

inline static void d011_store(BYTE value)
{
    int cycle, old_allow_bad_lines;
    unsigned int line;

    cycle = VICII_RASTER_CYCLE(maincpu_clk);
    line = VICII_RASTER_Y(maincpu_clk);

    VICII_DEBUG_REGISTER(("Control register: $%02X", value));
    VICII_DEBUG_REGISTER(("$D011 tricks at cycle %d, line $%04X, "
                          "value $%02X", cycle, line, value));

    vicii_irq_check_state(value, 1);

    /* This is the funniest part... handle bad line tricks.  */
    old_allow_bad_lines = vicii.allow_bad_lines;

    if (line == vicii.first_dma_line && cycle == 0) {
        vicii.allow_bad_lines = (value & 0x10) ? 1 : 0;
    }

    if (VICII_RASTER_Y(maincpu_clk - 1) == vicii.first_dma_line && (value & 0x10) != 0) {
        vicii.allow_bad_lines = 1;
    }

    if ((vicii.raster.ysmooth != (value & 7)
         || vicii.allow_bad_lines != old_allow_bad_lines)
        && line >= vicii.first_dma_line
        && line <= vicii.last_dma_line) {
        vicii_badline_check_state(value, cycle, line, old_allow_bad_lines);
    }

    vicii.raster.ysmooth = value & 0x7;

    /* Check for 24 <-> 25 line mode switch.  */
    check_lower_upper_border(value, line, cycle);

    vicii.raster.blank = !(value & 0x10); /* `DEN' bit.  */

    vicii.regs[0x11] = value;

    /* FIXME: save time.  */
    vicii_update_video_mode(cycle);
}

inline static void d012_store(BYTE value)
{
    VICII_DEBUG_REGISTER(("Raster compare register: $%02X", value));

    if (value == vicii.regs[0x12]) {
        return;
    }

    vicii.regs[0x12] = value;

    vicii_irq_check_state(value, 0);

    VICII_DEBUG_REGISTER(("Raster interrupt line set to $%04X", vicii.raster_irq_line));
}

inline static void d015_store(const BYTE value)
{
    int cycle;

    VICII_DEBUG_REGISTER(("Sprite Enable register: $%02X", value));

    cycle = VICII_RASTER_CYCLE(maincpu_clk);

    /* On the real C64, sprite DMA is checked two times: first at
       `VICII_SPRITE_FETCH_CYCLE', and then at `VICII_SPRITE_FETCH_CYCLE +
       1'.  In the average case, one DMA check is OK and there is no need to
       emulate both, but we have to kludge things a bit in case sprites are
       activated at cycle `VICII_SPRITE_FETCH_CYCLE + 1'.  */
    if (cycle == vicii.sprite_fetch_cycle + 1
        && ((value ^ vicii.regs[0x15]) & value) != 0) {
        vicii.fetch_idx = VICII_CHECK_SPRITE_DMA;
        vicii.fetch_clk = (VICII_LINE_START_CLK(maincpu_clk) + vicii.sprite_fetch_cycle + 1);
        alarm_set(vicii.raster_fetch_alarm, vicii.fetch_clk);
    }

    /* Sprites are turned on: force a DMA check.  */
    if (vicii.raster.sprite_status->visible_msk == 0
        && vicii.raster.sprite_status->dma_msk == 0
        && value != 0) {
        if ((vicii.fetch_idx == VICII_FETCH_MATRIX
             && vicii.fetch_clk > maincpu_clk
             && cycle > VICII_FETCH_CYCLE
             && cycle <= vicii.sprite_fetch_cycle)
            || vicii.raster.current_line < vicii.first_dma_line
            || vicii.raster.current_line >= vicii.last_dma_line) {
            CLOCK new_fetch_clk;

            new_fetch_clk = (VICII_LINE_START_CLK(maincpu_clk) + vicii.sprite_fetch_cycle);
            if (cycle > vicii.sprite_fetch_cycle) {
                new_fetch_clk += vicii.cycles_per_line;
            }
            if (new_fetch_clk < vicii.fetch_clk) {
                vicii.fetch_idx = VICII_CHECK_SPRITE_DMA;
                vicii.fetch_clk = new_fetch_clk;
                alarm_set(vicii.raster_fetch_alarm, vicii.fetch_clk);
            }
        }
    }

    vicii.regs[0x15] = vicii.raster.sprite_status->visible_msk = value;
}

inline static void check_lateral_border(const BYTE value, int cycle,
                                        raster_t *raster)
{
    if ((value & 0x8) != (vicii.regs[0x16] & 0x8)) {
        if (value & 0x8) {
            /* 40 column mode.  */
            if (cycle <= 17) {
                raster->display_xstart = VICII_40COL_START_PIXEL;
            } else {
                raster_changes_next_line_add_int(raster,
                                                 &raster->display_xstart,
                                                 VICII_40COL_START_PIXEL);
            }
            if (cycle <= 56) {
                raster->display_xstop = VICII_40COL_STOP_PIXEL;
            } else {
                raster_changes_next_line_add_int(raster,
                                                 &raster->display_xstop,
                                                 VICII_40COL_STOP_PIXEL);
            }
            VICII_DEBUG_REGISTER(("40 column mode enabled"));

            /* If CSEL changes from 0 to 1 at cycle 17, the border is
               not turned off and this line is blank.  */
            if (cycle == 17 && !(vicii.regs[0x16] & 0x8)) {
                raster->blank_this_line = 1;
            }
        } else {
            /* 38 column mode.  */
            if (cycle <= 17) {
                raster->display_xstart = VICII_38COL_START_PIXEL;
            } else {
                raster_changes_next_line_add_int(raster,
                                                 &raster->display_xstart,
                                                 VICII_38COL_START_PIXEL);
            }
            if (cycle <= 56) {
                raster->display_xstop = VICII_38COL_STOP_PIXEL;
            } else {
                raster_changes_next_line_add_int(raster,
                                                 &raster->display_xstop,
                                                 VICII_38COL_STOP_PIXEL);
            }
            VICII_DEBUG_REGISTER(("38 column mode enabled"));

            /* If CSEL changes from 1 to 0 at cycle 56, the lateral
               border is open.  */
            if (cycle == 56 && (vicii.regs[0x16] & 0x8)
                && (raster->open_left_border
                    || (!raster->blank_enabled
                        && raster->current_line != raster->display_ystop))) {
                raster->open_right_border = 1;
                switch (vicii.get_background_from_vbuf) {
                    case VICII_HIRES_BITMAP_MODE:
                        raster_changes_background_add_int(
                            &vicii.raster,
                            VICII_RASTER_X(56),
                            &vicii.raster.xsmooth_color,
                            vicii.background_color_source & 0x0f);
                        break;
                    case VICII_EXTENDED_TEXT_MODE:
                        raster_changes_background_add_int(
                            &vicii.raster,
                            VICII_RASTER_X(56),
                            &vicii.raster.xsmooth_color,
                            vicii.regs[0x21 + (vicii.background_color_source
                                               >> 6)]);
                        break;
                }
            }
        }
    }
}

inline static void d016_store(const BYTE value)
{
    raster_t *raster;
    int cycle;
    BYTE xsmooth;

    VICII_DEBUG_REGISTER(("Control register: $%02X", value));

    raster = &vicii.raster;
    cycle = VICII_RASTER_CYCLE(maincpu_clk);
    xsmooth = value & 7;

    if (xsmooth != (vicii.regs[0x16] & 7)) {
        if (xsmooth < (vicii.regs[0x16] & 7)) {
            if (cycle < 56) {
                raster_changes_foreground_add_int(raster,
                                                  VICII_RASTER_CHAR(cycle) - 2,
                                                  &raster->xsmooth_shift_left,
                                                  (vicii.regs[0x16] & 7) - xsmooth);
            }
        } else {
            raster_changes_background_add_int(raster,
                                              VICII_RASTER_X(cycle),
                                              &raster->xsmooth_shift_right,
                                              xsmooth - (vicii.regs[0x16] & 7));
            raster_changes_sprites_add_int(raster,
                                           VICII_RASTER_X(cycle) + 8
                                           + (vicii.regs[0x16] & 7),
                                           &raster->sprite_xsmooth_shift_right,
                                           1);
            raster_changes_sprites_add_int(raster,
                                           VICII_RASTER_X(cycle) + 8 + xsmooth,
                                           &raster->sprite_xsmooth_shift_right,
                                           0);
        }
        raster_changes_foreground_add_int(raster,
                                          VICII_RASTER_CHAR(cycle) - 1,
                                          &raster->xsmooth,
                                          xsmooth);
        raster_changes_sprites_add_int(raster,
                                       VICII_RASTER_X(cycle) + 8 + xsmooth,
                                       &raster->sprite_xsmooth,
                                       xsmooth);
    }

    /* Bit 4 (CSEL) selects 38/40 column mode.  */
    check_lateral_border(value, cycle, raster);

    vicii.regs[0x16] = value;

    vicii_update_video_mode(cycle);
}

inline static void d017_store(const BYTE value)
{
    raster_sprite_status_t *sprite_status;
    int cycle;
    int i;
    BYTE b;

    VICII_DEBUG_REGISTER(("Sprite Y Expand register: $%02X", value));

    if (value == vicii.regs[0x17]) {
        return;
    }

    cycle = VICII_RASTER_CYCLE(maincpu_clk);
    sprite_status = vicii.raster.sprite_status;

    for (i = 0, b = 0x01; i < 8; b <<= 1, i++) {
        raster_sprite_t *sprite;

        sprite = sprite_status->sprites + i;

        sprite->y_expanded = value & b ? 1 : 0;

        if (!sprite->y_expanded && !sprite->exp_flag) {
            /* Sprite crunch!  */
            if (cycle == 15) {
                sprite->memptr_inc
                    = vicii_sprites_crunch_table[sprite->memptr];
            } else if (cycle < 15 || cycle >= vicii.sprite_fetch_cycle) {
                sprite->memptr_inc = 3;
            }
            sprite->exp_flag = 1;
        }

        /* (Enabling sprite Y-expansion never causes side effects.)  */
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
    vicii_update_memory_ptrs(VICII_RASTER_CYCLE(maincpu_clk));
}

inline static void d019_store(const BYTE value)
{
    /* Emulates Read-Modify-Write behaviour. */
    if (maincpu_rmw_flag) {
        vicii.irq_status &= ~((vicii.last_read & 0xf) | 0x80);
        if (maincpu_clk - 1 > vicii.raster_irq_clk
            && vicii.raster_irq_line < (unsigned int)vicii.screen_height) {
            if (maincpu_clk - 2 == vicii.raster_irq_clk) {
                vicii_irq_next_frame();
            } else {
                vicii_irq_alarm_handler(0, NULL);
            }
        }
    }

    if ((value & 1) && maincpu_clk > vicii.raster_irq_clk
        && vicii.raster_irq_line < (unsigned int)vicii.screen_height) {
        if (maincpu_clk - 1 == vicii.raster_irq_clk) {
            vicii_irq_next_frame();
        } else {
            vicii_irq_alarm_handler(0, NULL);
        }
    }

    vicii.irq_status &= ~((value & 0xf) | 0x80);
    vicii_irq_set_line();

    VICII_DEBUG_REGISTER(("IRQ flag register: $%02X", vicii.irq_status));
}

inline static void d01a_store(const BYTE value)
{
    vicii.regs[0x1a] = value & 0xf;

    vicii_irq_set_line();

/*    VICII_DEBUG_REGISTER(("IRQ mask register: $%02X", vicii.regs[addr])); */
}

inline static void d01b_store(const BYTE value)
{
    int i;
    BYTE b;
    int raster_x;

    VICII_DEBUG_REGISTER(("Sprite priority register: $%02X", value));

    if (value == vicii.regs[0x1b]) {
        return;
    }

    raster_x = VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk));

    for (i = 0, b = 0x01; i < 8; b <<= 1, i++) {
        raster_sprite_t *sprite;

        sprite = vicii.raster.sprite_status->sprites + i;

        if (sprite->x < raster_x) {
            raster_changes_next_line_add_int(&vicii.raster,
                                             &sprite->in_background,
                                             value & b ? 1 : 0);
        } else {
            sprite->in_background = value & b ? 1 : 0;
        }
    }

    vicii.regs[0x1b] = value;
}

inline static void d01c_store(const BYTE value)
{
    int i;
    BYTE b;
    int raster_x;
    int sprite_x;
    int delayed_load, delayed_shift, delayed_pixel;
    int x_exp;

    VICII_DEBUG_REGISTER(("Sprite Multicolor Enable register: $%02X", value));

    if (value == vicii.regs[0x1c]) {
        return;
    }

    raster_x = VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk));

    for (i = 0, b = 0x01; i < 8; b <<= 1, i++) {
        raster_sprite_t *sprite;

        sprite = vicii.raster.sprite_status->sprites + i;

        if ((vicii.regs[0x1c] & b) != (value & b)) {
            /* Test for MC bug condition */
            sprite_x = (vicii.regs[2 * i] | (vicii.regs[0x10] & b ? 0x100 : 0)) + vicii.screen_leftborderwidth - 0x20;
            x_exp = vicii.regs[0x1d] & b;
            delayed_pixel = 6;

            if (sprite_x < raster_x && sprite_x + (x_exp ? 48 : 24) >= raster_x) {
                if (value & b) {
                    /* HIRES -> MC */
                    if (x_exp) {
                        delayed_load = sprite_x % 2;
                        delayed_shift = ((sprite_x & 1) == ((sprite_x >> 1) & 1) ? 1 : 0);
                    } else {
                        delayed_shift = (sprite_x & 1);
                        delayed_load = 0;
                    }
                    delayed_pixel = 6 - delayed_load;
                } else {
                    /* MC -> HIRES */
                    delayed_shift = 0;
                    delayed_load = 0;
                    if (x_exp) {
                        delayed_pixel = (sprite_x & 1 ? 7 : 8 - (sprite_x & 2));
                    } else {
                        delayed_pixel = 6 + (sprite_x & 1);
                    }
                }
                raster_changes_sprites_add_int(&vicii.raster,
                                               raster_x + delayed_pixel,
                                               &sprite->mc_bug, delayed_shift << 1 | delayed_load);
            }

            raster_changes_sprites_add_int(&vicii.raster,
                                           raster_x + delayed_pixel,
                                           &sprite->multicolor, value & b ? 1 : 0);
        }
    }

    vicii.regs[0x1c] = value;
}

inline static void d01d_store(const BYTE value)
{
    int raster_x;
    int i;
    BYTE b;

    VICII_DEBUG_REGISTER(("Sprite X Expand register: $%02X", value));

    if (value == vicii.regs[0x1d]) {
        return;
    }

    /* FIXME: The offset of 6 was calibrated with the GULP demo and CCS */
    raster_x = VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk)) + 6;

    for (i = 0, b = 0x01; i < 8; b <<= 1, i++) {
        raster_sprite_t *sprite;

        sprite = vicii.raster.sprite_status->sprites + i;

        if ((value & b) != (vicii.regs[0x1d] & b)) {
            raster_changes_sprites_add_int(&vicii.raster,
                                           raster_x,
                                           &sprite->x_expanded,
                                           value & b ? 1 : 0);

            /* We have to shift the sprite virtually for the drawing code */
            if (raster_x > sprite->x) {
                int actual_shift;
                if (value & b) {
                    actual_shift = sprite->x - raster_x;
                } else {
                    actual_shift = (raster_x - sprite->x) / 2;
                }

                sprite->x_shift_sum += actual_shift;

                raster_changes_sprites_add_int(&vicii.raster,
                                               raster_x,
                                               &sprite->x_shift,
                                               sprite->x_shift_sum);
            }
        }
    }

    vicii.regs[0x1d] = value;
}

inline static void collision_store(const WORD addr, const BYTE value)
{
    VICII_DEBUG_REGISTER(("(collision register, Read Only)"));
}

inline static void d020_store(BYTE value)
{
    VICII_DEBUG_REGISTER(("Border color register: $%02X", value));

    value = (vicii.regs[0x20] & 0xf0) | (value & 0x0f);

    if (vicii.regs[0x20] == value) {
        return;
    }

    vicii.regs[0x20] = value;

    raster_changes_border_add_int(&vicii.raster,
                                  VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk)),
                                  (int *)&vicii.raster.border_color,
                                  value);
}

inline static void d021_store(BYTE value)
{
    int x_pos;

    value = (vicii.regs[0x21] & 0xf0) | (value & 0x0f);

    VICII_DEBUG_REGISTER(("Background #0 color register: $%02X", value));

    if (vicii.regs[0x21] == value) {
        return;
    }

    x_pos = VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk));

    if (!vicii.force_black_overscan_background_color) {
        raster_changes_background_add_int(&vicii.raster, x_pos,
                                          &vicii.raster.idle_background_color,
                                          value);
        raster_changes_background_add_int(&vicii.raster, x_pos,
                                          &vicii.raster.xsmooth_color,
                                          value);
    }

    raster_changes_background_add_int(&vicii.raster, x_pos,
                                      (int *)&vicii.raster.background_color,
                                      value);
    vicii.regs[0x21] = value;
}

inline static void ext_background_store(WORD addr, BYTE value)
{
    int char_num;

    value = (vicii.regs[addr] & 0xf0) | (value & 0x0f);

    VICII_DEBUG_REGISTER(("Background color #%d register: $%02X",
                          addr - 0x21, value));

    if (vicii.regs[addr] == value) {
        return;
    }

    vicii.regs[addr] = value;

    char_num = VICII_RASTER_CHAR(VICII_RASTER_CYCLE(maincpu_clk));

    if (vicii.video_mode == VICII_EXTENDED_TEXT_MODE) {
        raster_changes_background_add_int(&vicii.raster,
                                          VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk)),
                                          &vicii.raster.xsmooth_color,
                                          vicii.regs[0x21 + (vicii.background_color_source >> 6)]);
    }

    raster_changes_foreground_add_int(&vicii.raster,
                                      char_num - 1,
                                      &vicii.ext_background_color[addr - 0x22],
                                      value);
}

inline static void d025_store(BYTE value)
{
    raster_sprite_status_t *sprite_status;

    value &= 0xf;

    VICII_DEBUG_REGISTER(("Sprite multicolor register #0: $%02X", value));

    if (vicii.regs[0x25] == value) {
        return;
    }

    sprite_status = vicii.raster.sprite_status;

    raster_changes_sprites_add_int(&vicii.raster,
                                   VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk)) + 1,
                                   (int *)&sprite_status->mc_sprite_color_1,
                                   (int)value);

    vicii.regs[0x25] = value;
}

inline static void d026_store(BYTE value)
{
    raster_sprite_status_t *sprite_status;

    value &= 0xf;

    VICII_DEBUG_REGISTER(("Sprite multicolor register #1: $%02X", value));

    if (vicii.regs[0x26] == value) {
        return;
    }

    sprite_status = vicii.raster.sprite_status;

    raster_changes_sprites_add_int(&vicii.raster,
                                   VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk)) + 1,
                                   (int*)&sprite_status->mc_sprite_color_2,
                                   (int)value);

    vicii.regs[0x26] = value;
}

inline static void sprite_color_store(WORD addr, BYTE value)
{
    raster_sprite_t *sprite;
    int n;

    value &= 0xf;

    VICII_DEBUG_REGISTER(("Sprite #%d color register: $%02X",
                          addr - 0x27, value));

    if (vicii.regs[addr] == value) {
        return;
    }

    n = addr - 0x27;

    sprite = vicii.raster.sprite_status->sprites + n;

    raster_changes_sprites_add_int(&vicii.raster,
                                   VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk)) + 1,
                                   (int *)&sprite->color,
                                   value);

    vicii.regs[addr] = value;
}

void viciidtv_update_colorram()
{
    vicii.color_ram_ptr = mem_ram
                          + (vicii.regs[0x36] << 10)
                          + ((vicii.regs[0x37] & 0x07) << 18); /* TODO test */
}

/* Store a value in a VIC-II register.  */
void vicii_store(WORD addr, BYTE value)
{
    addr &= 0x3f;

    vicii_handle_pending_alarms_external_write();

    /* This is necessary as we must be sure that the previous line has been
       updated and `current_line' is actually set to the current Y position of
       the raster.  Otherwise we might mix the changes for this line with the
       changes for the previous one.  */
    if (maincpu_clk >= vicii.draw_clk) {
        vicii_raster_draw_alarm_handler(maincpu_clk - vicii.draw_clk, NULL);
    }

    VICII_DEBUG_REGISTER(("WRITE $D0%02X at cycle %d of current_line $%04X",
                          addr, VICII_RASTER_CYCLE(maincpu_clk),
                          VICII_RASTER_Y(maincpu_clk)));

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
        case 0x37:                /* $D036: Unused */
        case 0x38:                /* $D036: Unused */
        case 0x39:                /* $D036: Unused */
        case 0x3a:                /* $D036: Unused */
        case 0x3b:                /* $D036: Unused */
        case 0x3c:                /* $D036: Unused */
        case 0x3d:                /* $D036: Unused */
        case 0x3e:                /* $D036: Unused */
        case 0x3f:                /* $D03E: Unused */
            VICII_DEBUG_REGISTER(("(unused)"));
            break;
    }
}


/* Helper function for reading from $D011/$D012.  */
inline static unsigned int read_raster_y(void)
{
    int raster_y;

    raster_y = VICII_RASTER_Y(maincpu_clk);

    /* Line 0 is 62 cycles long, while line (SCREEN_HEIGHT - 1) is 64
       cycles long.  As a result, the counter is incremented one
       cycle later on line 0.  */
    if (raster_y == 0 && VICII_RASTER_CYCLE(maincpu_clk) == 0) {
        raster_y = vicii.screen_height - 1;
    }

    return raster_y;
}

inline static BYTE d01112_read(WORD addr)
{
    unsigned int tmp = read_raster_y();

    VICII_DEBUG_REGISTER(("Raster Line register %svalue = $%04X",
                          (addr == 0x11 ? "(highest bit) " : ""), tmp));
    if (addr == 0x11) {
        vicii.last_read = (vicii.regs[addr] & 0x7f) | ((tmp & 0x100) >> 1);
    } else {
        vicii.last_read = tmp & 0xff;
    }

    return vicii.last_read;
}


inline static BYTE d019_read(void)
{
    /* Manually set raster IRQ flag if the opcode reading $d019 has crossed
       the line end and the raster IRQ alarm has not been executed yet. */
    if (VICII_RASTER_Y(maincpu_clk) == vicii.raster_irq_line
        && vicii.raster_irq_clk != CLOCK_MAX
        && maincpu_clk >= vicii.raster_irq_clk) {
        if (vicii.regs[0x1a] & 0x1) {
            vicii.last_read = vicii.irq_status | 0xf1;
        } else {
            vicii.last_read = vicii.irq_status | 0x71;
        }
    } else {
        vicii.last_read = vicii.irq_status | 0x70;
    }

    return vicii.last_read;
}

inline static BYTE d01e_read(void)
{
    /* Remove the pending sprite-sprite interrupt, as the collision
       register is reset upon read accesses.  */
    vicii_irq_sscoll_clear();

    vicii.regs[0x1e] = vicii.sprite_sprite_collisions;
    vicii.sprite_sprite_collisions = 0;
    VICII_DEBUG_REGISTER(("Sprite-sprite collision mask: $%02X",
                          vicii.regs[0x1e]));

    return vicii.regs[0x1e];
}

inline static BYTE d01f_read(void)
{
    /* Remove the pending sprite-background interrupt, as the collision
       register is reset upon read accesses.  */
    vicii_irq_sbcoll_clear();

    vicii.regs[0x1f] = vicii.sprite_background_collisions;
    vicii.sprite_background_collisions = 0;
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
    addr &= 0x3f;

    /* Serve all pending events.  */
    vicii_handle_pending_alarms(0);

    VICII_DEBUG_REGISTER(("READ $D0%02X at cycle %d of current_line $%04X:",
                          addr, VICII_RASTER_CYCLE(maincpu_clk),
                          VICII_RASTER_Y(maincpu_clk)));

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
            return vicii.regs[addr];

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
            return vicii.regs[addr];

        case 0x10:                /* $D010: Sprite X position MSB */
            VICII_DEBUG_REGISTER(("Sprite X position MSB: $%02X",
                                  vicii.regs[addr]));
            return vicii.regs[addr];

        case 0x11:              /* $D011: video mode, Y scroll, 24/25 line mode
                                   and raster MSB */
        case 0x12:              /* $D012: Raster line compare */
            return d01112_read(addr);

        case 0x13:                /* $D013: Light Pen X */
            VICII_DEBUG_REGISTER(("Light pen X: %d", vicii.light_pen.x));
            return vicii.light_pen.x;

        case 0x14:                /* $D014: Light Pen Y */
            VICII_DEBUG_REGISTER(("Light pen Y: %d", vicii.light_pen.y));
            return vicii.light_pen.y;

        case 0x15:                /* $D015: Sprite Enable */
            VICII_DEBUG_REGISTER(("Sprite Enable register: $%02X",
                                  vicii.regs[addr]));
            return vicii.regs[addr];

        case 0x16:                /* $D016 */
            VICII_DEBUG_REGISTER(("$D016 Control register read: $%02X",
                                  vicii.regs[addr]));
            return vicii.regs[addr] | 0xc0;

        case 0x17:                /* $D017: Sprite Y-expand */
            VICII_DEBUG_REGISTER(("Sprite Y Expand register: $%02X",
                                  vicii.regs[addr]));
            return vicii.regs[addr];

        case 0x18:              /* $D018: Video and char matrix base address */
            VICII_DEBUG_REGISTER(("Video memory address register: $%02X",
                                  vicii.regs[addr]));
            return vicii.regs[addr] | 0x1;

        case 0x19:                /* $D019: IRQ flag register */
            {
                BYTE tmp;

                tmp = d019_read();
                VICII_DEBUG_REGISTER(("Interrupt register: $%02X", tmp));

                return tmp;
            }

        case 0x1a:                /* $D01A: IRQ mask register  */
            VICII_DEBUG_REGISTER(("Mask register: $%02X",
                                  vicii.regs[addr] | 0xf0));
            return vicii.regs[addr] | 0xf0;

        case 0x1b:                /* $D01B: Sprite priority */
            VICII_DEBUG_REGISTER(("Sprite Priority register: $%02X",
                                  vicii.regs[addr]));
            return vicii.regs[addr];

        case 0x1c:                /* $D01C: Sprite Multicolor select */
            VICII_DEBUG_REGISTER(("Sprite Multicolor Enable register: $%02X",
                                  vicii.regs[addr]));
            return vicii.regs[addr];

        case 0x1d:                /* $D01D: Sprite X-expand */
            VICII_DEBUG_REGISTER(("Sprite X Expand register: $%02X",
                                  vicii.regs[addr]));
            return vicii.regs[addr];

        case 0x1e:                /* $D01E: Sprite-sprite collision */
            return d01e_read();

        case 0x1f:                /* $D01F: Sprite-background collision */
            return d01f_read();

        case 0x20:                /* $D020: Border color */
            VICII_DEBUG_REGISTER(("Border Color register: $%02X",
                                  vicii.regs[addr]));
            return vicii.regs[addr] | 0xf0;

        case 0x21:                /* $D021: Background #0 color */
        case 0x22:                /* $D022: Background #1 color */
        case 0x23:                /* $D023: Background #2 color */
        case 0x24:                /* $D024: Background #3 color */
            VICII_DEBUG_REGISTER(("Background Color #%d register: $%02X",
                                  addr - 0x21, vicii.regs[addr]));
            return vicii.regs[addr] | 0xf0;

        case 0x25:                /* $D025: Sprite multicolor register #0 */
        case 0x26:                /* $D026: Sprite multicolor register #1 */
            VICII_DEBUG_REGISTER(("Multicolor register #%d: $%02X",
                                  addr - 0x22, vicii.regs[addr]));
            return vicii.regs[addr] | 0xf0;

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
            return vicii.regs[addr] | 0xf0;

        case 0x2f:                /* $D02F: Unused */
        case 0x30:                /* $D030: Unused */
        case 0x31:                /* $D031: Unused */
        case 0x32:                /* $D032: Unused */
        case 0x33:                /* $D033: Unused */
        case 0x34:                /* $D034: Unused */
        case 0x35:                /* $D035: Unused */
        case 0x36:                /* $D036: Unused */
        case 0x37:                /* $D036: Unused */
        case 0x38:                /* $D036: Unused */
        case 0x39:                /* $D036: Unused */
        case 0x3a:                /* $D036: Unused */
        case 0x3b:                /* $D036: Unused */
        case 0x3c:                /* $D036: Unused */
        case 0x3d:                /* $D036: Unused */
        case 0x3e:                /* $D036: Unused */
        case 0x3f:                /* $D036: Unused */
            return 0xff;

        default:
            return 0xff;
    }
    return 0xff;  /* make compiler happy */
}

inline static BYTE d019_peek(void)
{
    /* Manually set raster IRQ flag if the opcode reading $d019 has crossed
       the line end and the raster IRQ alarm has not been executed yet. */
    if (VICII_RASTER_Y(maincpu_clk) == vicii.raster_irq_line
        && vicii.raster_irq_clk != CLOCK_MAX
        && maincpu_clk >= vicii.raster_irq_clk) {
        if (vicii.regs[0x1a] & 0x1) {
            return vicii.irq_status | 0xf1;
        } else {
            return vicii.irq_status | 0x71;
        }
    } else {
        return vicii.irq_status | 0x70;
    }

    return vicii.irq_status;
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
        case 0x2f:            /* Extended keyboard row select */
            return /* vicii.regs[addr] | */ 0xff;
        default:
            return vicii.regs[addr] | unused_bits_in_registers[addr];
    }
}
