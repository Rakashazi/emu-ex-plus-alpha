/*
 * scpu64mem.h -- Emulation of the main 65816 processor.
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
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

#ifndef VICE_SCPU64CPU_H
#define VICE_SCPU64CPU_H

struct snapshot_module_s;

int scpu64_get_half_cycle(void);
void scpu64_set_fastmode(int mode);
void scpu64_set_fastmode_nosync(int mode);
void scpu64_set_simm_row_size(int value);
void scpu64_clock_read_stretch_io(void);
void scpu64_clock_read_stretch_eprom(void);
void scpu64_clock_write_stretch_eprom(void);
void scpu64_clock_write_stretch(void);
void scpu64_clock_write_stretch_io_start(void);
void scpu64_clock_write_stretch_io_start_cia(void);
void scpu64_clock_write_stretch_io(void);
void scpu64_clock_write_stretch_io_long(void);
void scpu64_clock_write_stretch_io_cia(void);
void scpu64_clock_read_stretch_simm(DWORD addr);
void scpu64_clock_write_stretch_simm(DWORD addr);
void scpu64_clock_read_ioram(void);
void maincpu_steal_cycles(void);
int scpu64_snapshot_write_cpu_state(struct snapshot_module_s *m);
int scpu64_snapshot_read_cpu_state(struct snapshot_module_s *m);
extern int scpu64_emulation_mode;
extern BYTE scpu64_fastmode;

#endif
