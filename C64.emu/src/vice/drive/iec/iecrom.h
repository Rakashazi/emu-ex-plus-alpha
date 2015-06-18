/*
 * iecrom.h
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

#ifndef VICE_IECROM_H
#define VICE_IECROM_H

#include "types.h"

struct drive_s;

extern void iecrom_init(void);
extern void iecrom_setup_image(struct drive_s *drive);
extern int iecrom_read(unsigned int type, WORD addr, BYTE *data);
extern int iecrom_check_loaded(unsigned int type);
extern void iecrom_do_checksum(struct drive_s *drive);

extern int iecrom_load_1540(void);
extern int iecrom_load_1541(void);
extern int iecrom_load_1541ii(void);
extern int iecrom_load_1570(void);
extern int iecrom_load_1571(void);
extern int iecrom_load_1581(void);
extern int iecrom_load_2000(void);
extern int iecrom_load_4000(void);

#endif
