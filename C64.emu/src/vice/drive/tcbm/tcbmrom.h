/*
 * tcbmrom.h
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

#ifndef VICE_TCBMROM_H
#define VICE_TCBMROM_H

#include "types.h"

struct drive_s;

extern void tcbmrom_init(void);
extern void tcbmrom_setup_image(struct drive_s *drive);
extern int tcbmrom_read(unsigned int type, WORD addr, BYTE *data);
extern int tcbmrom_check_loaded(unsigned int type);

extern int tcbmrom_load_1551(void);

#endif
