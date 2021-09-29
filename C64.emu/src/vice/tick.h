/** \file   tick.c
 * \brief   Relating to the management of time.
 *
 * \author  David Hogan <david.q.hogan@gmail.com>
 */

/*
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

#ifndef VICE_TICK_H
#define VICE_TICK_H

extern void tick_init(void);

/* number of ticks per second */
extern unsigned long tick_per_second(void);

/* Get time in ticks. */
extern unsigned long tick_now(void);

/* Get time in ticks, compensating for the +/- 1 tick that is possible on Windows. */
extern unsigned long tick_after(unsigned long previous_tick);

/* Get number of ticks since a previous tick, compensating for the +/- 1 tick that is possible on Windows. */
extern unsigned long tick_delta(unsigned long previous_tick);

/* Sleep a number of ticks. */
extern void tick_sleep(unsigned long delay);

#endif