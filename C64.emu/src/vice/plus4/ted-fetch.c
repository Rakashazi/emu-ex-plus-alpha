/*
 * ted-fetch.c - Phi2 data fetch for the TED emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include <string.h>

#include "alarm.h"
#include "dma.h"
#include "maincpu.h"
#include "ted-fetch.h"
#include "tedtypes.h"
#include "types.h"


/* Emulate a matrix line fetch, `num' bytes starting from `offs'.  This takes
   care of the 10-bit counter wraparound.  */
void ted_fetch_matrix(int offs, int num)
{
    BYTE *p;
    int start_char;
    int c;
    int dma_offset = (ted.ted_raster_counter & 7)
                     == (unsigned int)((ted.raster.ysmooth + 1) & 7) ? 0x0400 : 0;

    /* Matrix fetches are done during Phi2, the fabulous "bad lines" */
    p = ted.screen_ptr;

    start_char = (ted.memptr_col + offs + dma_offset) & 0x3ff;
    c = 0x3ff - start_char + 1;

    if (c >= num) {
        memcpy(ted.vbuf + offs, p + start_char, num);
    } else {
        memcpy(ted.vbuf + offs, p + start_char, c);
        memcpy(ted.vbuf + offs + c, p, num - c);
    }
    /*memcpy(ted.cbuf, ted.cbuf_tmp, TED_SCREEN_TEXTCOLS);*/

/*    log_debug("Fetch line  : %03x, %03x", ted.ted_raster_counter, start_char);*/
}

inline void ted_fetch_color(int offs, int num)
{
    int start_char;
    int c;

    start_char = (ted.memptr_col + offs) & 0x3ff;
    c = 0x3ff - start_char + 1;

    if (c >= num) {
        memcpy(ted.cbuf_tmp + offs, ted.color_ptr + start_char, num);
    } else {
        memcpy(ted.cbuf_tmp + offs, ted.color_ptr + start_char, c);
        memcpy(ted.cbuf_tmp + offs + c, ted.color_ptr, num - c);
    }
/*    log_debug("Color fetch : %03x, %03x", ted.ted_raster_counter, start_char);*/
}

/* If we are on a bad line, do the DMA.  Return nonzero if cycles have been
   stolen.  */
inline static int do_matrix_fetch(CLOCK sub)
{
    int reval = 0;

    if (!ted.memory_fetch_done) {
        raster_t *raster;

        raster = &ted.raster;

        ted.memory_fetch_done = 1;
        ted.mem_counter = ted.memptr_col;
        ted.chr_pos_count = ted.memptr;         /* FIXME this is not here */

        if ((ted.ted_raster_counter & 7)
            == (unsigned int)((raster->ysmooth + 1) & 7)
            && ted.allow_bad_lines
            && ted.ted_raster_counter > ted.first_dma_line
            /* && ted.bad_line */
            && ted.ted_raster_counter <= ted.last_dma_line) {
            ted_fetch_matrix(0, TED_SCREEN_TEXTCOLS);

            raster->draw_idle_state = 0;
            /* raster->ycounter = 0; */

            ted.idle_state = 0;
            ted.idle_data_location = IDLE_NONE;
            ted.ycounter_reset_checked = 1;
            ted.memory_fetch_done = 2;

            dma_maincpu_steal_cycles(ted.fetch_clk,
                                     (TED_SCREEN_TEXTCOLS + 3) * 2 - sub, 0);
            ted_delay_oldclk((TED_SCREEN_TEXTCOLS + 3) * 2 - sub);
            ted.bad_line = 1;
            reval = 1;
        }

        if ((ted.ted_raster_counter & 7) == (unsigned int)raster->ysmooth
            && ted.allow_bad_lines
            && ted.ted_raster_counter >= ted.first_dma_line
            && ted.ted_raster_counter < ted.last_dma_line) {
            ted_fetch_color(0, TED_SCREEN_TEXTCOLS);
/*
            raster->draw_idle_state = 0;
            raster->ycounter = 0;

            ted.idle_state = 0;
            ted.idle_data_location = IDLE_NONE;
            ted.ycounter_reset_checked = 1;
            ted.memory_fetch_done = 2;
*/
            dma_maincpu_steal_cycles(ted.fetch_clk,
                                     (TED_SCREEN_TEXTCOLS + 3) * 2 - sub, 0);
            ted_delay_oldclk((TED_SCREEN_TEXTCOLS + 3) * 2 - sub);

            ted.bad_line = 1;
            reval = 1;
        }
    }

    return reval;
}

inline static void handle_fetch_matrix(long offset, CLOCK sub,
                                       CLOCK *write_offset)
{
    *write_offset = 0;

    do_matrix_fetch(sub);

    if ((ted.ted_raster_counter >= ted.first_dma_line) &&
        (ted.ted_raster_counter < ted.last_dma_line)) {
        ted.fetch_clk += ted.cycles_per_line;
    } else {
        ted.fetch_clk += (ted.screen_height - ted.ted_raster_counter) * ted.cycles_per_line;
    }

    alarm_set(ted.raster_fetch_alarm, ted.fetch_clk);

    return;
}

/* Handle matrix fetch events.  FIXME: could be made slightly faster.  */
void ted_fetch_alarm_handler(CLOCK offset, void *data)
{
    CLOCK last_opcode_first_write_clk, last_opcode_last_write_clk;

    /* This kludgy thing is used to emulate the behavior of the 6510 when BA
       goes low.  When BA goes low, every read access stops the processor
       until BA is high again; write accesses happen as usual instead.  */

    if (offset > 0) {
        switch (OPINFO_NUMBER(last_opcode_info)) {
            case 0:
                /* In BRK, IRQ and NMI the 3rd, 4th and 5th cycles are write
                   accesses, while the 1st, 2nd, 6th and 7th are read accesses.  */
                last_opcode_first_write_clk = maincpu_clk - 10;
                last_opcode_last_write_clk = maincpu_clk - 6;
                break;

            case 0x20:
                /* In JSR, the 4th and 5th cycles are write accesses, while the
                   1st, 2nd, 3rd and 6th are read accesses.  */
                last_opcode_first_write_clk = maincpu_clk - 6;
                last_opcode_last_write_clk = maincpu_clk - 4;
                break;

            default:
                /* In all the other opcodes, all the write accesses are the last
                   ones.  */
                if (maincpu_num_write_cycles() != 0) {
                    last_opcode_last_write_clk = maincpu_clk - 2;
                    last_opcode_first_write_clk = maincpu_clk
                                                  - maincpu_num_write_cycles() * 2;
                } else {
                    last_opcode_first_write_clk = (CLOCK)0;
                    last_opcode_last_write_clk = last_opcode_first_write_clk;
                }
                break;
        }
    } else { /* offset <= 0, i.e. offset == 0 */
        /* If we are called with no offset, we don't have to care about write
           accesses.  */
        last_opcode_first_write_clk = last_opcode_last_write_clk = 0;
    }

    {
        CLOCK sub;
        CLOCK write_offset;

        if (ted.fetch_clk < (last_opcode_first_write_clk - 1)
            || ted.fetch_clk > last_opcode_last_write_clk) {
            sub = 0;
        } else {
            sub = last_opcode_last_write_clk - ted.fetch_clk + 1;
        }

        handle_fetch_matrix(offset, sub, &write_offset);
        last_opcode_first_write_clk += write_offset;
        last_opcode_last_write_clk += write_offset;
    }
    if ((offset > 11) && (ted.fastmode)) {
        dma_maincpu_steal_cycles(ted.fetch_clk, -(((signed)offset - 11) / 2), 0);
        ted_delay_oldclk(-(((signed)offset - 11) / 2));
    }
}

void ted_fetch_init(void)
{
    ted.raster_fetch_alarm = alarm_new(maincpu_alarm_context, "TEDRasterFetch",
                                       ted_fetch_alarm_handler, NULL);
}
