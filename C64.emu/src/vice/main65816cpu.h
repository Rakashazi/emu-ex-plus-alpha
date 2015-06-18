/*
 * main65816cpu.h - Emulation of the main 65816/65802 processor.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_MAIN65816CPU_H
#define VICE_MAIN65816CPU_H

#include "types.h"

/* Mask: BA low */
#define MAINCPU_BA_LOW_VICII 1
#define MAINCPU_BA_LOW_REU   2
extern int maincpu_ba_low_flags;

struct WDC65816_regs_s;
extern struct WDC65816_regs_s maincpu_regs;

extern CLOCK maincpu_clk;

/* ------------------------------------------------------------------------- */

struct alarm_context_s;
struct snapshot_s;
struct clk_guard_s;
struct monitor_interface_s;

extern struct alarm_context_s *maincpu_alarm_context;
extern struct clk_guard_s *maincpu_clk_guard;
extern struct monitor_interface_s *maincpu_monitor_interface;

extern void maincpu_resync_limits(void);
extern void maincpu_init(void);
extern void maincpu_early_init(void);
extern void maincpu_shutdown(void);
extern void maincpu_reset(void);
extern void maincpu_mainloop(void);
extern struct monitor_interface_s *maincpu_monitor_interface_get(void);
extern int maincpu_snapshot_read_module(struct snapshot_s *s);
extern int maincpu_snapshot_write_module(struct snapshot_s *s);

extern void maincpu_set_pc(int);
extern void maincpu_set_a(int);
extern void maincpu_set_x(int);
extern void maincpu_set_y(int);
extern void maincpu_set_sign(int);
extern void maincpu_set_zero(int);
extern void maincpu_set_carry(int);
extern void maincpu_set_interrupt(int);
extern unsigned int maincpu_get_pc(void);
extern unsigned int maincpu_get_a(void);
extern unsigned int maincpu_get_x(void);
extern unsigned int maincpu_get_y(void);
extern unsigned int maincpu_get_sp(void);

#endif
