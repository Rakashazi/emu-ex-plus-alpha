/*
 * ieeerom.h
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

#ifndef VICE_IEEEROM_H
#define VICE_IEEEROM_H

#include "types.h"

struct drive_s;

extern void ieeerom_init(void);
extern void ieeerom_setup_image(struct drive_s *drive);
extern int ieeerom_read(unsigned int type, WORD addr, BYTE *data);
extern int ieeerom_check_loaded(unsigned int type);

extern int ieeerom_load_2031(void);
extern int ieeerom_load_1001(void);
extern int ieeerom_load_2040(void);
extern int ieeerom_load_3040(void);
extern int ieeerom_load_4040(void);

#endif
