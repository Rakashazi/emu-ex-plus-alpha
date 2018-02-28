/*
 * viciivsid-badline.c - Bad line handling for the MOS 6569 (VIC-II) emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "dma.h"
#include "maincpu.h"
#include "raster-changes.h"
#include "types.h"
#include "vicii-badline.h"
#include "vicii-fetch.h"
#include "viciitypes.h"


inline static void switch_to_display_state(const int cycle)
{
    raster_changes_foreground_add_int(&vicii.raster, VICII_RASTER_CHAR(cycle),
                                      &vicii.raster.draw_idle_state, 0);
    raster_changes_background_add_int(&vicii.raster, VICII_RASTER_X(cycle),
                                      &vicii.raster.draw_idle_state, 0);
    vicii.idle_state = 0;
    vicii.idle_data_location = IDLE_NONE;
}

inline static void line_becomes_good(const int cycle)
{
    if (cycle < VICII_FETCH_CYCLE) {
        vicii.bad_line = 0;
    }

    /* By changing the values in the registers, one can make the VIC-II
       switch from idle to display state, but not from display to idle
       state.  So we are always in display state if this happens. This
       is only true if the value changes in some cycle > 0, though;
       otherwise, the line never becomes bad.  */
    if (cycle > 0) {
        switch_to_display_state(cycle);

        if (cycle > VICII_FETCH_CYCLE + 2
            && !vicii.ycounter_reset_checked) {
            vicii.raster.ycounter = 0;
            vicii.ycounter_reset_checked = 1;
        }
    }
}

inline static void line_becomes_bad(const int cycle)
{
    if (cycle >= VICII_FETCH_CYCLE
        && cycle < VICII_FETCH_CYCLE + VICII_SCREEN_TEXTCOLS + 3) {
        int xpos;           /* Current X position */
        int pos;            /* Value of line counter when this happens.  */
        int inc;            /* Total increment for the line counter.  */
        int num_chars;      /* Total number of characters to fetch.  */
        int num_0xff_fetches; /* Number of 0xff fetches to do.  */

        vicii.bad_line = 1;

        if (cycle <= VICII_FETCH_CYCLE + 2) {
            vicii.raster.ycounter = 0;
        }

        xpos = cycle - (VICII_FETCH_CYCLE + 3);

        num_chars = VICII_SCREEN_TEXTCOLS - xpos;

        /* Take over the bus until the memory fetch is done.  */
        dma_maincpu_steal_cycles(maincpu_clk, num_chars, 0);

        if (num_chars <= VICII_SCREEN_TEXTCOLS) {
            /* Matrix fetches starts immediately, but the VICII needs
               at least 3 cycles to become the bus master.  Before
               this happens, it fetches 0xff.  */
            num_0xff_fetches = 3;

            /* If we were in idle state before creating the bad
               line, the counters have not been incremented.  */
            if (vicii.idle_state) {
                pos = 0;
                inc = num_chars;
                if (inc < 0) {
                    inc = 0;
                }
            } else {
                pos = xpos;
                if (pos > VICII_SCREEN_TEXTCOLS - 1) {
                    pos = VICII_SCREEN_TEXTCOLS - 1;
                }
                inc = VICII_SCREEN_TEXTCOLS;
            }
        } else {
            pos = 0;
            num_chars = inc = VICII_SCREEN_TEXTCOLS;
            num_0xff_fetches = cycle - VICII_FETCH_CYCLE;
        }

        /* This is normally done at cycle `VICII_FETCH_CYCLE + 2'.  */
        vicii.mem_counter = vicii.memptr;

        if (vicii.idle_state && xpos > 0) {
            vicii.buf_offset = xpos;
        }

        /* As we are on a bad line, switch to display state.
           Display state becomes visible after one cycle delay.
           The following equation must be true:
           cycle + 1 - 15 = cycle - (11 + 3)
           15: first display cycle, 11: first fetch cycle */
        switch_to_display_state(cycle + 1);

        /* Force the DMA.  */
        if (num_chars > 0) {
            vicii_fetch_matrix(pos, num_chars, num_0xff_fetches, cycle);
        }

        /* Set the value by which `vicii.mem_counter' is incremented on
           this line.  */
        vicii.mem_counter_inc = inc;

        /* Remember we have done a DMA.  */
        vicii.memory_fetch_done = 2;
    } else if (cycle <= VICII_FETCH_CYCLE + VICII_SCREEN_TEXTCOLS + 6) {
        /* Bad line has been generated after fetch interval, but
           before `vicii.raster.ycounter' is incremented.  */

        vicii.bad_line = 1;

        /* If in idle state, counter is not incremented.  */
        if (vicii.idle_state) {
            vicii.mem_counter_inc = 0;
        }

        /* As we are on a bad line, switch to display state.  */
        switch_to_display_state(cycle + 1);
    } else {
        /* Line is now bad, so we must switch to display state.
           Anyway, we cannot do it here as the `ycounter' handling
           must happen in as in idle state.  */
        vicii.force_display_state = 1;
    }
    vicii.ycounter_reset_checked = 1;
}

void vicii_badline_check_state(BYTE value, const int cycle,
                               const unsigned int line,
                               const int old_allow_bad_lines)
{
    int was_bad_line, now_bad_line;

    /* Check whether bad line state has changed.  */
    was_bad_line = (old_allow_bad_lines
                    && (vicii.raster.ysmooth == (int)(line & 7)));
    now_bad_line = (vicii.allow_bad_lines
                    && ((int)(value & 7) == (int)(line & 7)));

    if (was_bad_line && !now_bad_line) {
        line_becomes_good(cycle);
    } else {
        if (!was_bad_line && now_bad_line) {
            line_becomes_bad(cycle);
        }
    }
}
