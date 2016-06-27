/*
 * ted-irq.c - IRQ related functions for the TED emulation.
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

#include "alarm.h"
#include "interrupt.h"
#include "maincpu.h"
#include "ted-irq.h"
#include "tedtypes.h"
#include "types.h"


void ted_irq_set_line(void)
{
    if (ted.irq_status & ted.regs[0x0a] & 0xfe) {
        ted.irq_status |= 0x80;
        maincpu_set_irq(ted.int_num, 1);
    } else {
        ted.irq_status &= 0x7f;
        maincpu_set_irq(ted.int_num, 0);
    }
}

static inline void ted_irq_set_line_clk(CLOCK mclk)
{
    if (ted.irq_status & ted.regs[0xa] & 0xfe) {
        ted.irq_status |= 0x80;
        maincpu_set_irq_clk(ted.int_num, 1, mclk);
    } else {
        ted.irq_status &= 0x7f;
        maincpu_set_irq_clk(ted.int_num, 0, mclk);
    }
}

void ted_irq_raster_set(CLOCK mclk)
{
    ted.irq_status |= 0x02;
    ted_irq_set_line_clk(mclk);
}

void ted_irq_raster_clear(CLOCK mclk)
{
    ted.irq_status &= 0xfd;
    ted_irq_set_line_clk(mclk);
}

void ted_irq_timer1_set(void)
{
    ted.irq_status |= 0x08;
    ted_irq_set_line();
}

void ted_irq_timer1_clear(void)
{
    ted.irq_status &= 0xf7;
    ted_irq_set_line();
}

void ted_irq_timer2_set(void)
{
    ted.irq_status |= 0x10;
    ted_irq_set_line();
}

void ted_irq_timer2_clear(void)
{
    ted.irq_status &= 0xef;
    ted_irq_set_line();
}

void ted_irq_timer3_set(void)
{
    ted.irq_status |= 0x40;
    ted_irq_set_line();
}

void ted_irq_timer3_clear(void)
{
    ted.irq_status &= 0xbf;
    ted_irq_set_line();
}

void ted_irq_set_raster_line(unsigned int line)
{
    if (line == ted.raster_irq_line && ted.raster_irq_clk != CLOCK_MAX) {
        return;
    }

    if (line < (unsigned int)ted.screen_height) {
        unsigned int current_line = TED_RASTER_Y(maincpu_clk);

        ted.raster_irq_clk = (TED_LINE_START_CLK(maincpu_clk)
                              + TED_RASTER_IRQ_DELAY - INTERRUPT_DELAY
                              + (ted.cycles_per_line
                                 * (line - current_line)));

        /* Raster interrupts on line 0 are delayed by 1 cycle.  */
        /* FIXME this needs to be checked */
        if (line == 0) {
            ted.raster_irq_clk++;
        }

        if (line <= current_line) {
            ted.raster_irq_clk += ((current_line >= ted.screen_height ? 512 : ted.screen_height)
                                   * ted.cycles_per_line);
        }
        alarm_set(ted.raster_irq_alarm, ted.raster_irq_clk);
    } else {
        unsigned int current_line = TED_RASTER_Y(maincpu_clk);
        if (current_line >= ted.screen_height) {
            ted.raster_irq_clk = (TED_LINE_START_CLK(maincpu_clk)
                                  + TED_RASTER_IRQ_DELAY - INTERRUPT_DELAY
                                  + (ted.cycles_per_line
                                     * (line - current_line)));

            if (line <= current_line) {
                ted.raster_irq_clk = CLOCK_MAX;
                alarm_unset(ted.raster_irq_alarm);
            } else {
                alarm_set(ted.raster_irq_alarm, ted.raster_irq_clk);
            }
        } else {
            ted.raster_irq_clk = CLOCK_MAX;
            alarm_unset(ted.raster_irq_alarm);
        }
    }

    TED_DEBUG_RASTER(("TED: update_raster_irq(): "
                      "ted.raster_irq_clk = %ul, "
                      "line = $%04X, "
                      "ted.regs[0x0a] & 2 = %d\n",
                      ted.raster_irq_clk, line, ted.regs[0x0a] & 2));

    ted.raster_irq_line = line;
}

void ted_irq_check_state(BYTE value, unsigned int high)
{
    unsigned int irq_line, line, user_irq_line;
    unsigned int old_raster_irq_line;

    user_irq_line = ted.raster_irq_line;

    if (high) {
        irq_line = (user_irq_line & 0xff) | ((value & 0x01) << 8);
    } else {
        irq_line = (user_irq_line & 0x100) | value;
    }

    if (irq_line == ted.raster_irq_line) {
        return;
    }

    line = TED_RASTER_Y(maincpu_clk);

    old_raster_irq_line = ted.raster_irq_line;
    ted_irq_set_raster_line(irq_line);

    if (ted.regs[0x0a] & 0x2) {
        int trigger_irq;

        trigger_irq = 0;

        if (maincpu_rmw_flag) {
            if (high) {
                if (TED_RASTER_CYCLE(maincpu_clk) == 0
                    && (line & 0xff) == 0) {
                    unsigned int previous_line = TED_PREVIOUS_LINE(line);

                    if (previous_line != old_raster_irq_line && ((old_raster_irq_line & 0xff) == (previous_line & 0xff))) {
                        trigger_irq = 1;
                    }
                } else {
                    if (line != old_raster_irq_line && (old_raster_irq_line & 0xff) == (line & 0xff)) {
                        trigger_irq = 1;
                    }
                }
            } else {
                if (TED_RASTER_CYCLE(maincpu_clk) == 0) {
                    unsigned int previous_line = TED_PREVIOUS_LINE(line);

                    if (previous_line != old_raster_irq_line
                        && ((old_raster_irq_line & 0x100) == (previous_line & 0x100))) {
                        trigger_irq = 1;
                    }
                } else {
                    if (line != old_raster_irq_line && (old_raster_irq_line & 0x100) == (line & 0x100)) {
                        trigger_irq = 1;
                    }
                }
            }
        }

        if (ted.raster_irq_line == line && line != old_raster_irq_line) {
            trigger_irq = 1;
        }

        if (trigger_irq) {
            ted_irq_raster_set(maincpu_clk);
        }
    }
}

void ted_irq_next_frame(void)
{
    ted.raster_irq_clk += ted.screen_height * ted.cycles_per_line;
    alarm_set(ted.raster_irq_alarm, ted.raster_irq_clk);
}

static void ted_irq_alarm_handler(CLOCK offset, void *data)
{
    ted_irq_raster_set(ted.raster_irq_clk);
    ted_irq_next_frame();
}

void ted_irq_init(void)
{
    ted.int_num = interrupt_cpu_status_int_new(maincpu_int_status, "TED");

    ted.raster_irq_alarm = alarm_new(maincpu_alarm_context, "TEDRasterIrq",
                                     ted_irq_alarm_handler, NULL);
}
