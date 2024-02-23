/*
 * riotd.h - Drive VIA definitions.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_RIOTD_H
#define VICE_RIOTD_H

#include "types.h"

struct diskunit_context_s;
struct riot_context_s;

void riot1_setup_context(struct diskunit_context_s *ctxptr);
void riot2_setup_context(struct diskunit_context_s *ctxptr);

void riot2_set_atn(struct riot_context_s *riot_context, int state);
void riot1_set_atn(struct riot_context_s *riot_context, uint8_t state);

void riot1_set_pardata(struct riot_context_s *riot_context);

void riot1_init(struct diskunit_context_s *ctxptr);
void riot1_store(struct diskunit_context_s *ctxptr, uint16_t addr, uint8_t byte);
uint8_t riot1_read(struct diskunit_context_s *ctxptr, uint16_t addr);
uint8_t riot1_peek(struct diskunit_context_s *ctxptr, uint16_t addr);

void riot2_init(struct diskunit_context_s *ctxptr);
void riot2_store(struct diskunit_context_s *ctxptr, uint16_t addr, uint8_t byte);
uint8_t riot2_read(struct diskunit_context_s *ctxptr, uint16_t addr);
uint8_t riot2_peek(struct diskunit_context_s *ctxptr, uint16_t addr);

#endif
