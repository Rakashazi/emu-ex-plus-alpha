/*
 * rawimage.c
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

#include <stdio.h>
#include <stdlib.h>

#include "archdep.h"
#include "blockdev.h"
#include "cmdline.h"
#include "diskimage.h"
#include "lib.h"
#include "log.h"
#include "rawimage.h"
#include "resources.h"
#include "translate.h"
#include "types.h"
#include "util.h"


static log_t rawimage_log = LOG_DEFAULT;

static char *raw_drive_driver = NULL;


void rawimage_name_set(disk_image_t *image, char *name)
{
    rawimage_t *rawimage;

    rawimage = image->media.rawimage;

    rawimage->name = name;
}

char *rawimage_name_get(const disk_image_t *image)
{
    rawimage_t *rawimage;

    rawimage = image->media.rawimage;

    return rawimage->name;
}

void rawimage_driver_name_set(disk_image_t *image)
{
    rawimage_name_set(image, lib_stralloc(raw_drive_driver));
}

/*-----------------------------------------------------------------------*/

void rawimage_media_create(disk_image_t *image)
{
    rawimage_t *rawimage;

    rawimage = lib_calloc(1, sizeof(rawimage_t));

    image->media.rawimage = rawimage;
}

void rawimage_media_destroy(disk_image_t *image)
{
    rawimage_t *rawimage;

    rawimage = image->media.rawimage;

    lib_free(rawimage->name);

    lib_free(rawimage);
}

/*-----------------------------------------------------------------------*/

int rawimage_open(disk_image_t *image)
{
    char *name;

    image->type = DISK_IMAGE_TYPE_D81;
    image->tracks = 80;

    name = rawimage_name_get(image);

    blockdev_open(name, &(image->read_only));

    return 0;
}

int rawimage_close(disk_image_t *image)
{
    blockdev_close();

    return 0;
}

/*-----------------------------------------------------------------------*/

int rawimage_read_sector(const disk_image_t *image, BYTE *buf,
                         const disk_addr_t *dadr)
{
    return blockdev_read_sector(buf, dadr->track, dadr->sector);
}

int rawimage_write_sector(disk_image_t *image, const BYTE *buf,
                          const disk_addr_t *dadr)
{
    return blockdev_write_sector(buf, dadr->track, dadr->sector);
}

/*-----------------------------------------------------------------------*/

void rawimage_init(void)
{
    rawimage_log = log_open("Raw Image");

    blockdev_init();
}

/*-----------------------------------------------------------------------*/

static int set_raw_drive_driver(const char *val, void *param)
{
    if (util_string_set(&raw_drive_driver, val)) {
        return 0;
    }

    return 0;
}

static const resource_string_t resources_string[] = {
    { "RawDriveDriver", ARCHDEP_RAWDRIVE_DEFAULT, RES_EVENT_NO, NULL,
      &raw_drive_driver, set_raw_drive_driver, NULL },
    { NULL }
};

int rawimage_resources_init(void)
{
    return resources_register_string(resources_string) | blockdev_resources_init();
}

void rawimage_resources_shutdown(void)
{
    lib_free(raw_drive_driver);
}

/*-----------------------------------------------------------------------*/

static const cmdline_option_t cmdline_options[] =
{
    { "-rawdrive", SET_RESOURCE, 1,
      NULL, NULL, "RawDriveDriver", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SET_RAW_DRIVE_DEVICE,
      NULL, NULL },
    { NULL }
};

int rawimage_cmdline_options_init()
{
    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    return blockdev_cmdline_options_init();
}
