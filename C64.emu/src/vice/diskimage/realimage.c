/*
 * realimage.c
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

#include "vice.h"

#include "diskimage.h"
#include "log.h"
#include "machine-bus.h"
#include "realimage.h"
#include "types.h"


static log_t realimage_log = LOG_DEFAULT;


void realimage_media_create(disk_image_t *image)
{
}

void realimage_media_destroy(disk_image_t *image)
{
}

/*-----------------------------------------------------------------------*/

int realimage_open(disk_image_t *image)
{
    return 0;
}

int realimage_close(disk_image_t *image)
{
    return 0;
}

/*-----------------------------------------------------------------------*/

int realimage_read_sector(const disk_image_t *image, BYTE *buf,
                          const disk_addr_t *dadr)
{
    return machine_bus_lib_read_sector(8, dadr->track, dadr->sector, buf);
}

int realimage_write_sector(disk_image_t *image, const BYTE *buf,
                           const disk_addr_t *dadr)
{
    return 0;
}

/*-----------------------------------------------------------------------*/

void realimage_init(void)
{
    realimage_log = log_open("Real Image");
}
