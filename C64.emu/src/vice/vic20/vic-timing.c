/*
 * vic-timing.c - Timing related settings for the VIC emulation.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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
#include "resources.h"
#include "vic-timing.h"
#include "vic.h"
#include "victypes.h"
#include "vic20.h"

void vic_timing_set(machine_timing_t *machine_timing, int border_mode)
{
    int mode;

    resources_get_int("MachineVideoStandard", &mode);

    switch (mode) {
        case MACHINE_SYNC_NTSC:
            vic.screen_height = VIC20_NTSC_SCREEN_LINES;
            switch (border_mode) {
                default:
                case VIC_NORMAL_BORDERS:
                    vic.first_displayed_line = VIC_NTSC_NORMAL_FIRST_DISPLAYED_LINE;
                    vic.last_displayed_line = VIC_NTSC_NORMAL_LAST_DISPLAYED_LINE;
                    vic.display_width = VIC_NTSC_NORMAL_DISPLAY_WIDTH;
                    vic.screen_leftborderwidth = VIC_NTSC_NORMAL_LEFTBORDERWIDTH;
                    break;
                case VIC_FULL_BORDERS:
                    vic.first_displayed_line = VIC_NTSC_FULL_FIRST_DISPLAYED_LINE;
                    vic.last_displayed_line = VIC_NTSC_FULL_LAST_DISPLAYED_LINE;
                    vic.display_width = VIC_NTSC_FULL_DISPLAY_WIDTH;
                    vic.screen_leftborderwidth = VIC_NTSC_FULL_LEFTBORDERWIDTH;
                    break;
                case VIC_DEBUG_BORDERS:
                    vic.first_displayed_line = VIC_NTSC_DEBUG_FIRST_DISPLAYED_LINE;
                    vic.last_displayed_line = VIC_NTSC_DEBUG_LAST_DISPLAYED_LINE;
                    vic.display_width = VIC_NTSC_DEBUG_DISPLAY_WIDTH;
                    vic.screen_leftborderwidth = VIC_NTSC_DEBUG_LEFTBORDERWIDTH;
                    break;
                case VIC_NO_BORDERS:
                    vic.first_displayed_line = VIC_NTSC_NO_BORDER_FIRST_DISPLAYED_LINE;
                    vic.last_displayed_line = VIC_NTSC_NO_BORDER_LAST_DISPLAYED_LINE;
                    vic.display_width = VIC_NTSC_NO_BORDER_DISPLAY_WIDTH;
                    vic.screen_leftborderwidth = VIC_NTSC_NO_BORDER_LEFTBORDERWIDTH;
                    break;
            }
            vic.screen_width = VIC_NTSC_SCREEN_WIDTH;
            vic.cycles_per_line = VIC20_NTSC_CYCLES_PER_LINE;
            vic.cycle_offset = VIC20_NTSC_CYCLE_OFFSET;
            vic.max_text_cols = VIC_NTSC_MAX_TEXT_COLS;
            break;
        case MACHINE_SYNC_PAL:
        default:
            vic.screen_height = VIC20_PAL_SCREEN_LINES;
            switch (border_mode) {
                default:
                case VIC_NORMAL_BORDERS:
                    vic.first_displayed_line = VIC_PAL_NORMAL_FIRST_DISPLAYED_LINE;
                    vic.last_displayed_line = VIC_PAL_NORMAL_LAST_DISPLAYED_LINE;
                    vic.display_width = VIC_PAL_NORMAL_DISPLAY_WIDTH;
                    vic.screen_leftborderwidth = VIC_PAL_NORMAL_LEFTBORDERWIDTH;
                    break;
                case VIC_FULL_BORDERS:
                    vic.first_displayed_line = VIC_PAL_FULL_FIRST_DISPLAYED_LINE;
                    vic.last_displayed_line = VIC_PAL_FULL_LAST_DISPLAYED_LINE;
                    vic.display_width = VIC_PAL_FULL_DISPLAY_WIDTH;
                    vic.screen_leftborderwidth = VIC_PAL_FULL_LEFTBORDERWIDTH;
                    break;
                case VIC_DEBUG_BORDERS:
                    vic.first_displayed_line = VIC_PAL_DEBUG_FIRST_DISPLAYED_LINE;
                    vic.last_displayed_line = VIC_PAL_DEBUG_LAST_DISPLAYED_LINE;
                    vic.display_width = VIC_PAL_DEBUG_DISPLAY_WIDTH;
                    vic.screen_leftborderwidth = VIC_PAL_DEBUG_LEFTBORDERWIDTH;
                    break;
                case VIC_NO_BORDERS:
                    vic.first_displayed_line = VIC_PAL_NO_BORDER_FIRST_DISPLAYED_LINE;
                    vic.last_displayed_line = VIC_PAL_NO_BORDER_LAST_DISPLAYED_LINE;
                    vic.display_width = VIC_PAL_NO_BORDER_DISPLAY_WIDTH;
                    vic.screen_leftborderwidth = VIC_PAL_NO_BORDER_LEFTBORDERWIDTH;
                    break;
            }
            vic.screen_width = VIC_PAL_SCREEN_WIDTH;
            vic.cycles_per_line = VIC20_PAL_CYCLES_PER_LINE;
            vic.cycle_offset = VIC20_PAL_CYCLE_OFFSET;
            vic.max_text_cols = VIC_PAL_MAX_TEXT_COLS;
            break;
    }
}
