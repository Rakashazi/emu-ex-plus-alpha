/*
 * profiler.h -- CPU Profiler
 *
 * Written by
 *  Oskar Linde <oskar.linde@gmail.com>
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

#ifndef VICE_PROFILER_H
#define VICE_PROFILER_H

#include "types.h"

extern bool maincpu_profiling;

/* resets sample statistics and starts profiling sample collection */
void profile_start(void);

/* stops profiling and writes profiling log to disk */
void profile_stop(void);

/* called by the CPU for each instruction */
void profile_sample_start(uint16_t pc);
void profile_sample_finish(uint16_t cycle_time, uint16_t stolen_cycles);

/* called whenever a JSR is encountered */
void profile_jsr(uint16_t pc_dst, uint16_t pc_src, uint8_t sp);

/* interrupts are handled like JSRs with PC as one of the
 * interrupt handler addresses (0xfffa-0xffff)
 * for interrupts sp = sp+1 to accommodate the pushed status register */
void profile_int(uint16_t pc_dst,
                 uint16_t handler,
                 uint8_t sp,
                 uint16_t cycle_time);

/* called whenever an RTS/RI is called */
void profile_rtx(uint8_t sp);

void profile_shutdown(void);

#endif /* VICE_PROFILER_H */
