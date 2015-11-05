/*
 * vicii-irq.h - IRQ related functions for the MOS 6569 (VIC-II) emulation.
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

#ifndef VICE_VICII_IRQ_H
#define VICE_VICII_IRQ_H

#include "types.h"

extern void vicii_irq_raster_set(CLOCK mclk);
extern void vicii_irq_raster_clear(CLOCK mclk);
extern void vicii_irq_sbcoll_set(void);
extern void vicii_irq_sbcoll_clear(void);
extern void vicii_irq_sscoll_set(void);
extern void vicii_irq_sscoll_clear(void);
extern void vicii_irq_lightpen_set(void);
extern void vicii_irq_lightpen_clear(void);

extern void vicii_irq_set_raster_line(unsigned int line);
extern void vicii_irq_check_state(BYTE value, unsigned int high);
extern void vicii_irq_set_line(void);
extern void vicii_irq_next_frame(void);
extern void vicii_irq_raster_trigger(void);

extern void vicii_irq_init(void);

#endif
