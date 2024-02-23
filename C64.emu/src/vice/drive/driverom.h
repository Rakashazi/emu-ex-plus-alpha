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
struct diskunit_context_s;
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
#define DRIVE_ROMCMDHD_SIZE           0x4000
#define DRIVE_ROM2031_SIZE            0x4000
#define DRIVE_ROM1001_SIZE            0x4000 /* same as ROM8050 and ROM8250 !*/
#define DRIVE_ROM9000_SIZE            0x4000
#define DRIVE_ROM2040_SIZE            0x2000
#define DRIVE_ROM3040_SIZE            0x3000
#define DRIVE_ROM4040_SIZE            0x3000

void driverom_init(void);
void driverom_initialize_traps(struct diskunit_context_s *drive);
int driverom_load(const char *resource_name, uint8_t *drive_rom, unsigned
                  int *loaded, int min, int max, const char *name,
                  unsigned int type, unsigned int *size);
int driverom_load_images(void);
int driverom_snapshot_write(struct snapshot_s *s, const struct drive_s *drive);
int driverom_snapshot_read(struct snapshot_s *s, struct drive_s *drive);

/* 1001 Images have the low and high part combined */
#define DRIVE_ROM1001_NAME          "dos1001-901887+8-01.bin"

/* 2031 Images have the low and high part combined */
#define DRIVE_ROM2031_NAME          "dos2031-901484-03+05.bin"

/* 2040 Images have the low and high parts combined */
#define DRIVE_ROM2040_NAME          "dos2040-901468-06+07.bin"

/* 3040 Images have the three parts combined */
#define DRIVE_ROM3040_NAME          "dos3040-901468-11-13.bin"

/* 4040 Images have the three parts combined */
#define DRIVE_ROM4040_NAME          "dos4040-901468-14-16.bin"

/* 90x0 Images have the low and high parts combined */
#define DRIVE_ROM9000_NAME          "dos9000-300516+7-revC.bin"

/* 154x Images have the low and high part combined */
#define DRIVE_ROM1540_NAME          "dos1540-325302+3-01.bin"
#define DRIVE_ROM1541_NAME          "dos1541-325302-01+901229-05.bin"
#define DRIVE_ROM1541II_NAME        "dos1541ii-251968-03.bin"

#define DRIVE_ROM1551_NAME          "dos1551-318008-01.bin"
#define DRIVE_ROM1570_NAME          "dos1570-315090-01.bin"
#define DRIVE_ROM1571_NAME          "dos1571-310654-05.bin"
#define DRIVE_ROM1571CR_NAME        "dos1571cr-318047-01.bin"
#define DRIVE_ROM1581_NAME          "dos1581-318045-02.bin"

#define DRIVE_ROM2000_NAME          "dos2000-cs-33cc6f.bin"
#define DRIVE_ROM4000_NAME          "dos4000-fd-350022.bin"
#define DRIVE_ROMCMDHD_NAME         "bootromCMDHD-v2-80.bin"

#endif
