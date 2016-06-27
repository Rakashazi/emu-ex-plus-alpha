/*
 * vicii-draw-cycle.h - Cycle based rendering for the VIC-II emulation.
 *
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
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

#ifndef VICE_VICII_DRAW_CYCLE_H
#define VICE_VICII_DRAW_CYCLE_H

extern void vicii_draw_cycle(void);
extern void vicii_draw_cycle_init(void);

struct snapshot_module_s;

extern int vicii_draw_cycle_snapshot_write(struct snapshot_module_s *m);
extern int vicii_draw_cycle_snapshot_read(struct snapshot_module_s *m);

#endif
