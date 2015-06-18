/*
 * ciad.h - Drive CIA definitions.
 *
 * Written by
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

#ifndef VICE_CIAD_H
#define VICE_CIAD_H

#include "types.h"

struct cia_context_s;
struct cia_initdesc_s;
struct drive_context_s;

extern void cia1571_setup_context(struct drive_context_s *ctxptr);
extern void cia1581_setup_context(struct drive_context_s *ctxptr);

extern void cia1571_init(struct drive_context_s *ctxptr);
extern void cia1571_store(struct drive_context_s *ctxptr, WORD addr, BYTE value);
extern BYTE cia1571_read(struct drive_context_s *ctxptr, WORD addr);
extern BYTE cia1571_peek(struct drive_context_s *ctxptr, WORD addr);

extern void cia1581_init(struct drive_context_s *ctxptr);
extern void cia1581_store(struct drive_context_s *ctxptr, WORD addr, BYTE value);
extern BYTE cia1581_read(struct drive_context_s *ctxptr, WORD addr);
extern BYTE cia1581_peek(struct drive_context_s *ctxptr, WORD addr);

extern void cia1571_set_timing(struct cia_context_s *cia_context, int tickspersec, int powerfreq);
extern void cia1581_set_timing(struct cia_context_s *cia_context, int tickspersec, int powerfreq);

#endif
