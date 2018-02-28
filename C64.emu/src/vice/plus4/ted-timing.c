/*
 * ted-timing.c - Timing related settings for the TED emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Tibor Biczo <crown@axelero.hu>
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

#include "machine.h"
#include "plus4.h"
#include "resources.h"
#include "ted-timing.h"
#include "ted.h"
#include "tedtypes.h"

/* Number of cycles per line.  */
#define TED_PAL_CYCLES_PER_LINE     PLUS4_PAL_CYCLES_PER_LINE
#define TED_NTSC_CYCLES_PER_LINE    PLUS4_NTSC_CYCLES_PER_LINE

/* Cycle # at which the current raster line is re-drawn.  It is set to
   `TED_CYCLES_PER_LINE', so this actually happens at the very beginning
   (i.e. cycle 0) of the next line.  */
#define TED_PAL_DRAW_CYCLE          TED_PAL_CYCLES_PER_LINE
#define TED_NTSC_DRAW_CYCLE         TED_NTSC_CYCLES_PER_LINE

void ted_timing_set(machine_timing_t *machine_timing, int border_mode)
{
    int mode;

    resources_get_int("MachineVideoStandard", &mode);

    switch (mode) {
        case MACHINE_SYNC_NTSC:
            ted.screen_height = TED_NTSC_SCREEN_HEIGHT;
            switch (border_mode) {
                default:
                case TED_NORMAL_BORDERS:
                    ted.first_displayed_line = TED_NTSC_NORMAL_FIRST_DISPLAYED_LINE;
                    ted.last_displayed_line = TED_NTSC_NORMAL_LAST_DISPLAYED_LINE;
                    ted.screen_rightborderwidth = TED_SCREEN_NTSC_NORMAL_LEFTBORDERWIDTH;
                    ted.screen_leftborderwidth = TED_SCREEN_NTSC_NORMAL_RIGHTBORDERWIDTH;
                    break;
                case TED_FULL_BORDERS:
                    ted.first_displayed_line = TED_NTSC_FULL_FIRST_DISPLAYED_LINE;
                    ted.last_displayed_line = TED_NTSC_FULL_LAST_DISPLAYED_LINE;
                    ted.screen_rightborderwidth = TED_SCREEN_NTSC_FULL_LEFTBORDERWIDTH;
                    ted.screen_leftborderwidth = TED_SCREEN_NTSC_FULL_RIGHTBORDERWIDTH;
                    break;
                case TED_DEBUG_BORDERS:
                    ted.first_displayed_line = TED_NTSC_DEBUG_FIRST_DISPLAYED_LINE;
                    ted.last_displayed_line = TED_NTSC_DEBUG_LAST_DISPLAYED_LINE;
                    ted.screen_rightborderwidth = TED_SCREEN_NTSC_DEBUG_LEFTBORDERWIDTH;
                    ted.screen_leftborderwidth = TED_SCREEN_NTSC_DEBUG_RIGHTBORDERWIDTH;
                    break;
                case TED_NO_BORDERS:
                    ted.first_displayed_line = TED_NTSC_NO_BORDER_FIRST_DISPLAYED_LINE;
                    ted.last_displayed_line = TED_NTSC_NO_BORDER_LAST_DISPLAYED_LINE;
                    ted.screen_rightborderwidth = 0;
                    ted.screen_leftborderwidth = 0;
                    break;
            }
            ted.row_25_start_line = TED_NTSC_25ROW_START_LINE;
            ted.row_25_stop_line = TED_NTSC_25ROW_STOP_LINE;
            ted.row_24_start_line = TED_NTSC_24ROW_START_LINE;
            ted.row_24_stop_line = TED_NTSC_24ROW_STOP_LINE;
            ted.cycles_per_line = TED_NTSC_CYCLES_PER_LINE;
            ted.draw_cycle = TED_NTSC_DRAW_CYCLE;
            ted.first_dma_line = TED_NTSC_FIRST_DMA_LINE;
            ted.last_dma_line = TED_NTSC_LAST_DMA_LINE;
            ted.offset = TED_NTSC_OFFSET;
            ted.vsync_line = TED_NTSC_VSYNC_LINE;
            break;
        case MACHINE_SYNC_PAL:
        default:
            ted.screen_height = TED_PAL_SCREEN_HEIGHT;
            switch (border_mode) {
                default:
                case TED_NORMAL_BORDERS:
                    ted.first_displayed_line = TED_PAL_NORMAL_FIRST_DISPLAYED_LINE;
                    ted.last_displayed_line = TED_PAL_NORMAL_LAST_DISPLAYED_LINE;
                    ted.screen_rightborderwidth = TED_SCREEN_PAL_NORMAL_LEFTBORDERWIDTH;
                    ted.screen_leftborderwidth = TED_SCREEN_PAL_NORMAL_RIGHTBORDERWIDTH;
                    break;
                case TED_FULL_BORDERS:
                    ted.first_displayed_line = TED_PAL_FULL_FIRST_DISPLAYED_LINE;
                    ted.last_displayed_line = TED_PAL_FULL_LAST_DISPLAYED_LINE;
                    ted.screen_rightborderwidth = TED_SCREEN_PAL_FULL_LEFTBORDERWIDTH;
                    ted.screen_leftborderwidth = TED_SCREEN_PAL_FULL_RIGHTBORDERWIDTH;
                    break;
                case TED_DEBUG_BORDERS:
                    ted.first_displayed_line = TED_PAL_DEBUG_FIRST_DISPLAYED_LINE;
                    ted.last_displayed_line = TED_PAL_DEBUG_LAST_DISPLAYED_LINE;
                    ted.screen_rightborderwidth = TED_SCREEN_PAL_DEBUG_LEFTBORDERWIDTH;
                    ted.screen_leftborderwidth = TED_SCREEN_PAL_DEBUG_RIGHTBORDERWIDTH;
                    break;
                case TED_NO_BORDERS:
                    ted.first_displayed_line = TED_PAL_NO_BORDER_FIRST_DISPLAYED_LINE;
                    ted.last_displayed_line = TED_PAL_NO_BORDER_LAST_DISPLAYED_LINE;
                    ted.screen_rightborderwidth = 0;
                    ted.screen_leftborderwidth = 0;
                    break;
            }
            ted.row_25_start_line = TED_PAL_25ROW_START_LINE;
            ted.row_25_stop_line = TED_PAL_25ROW_STOP_LINE;
            ted.row_24_start_line = TED_PAL_24ROW_START_LINE;
            ted.row_24_stop_line = TED_PAL_24ROW_STOP_LINE;
            ted.cycles_per_line = TED_PAL_CYCLES_PER_LINE;
            ted.draw_cycle = TED_PAL_DRAW_CYCLE;
            ted.first_dma_line = TED_PAL_FIRST_DMA_LINE;
            ted.last_dma_line = TED_PAL_LAST_DMA_LINE;
            ted.offset = TED_PAL_OFFSET;
            ted.vsync_line = TED_PAL_VSYNC_LINE;
            break;
    }
}
