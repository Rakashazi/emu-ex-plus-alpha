/*
 * fsimage.c
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
#include "diskconstants.h"
#include "diskimage.h"
#include "fsimage-dxx.h"
#include "fsimage-gcr.h"
#include "fsimage-p64.h"
#include "fsimage-probe.h"
#include "fsimage.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "zfile.h"
#include "util.h"
#include "cbmdos.h"


static log_t fsimage_log = LOG_DEFAULT;


/** \brief  Set image name
 *
 * \param[in]   name    image name
 */
void fsimage_name_set(disk_image_t *image, const char *name)
{
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    fsimage->name = lib_stralloc(name);
}


/** \brief  Get image name
 *
 * \param[in]   image   disk image
 *
 * \return  image name
 */
const char *fsimage_name_get(const disk_image_t *image)
{
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    return fsimage->name;
}


/** \brief  Get file descriptor for \a image
 *
 * \param[in]   image   disk image
 *
 * \return  file descriptor
 *
 * XXX: \a image should not be const, since we return the a member we later use
 *      to manipulate the image.
 */
void *fsimage_fd_get(const disk_image_t *image)
{
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    return (void *)(fsimage->fd);
}

/*-----------------------------------------------------------------------*/

void fsimage_media_create(disk_image_t *image)
{
    fsimage_t *fsimage;

    fsimage = lib_calloc(1, sizeof(fsimage_t));

    image->media.fsimage = fsimage;
}

void fsimage_media_destroy(disk_image_t *image)
{
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    if (fsimage->fd) {
        fsimage_close(image);
    }
    lib_free(fsimage->name);
    lib_free(fsimage);
}

/*-----------------------------------------------------------------------*/

int fsimage_open(disk_image_t *image)
{
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;
    fsimage->error_info.map = NULL;

    if (image->read_only) {
        fsimage->fd = zfile_fopen(fsimage->name, MODE_READ);
    } else {
        fsimage->fd = zfile_fopen(fsimage->name, MODE_READ_WRITE);

        /* If we cannot open the image read/write, try to open it read only. */
        if (fsimage->fd == NULL) {
            fsimage->fd = zfile_fopen(fsimage->name, MODE_READ);
            image->read_only = 1;
        }
    }

    if (fsimage->fd == NULL) {
        log_error(fsimage_log, "Cannot open file `%s'.", fsimage->name);
        return -1;
    }

    if (fsimage_probe(image) == 0) {
        return 0;
    }

    log_message(fsimage_log, "Unknown disk image `%s'.", fsimage->name);
    fsimage_close(image);
    return -1;
}

int fsimage_close(disk_image_t *image)
{
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    if (fsimage->fd == NULL) {
        log_error(fsimage_log, "Cannot close file `%s'.", fsimage->name);
        return -1;
    }

/*   if (image->type == DISK_IMAGE_TYPE_P64) {
        fsimage_write_p64_image(image);
    }*/

    if (fsimage->error_info.map) {
        lib_free(fsimage->error_info.map);
        fsimage->error_info.map = NULL;
    }
    zfile_fclose(fsimage->fd);
    fsimage->fd = NULL;

    return 0;
}

/*-----------------------------------------------------------------------*/

int fsimage_read_sector(const disk_image_t *image, BYTE *buf, const disk_addr_t *dadr)
{
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    if (fsimage->fd == NULL) {
        log_error(fsimage_log, "Attempt to read without disk image.");
        return CBMDOS_IPE_NOT_READY;
    }

    switch (image->type) {
        case DISK_IMAGE_TYPE_D64:
        case DISK_IMAGE_TYPE_D67:
        case DISK_IMAGE_TYPE_D71:
        case DISK_IMAGE_TYPE_D81:
        case DISK_IMAGE_TYPE_D80:
        case DISK_IMAGE_TYPE_D82:
        case DISK_IMAGE_TYPE_X64:
        case DISK_IMAGE_TYPE_D1M:
        case DISK_IMAGE_TYPE_D2M:
        case DISK_IMAGE_TYPE_D4M:
            return fsimage_dxx_read_sector(image, buf, dadr);
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_G71:
            return fsimage_gcr_read_sector(image, buf, dadr);
        case DISK_IMAGE_TYPE_P64:
            return fsimage_p64_read_sector(image, buf, dadr);
        default:
            log_error(fsimage_log,
                      "Unknown disk image type %i.  Cannot read sector.",
                      image->type);
            return CBMDOS_IPE_NOT_READY;
    }
}

int fsimage_write_sector(disk_image_t *image, const BYTE *buf,
                         const disk_addr_t *dadr)
{
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    if (fsimage->fd == NULL) {
        log_error(fsimage_log, "Attempt to write without disk image.");
        return -1;
    }

    switch (image->type) {
        case DISK_IMAGE_TYPE_D64:
        case DISK_IMAGE_TYPE_D67:
        case DISK_IMAGE_TYPE_D71:
        case DISK_IMAGE_TYPE_D81:
        case DISK_IMAGE_TYPE_D80:
        case DISK_IMAGE_TYPE_D82:
        case DISK_IMAGE_TYPE_X64:
        case DISK_IMAGE_TYPE_D1M:
        case DISK_IMAGE_TYPE_D2M:
        case DISK_IMAGE_TYPE_D4M:
            if (fsimage_dxx_write_sector(image, buf, dadr) < 0) {
                return -1;
            }
            break;
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_G71:
            if (fsimage_gcr_write_sector(image, buf, dadr) < 0) {
                return -1;
            }
            break;
        case DISK_IMAGE_TYPE_P64:
            if (fsimage_p64_write_sector(image, buf, dadr) < 0) {
                return -1;
            }
            break;
        default:
            log_error(fsimage_log, "Unknown disk image.  Cannot write sector.");
            return -1;
    }
    return 0;
}

/*-----------------------------------------------------------------------*/

void fsimage_init(void)
{
    fsimage_log = log_open("Filesystem Image");
    fsimage_dxx_init();
    fsimage_gcr_init();
    fsimage_p64_init();
    fsimage_probe_init();
}
