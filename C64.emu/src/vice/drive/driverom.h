/*
 * driverom.h
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

#ifndef VICE_DRIVEROM_H
#define VICE_DRIVEROM_H

#include "types.h"

struct drive_s;
struct snapshot_s;

#define DRIVE_ROM1540_SIZE            0x4000
#define DRIVE_ROM1540_SIZE_EXPANDED   0x8000
#define DRIVE_ROM1541_SIZE            0x4000
#define DRIVE_ROM1541_SIZE_EXPANDED   0x8000
#define DRIVE_ROM1541II_SIZE          0x4000
#define DRIVE_ROM1541II_SIZE_EXPANDED 0x8000
#define DRIVE_ROM1551_SIZE            0x4000
#define DRIVE_ROM1570_SIZE            0x8000
#define DRIVE_ROM1571_SIZE            0x8000
#define DRIVE_ROM1571CR_SIZE          0x8000
#define DRIVE_ROM1581_SIZE            0x8000
#define DRIVE_ROM2000_SIZE            0x8000
#define DRIVE_ROM4000_SIZE            0x8000
#define DRIVE_ROM2031_SIZE            0x4000
#define DRIVE_ROM1001_SIZE            0x4000 /* same as ROM8050 and ROM8250 !*/
#define DRIVE_ROM2040_SIZE            0x2000
#define DRIVE_ROM3040_SIZE            0x3000
#define DRIVE_ROM4040_SIZE            0x3000

extern void driverom_init(void);
extern void driverom_initialize_traps(struct drive_s *drive);
extern int driverom_load(const char *resource_name, BYTE *drive_rom, unsigned
                         int *loaded, int min, int max, const char *name,
                         unsigned int type, unsigned int *size);
extern int driverom_load_images(void);
extern int driverom_snapshot_write(struct snapshot_s *s, const struct drive_s *drive);
extern int driverom_snapshot_read(struct snapshot_s *s, struct drive_s *drive);

#endif
