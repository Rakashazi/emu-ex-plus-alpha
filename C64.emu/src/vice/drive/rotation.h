/*
 * rotation.h
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

#ifndef VICE_ROTATION_H
#define VICE_ROTATION_H

#include "types.h"

struct drive_s;

/* 875ns delay (14*62.5ns R cycles) for data bus read access */
#define BUS_READ_DELAY 14

void rotation_init(int freq, unsigned int dnr);
void rotation_reset(struct drive_s *drive);
void rotation_speed_zone_set(unsigned int zone, unsigned int dnr);
void rotation_table_get(uint32_t *rotation_table_ptr);
void rotation_table_set(uint32_t *rotation_table_ptr);
void rotation_overflow_callback(CLOCK sub, unsigned int dnr);
void rotation_change_mode(unsigned int dnr);
void rotation_begins(struct drive_s *dptr);
void rotation_rotate_disk(struct drive_s *dptr);
uint8_t rotation_sync_found(struct drive_s *dptr);
void rotation_byte_read(struct drive_s *dptr);

#endif
