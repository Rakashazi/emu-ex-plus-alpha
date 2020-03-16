/*
 * vicii-lightpen.h - VIC-II light pen emulation.
 *
 * Written by
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

#include "maincpu.h"
#include "types.h"
#include "vicii-chip-model.h"
#include "vicii-irq.h"
#include "vicii-lightpen.h"
#include "vicii.h"
#include "viciitypes.h"

/* Set light pen input state. Used by c64cia1.c.  */
void vicii_set_light_pen(CLOCK mclk, int state)
{
    if (state) {
        /* add offset depending on chip model (FIXME use proper variable) */
        vicii.light_pen.x_extra_bits = (vicii.color_latency ? 2 : 1);
        /* delay trigger by 1 clock */
        vicii.light_pen.trigger_cycle = mclk + 1;
    }
    vicii.light_pen.state = state;
}

/* Trigger the light pen. Used internally.  */
void vicii_trigger_light_pen_internal(int retrigger)
{
    int x;
    unsigned int y;

    /* Unset the trigger cycle (originating from lightpen.c).
       If this function was call from elsewhere before the cycle,
       then the light pen was triggered by other means than
       an actual light pen and the following "if" would make the
       scheduled "actual" triggering pointless. */
    vicii.light_pen.trigger_cycle = CLOCK_MAX;

    if (vicii.light_pen.triggered) {
        return;
    }

    vicii.light_pen.triggered = 1;

    y = vicii.raster_line;

    /* don't trigger on the last line, except on the first cycle */
    if ((y == (vicii.screen_height - 1)) && (vicii.raster_cycle > 0)) {
        return;
    }

    x = cycle_get_xpos(vicii.cycle_table[vicii.raster_cycle]) / 2;

    /* add offset from chip model or an actual light pen */
    x += vicii.light_pen.x_extra_bits;

    /* signaled retrigger from vicii_cycle */
    if (retrigger) {
        switch (vicii.cycles_per_line) {
            /* TODO case 64: */
            case 63:
            default:
                x = 0xd1;
                break;
            case 65:
                x = 0xd5;
                break;
        }

        /* On 6569R1 the interrupt is triggered only when the line is low
           on the first cycle of the frame. */
        if (vicii.lightpen_old_irq_mode) {
            vicii_irq_lightpen_set();
        }
    }

    vicii.light_pen.x = x;
    vicii.light_pen.y = y;

    vicii.light_pen.x_extra_bits = 0;

    if (!vicii.lightpen_old_irq_mode) {
        vicii_irq_lightpen_set();
    }
}

/* Calculate lightpen pulse time based on x/y.  */
CLOCK vicii_lightpen_timing(int x, int y)
{
    CLOCK pulse_time = maincpu_clk;

    x += 0x80 - vicii.screen_leftborderwidth;
    y += vicii.first_displayed_line;

    /* Check if x would wrap to previous line */
    if (x < 104) {
        /* lightpen is off screen */
        pulse_time = 0;
    } else {
        pulse_time += (x / 8) + (y * vicii.cycles_per_line);

        /* Store x extra bits for sub CLK precision */
        vicii.light_pen.x_extra_bits = (x >> 1) & 0x3;
    }

    return pulse_time;
}

/* Trigger the light pen. Used by lightpen.c only.  */
void vicii_trigger_light_pen(CLOCK mclk)
{
    /* Record the real trigger time */
    vicii.light_pen.trigger_cycle = mclk;
}
