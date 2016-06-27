/*
 * vicii-irq.c - IRQ related functions for the VIC-II emulation.
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

#include "interrupt.h"
#include "mainc64cpu.h"
#include "types.h"
#include "vicii-irq.h"
#include "viciitypes.h"

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

void vicii_irq_lightpen_set(void)
{
    vicii.irq_status |= 0x8;
    vicii_irq_set_line();
}

void vicii_irq_lightpen_clear(void)
{
    vicii.irq_status &= 0xf7;
    vicii_irq_set_line();
}

void vicii_irq_set_raster_line(unsigned int line)
{
}

void vicii_irq_check_state(BYTE value, unsigned int high)
{
}

/* If necessary, emulate a raster compare IRQ. This is called when the raster
   line counter matches the value stored in the raster line register.  */
void vicii_irq_raster_trigger(void)
{
    if (!(vicii.irq_status & 0x1)) {
        vicii_irq_raster_set(maincpu_clk);
    }
}

void vicii_irq_init(void)
{
    vicii.int_num = interrupt_cpu_status_int_new(maincpu_int_status, "VICII");
}
