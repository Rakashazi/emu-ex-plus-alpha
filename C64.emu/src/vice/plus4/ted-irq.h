/*
 * ted-irq.c - IRQ related functions for the TED emulation.
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

#ifndef VICE_TED_IRQ_H
#define VICE_TED_IRQ_H

#include "types.h"

extern void ted_irq_raster_set(CLOCK mclk);
extern void ted_irq_raster_clear(CLOCK mclk);
extern void ted_irq_timer1_set(void);
extern void ted_irq_timer1_clear(void);
extern void ted_irq_timer2_set(void);
extern void ted_irq_timer2_clear(void);
extern void ted_irq_timer3_set(void);
extern void ted_irq_timer3_clear(void);

extern void ted_irq_set_raster_line(unsigned int line);
extern void ted_irq_check_state(BYTE value, unsigned int high);
extern void ted_irq_set_line(void);
extern void ted_irq_next_frame(void);

extern void ted_irq_init(void);

#endif
