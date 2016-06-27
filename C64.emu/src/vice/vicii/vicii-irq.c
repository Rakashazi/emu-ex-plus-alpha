/*
 * vicii-irq.c - IRQ related functions for the MOS 6569 (VIC-II) emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
 * DTV sections written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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
#include "types.h"
#include "vicii-irq.h"
#include "viciitypes.h"
#include "c64dtvblitter.h"
#include "c64dtvdma.h"

void vicii_irq_set_line(void)
{
    if (vicii.irq_status & vicii.regs[0x1a]) {
        vicii.irq_status |= 0x80;
        maincpu_set_irq(vicii.int_num, 1);
    } else {
        vicii.irq_status &= 0x7f;
        maincpu_set_irq(vicii.int_num, 0);
    }
}

static inline void vicii_irq_set_line_clk(CLOCK mclk)
{
    if (vicii.irq_status & vicii.regs[0x1a]) {
        vicii.irq_status |= 0x80;
        maincpu_set_irq_clk(vicii.int_num, 1, mclk);
    } else {
        vicii.irq_status &= 0x7f;
        maincpu_set_irq_clk(vicii.int_num, 0, mclk);
    }
}

void vicii_irq_raster_set(CLOCK mclk)
{
    vicii.irq_status |= 0x1;
    vicii_irq_set_line_clk(mclk);
}

void vicii_irq_raster_clear(CLOCK mclk)
{
    vicii.irq_status &= 0xfe;
    vicii_irq_set_line_clk(mclk);
}

void vicii_irq_sbcoll_set(void)
{
    vicii.irq_status |= 0x2;
    vicii_irq_set_line();
}

void vicii_irq_sbcoll_clear(void)
{
    vicii.irq_status &= 0xfd;
    vicii_irq_set_line();
}

void vicii_irq_sscoll_set(void)
{
    vicii.irq_status |= 0x4;
    vicii_irq_set_line();
}

void vicii_irq_sscoll_clear(void)
{
    vicii.irq_status &= 0xfb;
    vicii_irq_set_line();
}

void vicii_irq_lightpen_set(CLOCK mclk)
{
    vicii.irq_status |= 0x8;
    vicii_irq_set_line_clk(mclk);
}

void vicii_irq_lightpen_clear(CLOCK mclk)
{
    vicii.irq_status &= 0xf7;
    vicii_irq_set_line_clk(mclk);
}

void vicii_irq_set_raster_line(unsigned int line)
{
    if (vicii.raster_irq_prevent) {
        vicii.raster_irq_clk = CLOCK_MAX;
        alarm_unset(vicii.raster_irq_alarm);
        return;
    }

    if (line == vicii.raster_irq_line && vicii.raster_irq_clk != CLOCK_MAX) {
        return;
    }

    if (line < (unsigned int)vicii.screen_height) {
        unsigned int current_line = VICII_RASTER_Y(maincpu_clk);

        vicii.raster_irq_clk = (VICII_LINE_START_CLK(maincpu_clk)
                                + VICII_RASTER_IRQ_DELAY - INTERRUPT_DELAY
                                + (vicii.cycles_per_line
                                   * (line - current_line)));
        if (vicii.viciidtv) {
            vicii.raster_irq_clk += vicii.raster_irq_offset;
        }

        /* Raster interrupts on line 0 are delayed by 1 cycle.  */
        if (line == 0) {
            vicii.raster_irq_clk++;
        }

        if (line <= current_line) {
            vicii.raster_irq_clk += (vicii.screen_height * vicii.cycles_per_line);
        }
        alarm_set(vicii.raster_irq_alarm, vicii.raster_irq_clk);
    } else {
        VICII_DEBUG_RASTER(("update_raster_irq(): "
                            "raster compare out of range ($%04X)!", line));
        vicii.raster_irq_clk = CLOCK_MAX;
        alarm_unset(vicii.raster_irq_alarm);
    }

    VICII_DEBUG_RASTER(("update_raster_irq(): "
                        "vicii.raster_irq_clk = %ul, "
                        "line = $%04X, "
                        "vicii.regs[0x1a] & 1 = %d",
                        vicii.raster_irq_clk, line, vicii.regs[0x1a] & 1));

    vicii.raster_irq_line = line;
}

void vicii_irq_check_state(BYTE value, unsigned int high)
{
    unsigned int irq_line, line;
    unsigned int old_raster_irq_line;
    CLOCK old_raster_irq_clk = vicii.raster_irq_clk;

    if (high) {
        irq_line = (vicii.raster_irq_line & 0xff) | ((value & 0x80) << 1);
    } else {
        irq_line = (vicii.raster_irq_line & 0x100) | value;
    }

    if (irq_line == vicii.raster_irq_line) {
        return;
    }

    line = VICII_RASTER_Y(maincpu_clk);

    old_raster_irq_line = vicii.raster_irq_line;
    vicii_irq_set_raster_line(irq_line);

    if (vicii.regs[0x1a] & 0x1) {
        int trigger_irq;

        trigger_irq = 0;

        if (old_raster_irq_clk == VICII_LINE_START_CLK(maincpu_clk) + (line == 0 ? 1 : 0)) {
            trigger_irq = 2;
        }

        if (maincpu_rmw_flag) {
            if (high) {
                if (VICII_RASTER_CYCLE(maincpu_clk) == 0
                    && (line & 0xff) == 0) {
                    unsigned int previous_line = VICII_PREVIOUS_LINE(line);

                    if (previous_line != old_raster_irq_line
                        && ((old_raster_irq_line & 0xff)
                            == (previous_line & 0xff))) {
                        trigger_irq = 1;
                    }
                } else {
                    if (line != old_raster_irq_line
                        && (old_raster_irq_line & 0xff) == (line & 0xff)) {
                        trigger_irq = 1;
                    }
                }
            } else {
                if (VICII_RASTER_CYCLE(maincpu_clk) == 0) {
                    unsigned int previous_line = VICII_PREVIOUS_LINE(line);

                    if (previous_line != old_raster_irq_line
                        && ((old_raster_irq_line & 0x100)
                            == (previous_line & 0x100))) {
                        trigger_irq = 1;
                    }
                } else {
                    if (line != old_raster_irq_line
                        && (old_raster_irq_line & 0x100) == (line & 0x100)) {
                        trigger_irq = 1;
                    }
                }
            }
        }

        if (vicii.raster_irq_line == line && line != old_raster_irq_line) {
            trigger_irq = 1;
        }

        if (trigger_irq == 1) {
            vicii_irq_raster_set(maincpu_clk);
        }

        if (trigger_irq == 2) {
            vicii_irq_raster_set(old_raster_irq_clk);
        }
    }
}

void vicii_irq_next_frame(void)
{
    vicii.raster_irq_clk += vicii.screen_height * vicii.cycles_per_line;
    alarm_set(vicii.raster_irq_alarm, vicii.raster_irq_clk);
}

/* If necessary, emulate a raster compare IRQ. This is called when the raster
   line counter matches the value stored in the raster line register.  */
void vicii_irq_alarm_handler(CLOCK offset, void *data)
{
    /* Scheduled Blitter */
    if (blitter_on_irq & 0x10) {
        c64dtvblitter_trigger_blitter();
    }
    /* Scheduled DMA */
    if (dma_on_irq & 0x10) {
        c64dtvdma_trigger_dma();
    }

    vicii_irq_raster_set(vicii.raster_irq_clk);
    vicii_irq_next_frame();
}

void vicii_irq_init(void)
{
    vicii.int_num = interrupt_cpu_status_int_new(maincpu_int_status, "VICII");

    vicii.raster_irq_alarm = alarm_new(maincpu_alarm_context, "VicIIRasterIrq",
                                       vicii_irq_alarm_handler, NULL);
}
