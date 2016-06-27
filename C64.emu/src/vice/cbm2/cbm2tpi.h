/*
 * cbm2tpi.h - IEEE488 and keyboard interface for the CBM-II
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_CBM2TPI_H
#define VICE_CBM2TPI_H

#include "types.h"

struct machine_context_s;
struct tpi_context_s;

extern void tpi1_setup_context(struct machine_context_s *machine_context);
extern void tpi1_init(struct tpi_context_s *tpi_context);
extern void tpi1_store(WORD addr, BYTE byte);
extern BYTE tpi1_read(WORD addr);
extern BYTE tpi1_peek(WORD addr);

extern void tpi1_set_tape_sense(int v);

extern void tpi2_setup_context(struct machine_context_s *machine_context);
extern void tpi2_init(struct tpi_context_s *tpi_context);
extern void tpi2_store(WORD addr, BYTE byte);
extern BYTE tpi2_read(WORD addr);
extern BYTE tpi2_peek(WORD addr);

extern void set_cbm2_model_port_mask(BYTE);

#endif
