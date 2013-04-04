/*
 * blockdev.h
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

#ifndef VICE_BLOCKDEV_H
#define VICE_BLOCKDEV_H

#include "types.h"

struct disk_image_s;

extern void blockdev_init(void);
extern int blockdev_resources_init(void);
extern int blockdev_cmdline_options_init(void);

extern int blockdev_open(const char *name, unsigned int *read_only);
extern int blockdev_close(void);
extern int blockdev_read_sector(BYTE *buf, unsigned int track,
                                unsigned int sector);
extern int blockdev_write_sector(const BYTE *buf, unsigned int track,
                                 unsigned int sector);

#endif
