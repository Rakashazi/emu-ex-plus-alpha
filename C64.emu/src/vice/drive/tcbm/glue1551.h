/*
 * glue1551.h - 1551 glue logic.
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

#ifndef VICE_GLUE1551_H
#define VICE_GLUE1551_H

#include "types.h"

struct diskunit_context_s;

uint8_t glue1551_port0_read(struct diskunit_context_s *drv);
uint8_t glue1551_port1_read(struct diskunit_context_s *drv);
void glue1551_port0_store(struct diskunit_context_s *drv, uint8_t value);
void glue1551_port1_store(struct diskunit_context_s *drv, uint8_t value);

void glue1551_init(struct diskunit_context_s *drv);
void glue1551_reset(struct diskunit_context_s *drv);

#endif
