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

uint8_t m93c86_read_data(void);
void m93c86_write_data(uint8_t value);
void m93c86_write_select(uint8_t value);
void m93c86_write_clock(uint8_t value);

int  m93c86_open_image(char *name, int rw);
int  m93c86_save_image(const char *name);
int  m93c86_flush_image(void);
void m93c86_close_image(int rw);
void m93c86_set_image_rw(int rw);

struct snapshot_s;

int m93c86_snapshot_read_module(struct snapshot_s *s);
int m93c86_snapshot_write_module(struct snapshot_s *s);

#endif
