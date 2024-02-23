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
struct diskunit_context_s;

void cia1571_setup_context(struct diskunit_context_s *ctxptr);
void cia1581_setup_context(struct diskunit_context_s *ctxptr);

void cia1571_init(struct diskunit_context_s *ctxptr);
void cia1571_store(struct diskunit_context_s *ctxptr, uint16_t addr, uint8_t value);
uint8_t cia1571_read(struct diskunit_context_s *ctxptr, uint16_t addr);
uint8_t cia1571_peek(struct diskunit_context_s *ctxptr, uint16_t addr);
int cia1571_dump(struct diskunit_context_s *ctxptr, uint16_t addr);

void mos5710_store(struct diskunit_context_s *ctxptr, uint16_t addr, uint8_t value);
uint8_t mos5710_read(struct diskunit_context_s *ctxptr, uint16_t addr);
uint8_t mos5710_peek(struct diskunit_context_s *ctxptr, uint16_t addr);
int mos5710_dump(struct diskunit_context_s *ctxptr, uint16_t addr);

void cia1581_init(struct diskunit_context_s *ctxptr);
void cia1581_store(struct diskunit_context_s *ctxptr, uint16_t addr, uint8_t value);
uint8_t cia1581_read(struct diskunit_context_s *ctxptr, uint16_t addr);
uint8_t cia1581_peek(struct diskunit_context_s *ctxptr, uint16_t addr);
int cia1581_dump(struct diskunit_context_s *ctxptr, uint16_t addr);

void cia1571_set_timing(struct cia_context_s *cia_context, int tickspersec, int powerfreq);
void cia1581_set_timing(struct cia_context_s *cia_context, int tickspersec, int powerfreq);

#endif
