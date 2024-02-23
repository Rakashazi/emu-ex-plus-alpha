/*
 * drivecpu.h - 6502 processor emulation of CBM disk drives.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_DRIVECPU_H
#define VICE_DRIVECPU_H

#include "types.h"

/* Masks to extract information. */
#define OPINFO_NUMBER_MSK               0xff

/* Return the opcode number for `opinfo'.  */
#define OPINFO_NUMBER(opinfo)                   \
    ((opinfo) & OPINFO_NUMBER_MSK)

struct diskunit_context_s;
struct interrupt_cpu_status_s;
struct monitor_interface_s;
struct snapshot_s;

void drivecpu_setup_context(struct diskunit_context_s *drv, int i);

void drivecpu_init(struct diskunit_context_s *drv, int type);
void drivecpu_reset(struct diskunit_context_s *drv);
void drivecpu_sleep(struct diskunit_context_s *drv);
void drivecpu_wake_up(struct diskunit_context_s *drv);
void drivecpu_shutdown(struct diskunit_context_s *drv);
void drivecpu_reset_clk(struct diskunit_context_s *drv);
void drivecpu_trigger_reset(unsigned int dnr);
void drivecpu_set_overflow(struct diskunit_context_s *drv);

void drivecpu_execute(struct diskunit_context_s *drv, CLOCK clk_value);
int drivecpu_snapshot_write_module(struct diskunit_context_s *drv,
                                   struct snapshot_s *s);
int drivecpu_snapshot_read_module(struct diskunit_context_s *drv,
                                  struct snapshot_s *s);

#endif
