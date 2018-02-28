/*
 * m93c86.h
 *
 * Written by
 *  Groepaz/Hitmen <groepaz@gmx.net>
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

#ifndef VICE_M93C86
#define VICE_M93C86

#include "types.h"

extern BYTE m93c86_read_data(void);
extern void m93c86_write_data(BYTE value);
extern void m93c86_write_select(BYTE value);
extern void m93c86_write_clock(BYTE value);

extern int  m93c86_open_image(char *name, int rw);
extern void m93c86_close_image(int rw);

struct snapshot_s;
extern int m93c86_snapshot_read_module(struct snapshot_s *s);
extern int m93c86_snapshot_write_module(struct snapshot_s *s);

#endif
