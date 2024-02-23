/** \file   archdep_tick.h
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

#include <stdint.h>

#define MILLI_PER_SECOND    (1000)
#define MICRO_PER_SECOND    (1000 * 1000)
#define NANO_PER_SECOND     (1000 * 1000 * 1000)

/*
 * The internal VICE timing resolution.
 * BTW, Windows can't be more accurate than (NANO_PER_SECOND / 100).
 */
#define TICK_PER_SECOND     (MICRO_PER_SECOND)

/* Using a 32-bit type for tick_t prevents wraparound bugs on platforms with 32-bit timers */
typedef uint32_t tick_t;

#define TICK_TO_MILLI(tick) ((uint32_t) (uint64_t)((double)(tick) * ((double)MILLI_PER_SECOND / TICK_PER_SECOND)))
#define TICK_TO_MICRO(tick) ((uint32_t) (uint64_t)((double)(tick) * ((double)MICRO_PER_SECOND / TICK_PER_SECOND)))
#define TICK_TO_NANO(tick)  ((uint64_t) (uint64_t)((double)(tick) * ((double)NANO_PER_SECOND  / TICK_PER_SECOND)))
#define NANO_TO_TICK(nano)  ((tick_t)   (uint64_t)((double)(nano) / ((double)NANO_PER_SECOND  / TICK_PER_SECOND)))

void tick_init(void);

/* number of ticks per second */
tick_t tick_per_second(void);

/* Get time in ticks. */
tick_t tick_now(void);

/* Get time in ticks, compensating for the +/- 1 tick that is possible on Windows. */
tick_t tick_now_after(tick_t previous_tick);

/* Get number of ticks since a previous tick, compensating for the +/- 1 tick that is possible on Windows. */
tick_t tick_now_delta(tick_t previous_tick);

/* Sleep a number of ticks. */
void tick_sleep(tick_t delay);

#endif
