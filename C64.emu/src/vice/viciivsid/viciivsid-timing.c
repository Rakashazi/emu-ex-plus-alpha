/*
 * viciivsid-timing.c - Timing related settings for the MOS 6569 (VIC-II) emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Gunnar Ruthenberg <Krill.Plush@gmail.com>
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

#include "archdep.h"
#include "c64.h"
#include "machine.h"
#include "resources.h"
#include "vicii-sprites.h"
#include "vicii-timing.h"
#include "vicii.h"
#include "viciitypes.h"

/* Number of cycles per line.  */
#define VICII_PAL_CYCLES_PER_LINE      C64_PAL_CYCLES_PER_LINE
#define VICII_NTSC_CYCLES_PER_LINE     C64_NTSC_CYCLES_PER_LINE
#define VICII_NTSCOLD_CYCLES_PER_LINE  C64_NTSCOLD_CYCLES_PER_LINE
#define VICII_PALN_CYCLES_PER_LINE     C64_PALN_CYCLES_PER_LINE

/* Cycle # at which sprite DMA is set.  */
#define VICII_PAL_SPRITE_FETCH_CYCLE       54
#define VICII_NTSC_SPRITE_FETCH_CYCLE      55
#define VICII_NTSCOLD_SPRITE_FETCH_CYCLE   55
#define VICII_PALN_SPRITE_FETCH_CYCLE      55

#define VICII_PAL_SPRITE_WRAP_X     504
#define VICII_NTSC_SPRITE_WRAP_X    520
#define VICII_NTSCOLD_SPRITE_WRAP_X 512
#define VICII_PALN_SPRITE_WRAP_X    520

/* Cycle # at which the current raster line is re-drawn.  It is set to
   `VICII_CYCLES_PER_LINE', so this actually happens at the very beginning
   (i.e. cycle 0) of the next line.  */
#define VICII_PAL_DRAW_CYCLE       VICII_PAL_CYCLES_PER_LINE
#define VICII_NTSC_DRAW_CYCLE      VICII_NTSC_CYCLES_PER_LINE
#define VICII_NTSCOLD_DRAW_CYCLE   VICII_NTSCOLD_CYCLES_PER_LINE
#define VICII_PALN_DRAW_CYCLE      VICII_PALN_CYCLES_PER_LINE

/* Common parameters for all video standards */
#define VICII_25ROW_START_LINE    0x33
#define VICII_25ROW_STOP_LINE     0xfb
#define VICII_24ROW_START_LINE    0x37
#define VICII_24ROW_STOP_LINE     0xf7

/* Bad line range.  */
#define VICII_FIRST_DMA_LINE      0x30
#define VICII_LAST_DMA_LINE       0xf7


void vicii_timing_set(machine_timing_t *machine_timing)
{
    int mode;

    resources_get_int("MachineVideoStandard", &mode);

    switch (mode) {
        case MACHINE_SYNC_NTSC:
            vicii.screen_height = VICII_NTSC_SCREEN_HEIGHT;
            vicii.screen_leftborderwidth = VICII_SCREEN_NTSC_NORMAL_LEFTBORDERWIDTH;
            vicii.screen_rightborderwidth = VICII_SCREEN_NTSC_NORMAL_RIGHTBORDERWIDTH;
            vicii.first_displayed_line = VICII_NTSC_NORMAL_FIRST_DISPLAYED_LINE;
            vicii.last_displayed_line = VICII_NTSC_NORMAL_LAST_DISPLAYED_LINE;
            vicii.cycles_per_line = VICII_NTSC_CYCLES_PER_LINE;
            vicii.draw_cycle = VICII_NTSC_DRAW_CYCLE;
            vicii.sprite_fetch_cycle = VICII_NTSC_SPRITE_FETCH_CYCLE;
            vicii.sprite_wrap_x = VICII_NTSC_SPRITE_WRAP_X;
            break;
        case MACHINE_SYNC_NTSCOLD:
            vicii.screen_height = VICII_NTSCOLD_SCREEN_HEIGHT;
            vicii.screen_leftborderwidth = VICII_SCREEN_NTSCOLD_NORMAL_LEFTBORDERWIDTH;
            vicii.screen_rightborderwidth = VICII_SCREEN_NTSCOLD_NORMAL_RIGHTBORDERWIDTH;
            vicii.first_displayed_line = VICII_NTSCOLD_NORMAL_FIRST_DISPLAYED_LINE;
            vicii.last_displayed_line = VICII_NTSCOLD_NORMAL_LAST_DISPLAYED_LINE;
            vicii.cycles_per_line = VICII_NTSCOLD_CYCLES_PER_LINE;
            vicii.draw_cycle = VICII_NTSCOLD_DRAW_CYCLE;
            vicii.sprite_fetch_cycle = VICII_NTSCOLD_SPRITE_FETCH_CYCLE;
            vicii.sprite_wrap_x = VICII_NTSCOLD_SPRITE_WRAP_X;
            break;
        case MACHINE_SYNC_PALN:
            vicii.screen_height = VICII_PALN_SCREEN_HEIGHT;
            vicii.screen_leftborderwidth = VICII_SCREEN_PALN_NORMAL_LEFTBORDERWIDTH;
            vicii.screen_rightborderwidth = VICII_SCREEN_PALN_NORMAL_RIGHTBORDERWIDTH;
            vicii.first_displayed_line = VICII_PALN_NORMAL_FIRST_DISPLAYED_LINE;
            vicii.last_displayed_line = VICII_PALN_NORMAL_LAST_DISPLAYED_LINE;
            vicii.cycles_per_line = VICII_PALN_CYCLES_PER_LINE;
            vicii.draw_cycle = VICII_PALN_DRAW_CYCLE;
            vicii.sprite_fetch_cycle = VICII_PALN_SPRITE_FETCH_CYCLE;
            vicii.sprite_wrap_x = VICII_PALN_SPRITE_WRAP_X;
            break;
        case MACHINE_SYNC_PAL:
        default:
            vicii.screen_height = VICII_PAL_SCREEN_HEIGHT;
            vicii.screen_leftborderwidth = VICII_SCREEN_PAL_NORMAL_LEFTBORDERWIDTH;
            vicii.screen_rightborderwidth = VICII_SCREEN_PAL_NORMAL_RIGHTBORDERWIDTH;
            vicii.first_displayed_line = VICII_PAL_NORMAL_FIRST_DISPLAYED_LINE;
            vicii.last_displayed_line = VICII_PAL_NORMAL_LAST_DISPLAYED_LINE;
            vicii.cycles_per_line = VICII_PAL_CYCLES_PER_LINE;
            vicii.draw_cycle = VICII_PAL_DRAW_CYCLE;
            vicii.sprite_fetch_cycle = VICII_PAL_SPRITE_FETCH_CYCLE;
            vicii.sprite_wrap_x = VICII_PAL_SPRITE_WRAP_X;
            break;
    }

    vicii.first_dma_line = VICII_FIRST_DMA_LINE;
    vicii.last_dma_line = VICII_LAST_DMA_LINE;
    vicii.row_25_start_line = VICII_25ROW_START_LINE;
    vicii.row_25_stop_line = VICII_25ROW_STOP_LINE;
    vicii.row_24_start_line = VICII_24ROW_START_LINE;
    vicii.row_24_stop_line = VICII_24ROW_STOP_LINE;

    vicii.raster.display_xstart = VICII_40COL_START_PIXEL;
    vicii.raster.display_xstop = VICII_40COL_STOP_PIXEL;

    vicii_sprites_init_sprline();
}
