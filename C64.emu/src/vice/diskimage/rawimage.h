/*
 * rawimage.h
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

#ifndef VICE_RAWIMAGE_H
#define VICE_RAWIMAGE_H

#include "types.h"

struct disk_image_s;
struct disk_addr_s;

typedef struct rawimage_s {
    char *name;
    unsigned int drivetype;
} rawimage_t;


extern void rawimage_init(void);
extern int rawimage_resources_init(void);
extern int rawimage_cmdline_options_init(void);
extern void rawimage_resources_shutdown(void);

extern void rawimage_name_set(struct disk_image_s *image, const char *name);
extern const char *rawimage_name_get(const struct disk_image_s *image);
extern void rawimage_driver_name_set(struct disk_image_s *image);

extern int rawimage_open(struct disk_image_s *image);
extern int rawimage_close(struct disk_image_s *image);
extern int rawimage_read_sector(const struct disk_image_s *image, BYTE *buf,
                                const struct disk_addr_s *dadr);
extern int rawimage_write_sector(struct disk_image_s *image, const BYTE *buf,
                                 const struct disk_addr_s *dadr);
extern void rawimage_media_create(struct disk_image_s *image);
extern void rawimage_media_destroy(struct disk_image_s *image);

#endif
