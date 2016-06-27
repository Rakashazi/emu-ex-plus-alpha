/*
 * driveimage.h
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

#ifndef VICE_DRIVEIMAGE_H
#define VICE_DRIVEIMAGE_H

struct disk_image_s;
struct drive_s;

extern void drive_image_init(void);
extern void drive_image_init_track_size_d64(struct drive_s *drive);

extern int drive_image_attach(struct disk_image_s *image, unsigned int unit);
extern int drive_image_detach(struct disk_image_s *image, unsigned int unit);

#endif
