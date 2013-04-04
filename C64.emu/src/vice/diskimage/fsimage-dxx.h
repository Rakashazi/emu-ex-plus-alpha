/*
 * fsimage-dxx.h
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
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

#ifndef VICE_FSIMAGE_DXX_H
#define VICE_FSIMAGE_DXX_H

#include "types.h"

struct disk_image_s;
struct disk_track_s;
struct disk_addr_s;

extern void fsimage_dxx_init(void);

extern int fsimage_read_dxx_image(const disk_image_t *image);

extern int fsimage_dxx_write_half_track(disk_image_t *image, unsigned int half_track,
                                        const struct disk_track_s *raw);
extern int fsimage_dxx_read_sector(const struct disk_image_s *image, BYTE *buf,
                                   const struct disk_addr_s *dadr);
extern int fsimage_dxx_write_sector(struct disk_image_s *image, const BYTE *buf,
                                    const struct disk_addr_s *dadr);
#endif
