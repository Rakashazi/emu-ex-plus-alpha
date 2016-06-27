/*
 * via20via.h - VIC20 VIA emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_VIC20VIA_H
#define VICE_VIC20VIA_H

#include "types.h"

struct machine_context_s;
struct via_context_s;

extern void vic20via1_setup_context(struct machine_context_s *machine_context);
extern void via1_init(struct via_context_s *via_context);
extern void via1_store(WORD addr, BYTE byte);
extern BYTE via1_read(WORD addr);
extern BYTE via1_peek(WORD addr);

extern void vic20via2_setup_context(struct machine_context_s *machine_context);
extern void via2_init(struct via_context_s *via_context);
extern void via2_store(WORD addr, BYTE byte);
extern BYTE via2_read(WORD addr);
extern BYTE via2_peek(WORD addr);

extern void via2_set_tape_sense(int v);
extern void via2_check_lightpen(void);

#endif
