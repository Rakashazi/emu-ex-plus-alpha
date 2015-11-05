/*
 * cbm2cia.h - Definitions for MOS6526 (CIA) chip emulation.
 *
 * Written by
 *  Jouko Valta <jopi@stekt.oulu.fi>
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

#ifndef VICE_CBM2CIA_H
#define VICE_CBM2CIA_H

#include "types.h"

struct cia_context_s;
struct machine_context_s;

extern void cia1_setup_context(struct machine_context_s *machine_context);

extern void cia1_init(struct cia_context_s *cia_context);
extern void cia1_store(WORD addr, BYTE value);
extern BYTE cia1_read(WORD addr);
extern BYTE cia1_peek(WORD addr);

extern void cia1_set_ieee_dir(struct cia_context_s *cia_context, int isout);

extern void cia1_update_model(void);

extern void cia1_set_timing(struct cia_context_s *cia_context, int tickspersec, int powerfreq);

#endif
