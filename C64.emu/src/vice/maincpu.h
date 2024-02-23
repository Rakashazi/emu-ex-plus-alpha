/*
 * maincpu.h - Emulation of the main 6510 processor.
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

#ifndef VICE_MAINCPU_H
#define VICE_MAINCPU_H

#include "mainlock.h"
#include "types.h"
#include "vsyncapi.h"

/* Information about the last opcode executed by the main CPU.  */
extern unsigned int last_opcode_info;
extern unsigned int last_opcode_addr;

/* Masks to extract information. */
#define OPINFO_NUMBER_MSK               0xff

/* Return the opcode number for `opinfo'.  */
#define OPINFO_NUMBER(opinfo)                   \
    ((opinfo) & OPINFO_NUMBER_MSK)

/* The VIC-II emulation needs this ugly hack.  */
/* FIXME: this should really be uint16_t, but it breaks things (eg trap17.prg) */
extern unsigned int reg_pc;

#ifdef C64DTV
struct mos6510dtv_regs_s;
extern struct mos6510dtv_regs_s maincpu_regs;
#else
struct mos6510_regs_s;
extern struct mos6510_regs_s maincpu_regs;
#endif

extern int maincpu_rmw_flag;
extern CLOCK maincpu_clk;
extern CLOCK maincpu_clk_limit;

/* 8502 cycle stretch indicator */
extern int maincpu_stretch;

/* 8502 memory refresh alarm */
extern CLOCK c128cpu_memory_refresh_clk;

/* C64DTV negative clock counter for cycle exact operations,
   also used by common code in vicii/.
   Actual variable in c64dtv/c64dtvcpu.c or vicii/vicii-stubs.c. */
extern int dtvclockneg;

/* ------------------------------------------------------------------------- */

struct alarm_context_s;
struct snapshot_s;
struct monitor_interface_s;

extern const CLOCK maincpu_opcode_write_cycles[];
extern struct alarm_context_s *maincpu_alarm_context;
extern CLOCK maincpu_clk;
extern struct monitor_interface_s *maincpu_monitor_interface;

/* Return the number of write accesses in the last opcode emulated. */
#define maincpu_num_write_cycles() maincpu_opcode_write_cycles[OPINFO_NUMBER(last_opcode_info)]

void maincpu_resync_limits(void);
void maincpu_init(void);
void maincpu_early_init(void);
void maincpu_shutdown(void);
void maincpu_reset(void);
void maincpu_mainloop(void);
struct monitor_interface_s *maincpu_monitor_interface_get(void);
int maincpu_snapshot_read_module(struct snapshot_s *s);
int maincpu_snapshot_write_module(struct snapshot_s *s);

void maincpu_set_pc(int);
void maincpu_set_a(int);
void maincpu_set_x(int);
void maincpu_set_y(int);
void maincpu_set_sign(int);
void maincpu_set_zero(int);
void maincpu_set_carry(int);
void maincpu_set_interrupt(int);
unsigned int maincpu_get_pc(void);
unsigned int maincpu_get_a(void);
unsigned int maincpu_get_x(void);
unsigned int maincpu_get_y(void);
unsigned int maincpu_get_sp(void);

#endif
