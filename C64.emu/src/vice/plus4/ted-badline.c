/*
 * ted-badline.c - Bad line handling for the TED emulation.
 *
 * Written by
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

#include <string.h>

#include "dma.h"
#include "maincpu.h"
#include "mem.h"
#include "ted-badline.h"
#include "ted-fetch.h"
#include "tedtypes.h"
#include "types.h"


inline static void line_becomes_good(int cycle)
{
    /* Bad line becomes good.  */
    ted.bad_line = 0;

    /* By changing the values in the registers, one can make the TED
       switch from idle to display state, but not from display to
       idle state.  So we are always in display state if this
       happens.  This is only true if the value changes in some
       cycle > 0, though; otherwise, the line never becomes bad.  */
    if (cycle > 0) {
        ted.raster.draw_idle_state = ted.idle_state = 0;
        ted.idle_data_location = IDLE_NONE;
        if (cycle > TED_FETCH_CYCLE + 2
            && !ted.ycounter_reset_checked) {
            /*ted.raster.ycounter = 0;*/
            ted.ycounter_reset_checked = 1;
        }
    }
}

inline static void line_becomes_bad(int cycle)
{
    if (cycle >= TED_FETCH_CYCLE
        && cycle <= TED_FETCH_CYCLE + TED_SCREEN_TEXTCOLS + 3) {
        int pos;            /* Value of line counter when this happens.  */
        int inc;            /* Total increment for the line counter.  */
        int num_chars;      /* Total number of characters to fetch.  */
        int num_0xff_fetches; /* Number of 0xff fetches to do.  */

        ted.bad_line = 1;

        /*if (cycle <= TED_FETCH_CYCLE + 2)
            ted.raster.ycounter = 0;*/

        ted.ycounter_reset_checked = 1;

        num_chars = (TED_SCREEN_TEXTCOLS - (cycle - (TED_FETCH_CYCLE + 3)));

        /* Take over the bus until the memory fetch is done.  */
        dma_maincpu_steal_cycles(maincpu_clk, num_chars, 0);
        ted_delay_oldclk(num_chars);

        if (num_chars <= TED_SCREEN_TEXTCOLS) {
            /* Matrix fetches starts immediately, but the TED needs
               at least 3 cycles to become the bus master.  Before
               this happens, it fetches 0xff.  */
            num_0xff_fetches = 3;

            /* If we were in idle state before creating the bad
               line, the counters have not been incremented.  */
            if (ted.idle_state) {
                pos = 0;
                inc = num_chars;
                if (inc < 0) {
                    inc = 0;
                }
            } else {
                pos = cycle - (TED_FETCH_CYCLE + 3);
                if (pos > TED_SCREEN_TEXTCOLS - 1) {
                    pos = TED_SCREEN_TEXTCOLS - 1;
                }
                inc = TED_SCREEN_TEXTCOLS;
            }
        } else {
            pos = 0;
            num_chars = inc = TED_SCREEN_TEXTCOLS;
            num_0xff_fetches = cycle - TED_FETCH_CYCLE;
        }
        /* This is normally done at cycle `TED_FETCH_CYCLE + 2'.  */
        /*ted.mem_counter = ted.memptr;*/
        /*ted.memptr_col = ted.mem_counter;*/

        /* Force the DMA.  */
        /* Note that `ted.cbuf' is loaded from the value of
           the next opcode as the VIC-II is not the bus master yet.  */
        if (num_chars <= num_0xff_fetches) {
            /*memset(ted.vbuf + pos, 0xff, num_chars);*/
            memset(ted.cbuf_tmp + pos, mem_ram[reg_pc] & 0x7f,
                   num_chars);
        } else {
            /*memset(ted.vbuf + pos, 0xff, num_0xff_fetches);*/
            memcpy(ted.cbuf_tmp, ted.cbuf, pos);
            memset(ted.cbuf_tmp + pos, mem_ram[reg_pc] & 0x7f,
                   num_0xff_fetches);
            /*ted_fetch_matrix(pos + num_0xff_fetches,
                             num_chars - num_0xff_fetches);*/
            ted_fetch_color(pos + num_0xff_fetches,
                            num_chars - num_0xff_fetches);
        }

        /* Set the value by which `ted.mem_counter' is incremented on
           this line.  */
        ted.mem_counter_inc = TED_SCREEN_TEXTCOLS; /*inc;*/

        /* Remember we have done a DMA.  */
        ted.memory_fetch_done = 2;

        /* As we are on a bad line, switch to display state.  */
        ted.idle_state = 0;

        /* Try to display things correctly.  This is not exact,
           but should be OK for most cases (FIXME?).  */
        if (inc == TED_SCREEN_TEXTCOLS) {
            ted.raster.draw_idle_state = 0;
            ted.idle_data_location = IDLE_NONE;
        }
    } else if (cycle <= TED_FETCH_CYCLE + TED_SCREEN_TEXTCOLS + 6) {
        /* Bad line has been generated after fetch interval, but
           before `ted.raster.ycounter' is incremented.  */

        ted.bad_line = 1;

        /* If in idle state, counter is not incremented.  */
        if (ted.idle_state) {
            ted.mem_counter_inc = 0;
        }

        /* We are not in idle state anymore.  */
        /* This is not 100% correct, but should be OK for most cases.
           (FIXME?)  */
        ted.raster.draw_idle_state = ted.idle_state = 0;
        ted.idle_data_location = IDLE_NONE;
    } else {
        /* Line is now bad, so we must switch to display state.
           Anyway, we cannot do it here as the `ycounter' handling
           must happen in as in idle state.  */
        ted.force_display_state = 1;
    }
}

void ted_badline_check_state(BYTE value, const int cycle,
                             const unsigned int line)
{
    int was_bad_line, now_bad_line;

    /* Check whether bad line state has changed.  */
    /*was_bad_line = (ted.allow_bad_lines
                    && (ted.raster.ysmooth == (int)(line & 7)));*/
    was_bad_line = ted.bad_line;
    now_bad_line = (ted.allow_bad_lines
                    && (((int)(value & 7) == (int)(line & 7))
                        /*|| (ted.bad_line &&
                         ((int)((value + 1) & 7) == (int)(line & 7) ))*/))
    ;

    if (was_bad_line && !now_bad_line) {
        line_becomes_good(cycle);
    } else {
        /*if (!was_bad_line && now_bad_line)
                        ted.raster_irq_clk++;*/
        /*line_becomes_bad(cycle);*/
    }
}
