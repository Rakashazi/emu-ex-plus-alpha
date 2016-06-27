/*
 * cbm2acia.h  - Template file for ACIA 6551 emulation.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

/* ACIA 6551 register number declarations.
 * Interrupt signals are defined in interrupt.h. */

#ifndef VICE_CBM2ACIA_H
#define VICE_CBM2ACIA_H

#include "types.h"

struct snapshot_s;

extern void acia1_init(void);
extern BYTE acia1_read(WORD a);
extern BYTE acia1_peek(WORD a);
extern void acia1_store(WORD a, BYTE b);
extern void acia1_reset(void);

extern int acia1_cmdline_options_init(void);
extern int acia1_resources_init(void);

extern int acia1_snapshot_write_module(struct snapshot_s *p);
extern int acia1_snapshot_read_module(struct snapshot_s *p);

#endif
