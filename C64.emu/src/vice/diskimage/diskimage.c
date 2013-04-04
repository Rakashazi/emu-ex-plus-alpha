/*
 * diskimage.c - Common low-level disk image access.
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

/* #define DEBUG_DISKIMAGE */

#ifdef DEBUG_DISKIMAGE
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diskconstants.h"
#include "diskimage.h"
#include "fsimage-check.h"
#include "fsimage-create.h"
#include "fsimage-dxx.h"
#include "fsimage-gcr.h"
#include "fsimage-p64.h"
#include "fsimage.h"
#include "lib.h"
#include "log.h"
#include "rawimage.h"
#include "realimage.h"
#include "types.h"
#include "p64.h"

static log_t disk_image_log = LOG_DEFAULT;


/*-----------------------------------------------------------------------*/
/* Speed zones */

unsigned int disk_image_speed_map(unsigned int format, unsigned int track)
{
    switch (format) {
        case DISK_IMAGE_TYPE_D71:
            if (track > 35) {
                track -= 35;
            }
        /* fall through */
        case DISK_IMAGE_TYPE_D67:
        case DISK_IMAGE_TYPE_D64:
        case DISK_IMAGE_TYPE_X64:
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_P64:
            return (track < 31) + (track < 25) + (track < 18);
        case DISK_IMAGE_TYPE_D82:
            if (track > 77) {
                track -= 77;
            }
        /* fall through */
        case DISK_IMAGE_TYPE_D80:
            return (track < 65) + (track < 54) + (track < 40);
        default:
            log_message(disk_image_log,
                        "Unknown disk type %i. Cannot calculate zone speed",
                        format);
    }
    return 0;
}

/*-----------------------------------------------------------------------*/
/* Number of sectors per track */

static const unsigned int sector_map_d64[4] = { 17, 18, 19, 21 };
static const unsigned int sector_map_d67[4] = { 17, 18, 20, 21 };
static const unsigned int sector_map_d80[4] = { 23, 25, 27, 29 };

unsigned int disk_image_sector_per_track(unsigned int format,
                                         unsigned int track)
{
    switch (format) {
        case DISK_IMAGE_TYPE_D64:
        case DISK_IMAGE_TYPE_X64:
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_P64:
        case DISK_IMAGE_TYPE_D71:
            return sector_map_d64[disk_image_speed_map(format, track)];
        case DISK_IMAGE_TYPE_D67:
            return sector_map_d67[disk_image_speed_map(format, track)];
        case DISK_IMAGE_TYPE_D80:
        case DISK_IMAGE_TYPE_D82:
            return sector_map_d80[disk_image_speed_map(format, track)];
        default:
            log_message(disk_image_log,
                        "Unknown disk type %i.  Cannot calculate sectors per track",
                        format);
    }
    return 0;
}

/*-----------------------------------------------------------------------*/
/* Bytes per track */

static const unsigned int raw_track_size_d64[4] = {
    6250, 6666, 7142, 7692
};
static const unsigned int raw_track_size_d80[4] = {
    9375, 10000, 10714, 11538
};

unsigned int disk_image_raw_track_size(unsigned int format,
                                       unsigned int track)
{
    switch (format) {
        case DISK_IMAGE_TYPE_D64:
        case DISK_IMAGE_TYPE_X64:
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_P64:
        case DISK_IMAGE_TYPE_D71:
        case DISK_IMAGE_TYPE_D67:
            return raw_track_size_d64[disk_image_speed_map(format, track)];
        case DISK_IMAGE_TYPE_D80:
        case DISK_IMAGE_TYPE_D82:
            return raw_track_size_d80[disk_image_speed_map(format, track)];
        default:
            log_message(disk_image_log,
                        "Unknown disk type %i.  Cannot calculate raw size of track",
                        format);
    }
    return 1;
}

/*-----------------------------------------------------------------------*/
/* Gap between sectors */

static const unsigned int gap_size_d64[4] = {
    9, 12, 17, 8
};

unsigned int disk_image_gap_size(unsigned int format, unsigned int track)
{
    switch (format) {
        case DISK_IMAGE_TYPE_D64:
        case DISK_IMAGE_TYPE_X64:
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_P64:
        case DISK_IMAGE_TYPE_D71:
        case DISK_IMAGE_TYPE_D67:
            return gap_size_d64[disk_image_speed_map(format, track)];
        case DISK_IMAGE_TYPE_D80:
        case DISK_IMAGE_TYPE_D82:
            return 25;
        default:
            log_message(disk_image_log,
                        "Unknown disk type %i.  Cannot calculate gap size",
                        format);
    }
    return 1;
}

/*-----------------------------------------------------------------------*/
/* Check out of bound */
int disk_image_check_sector(const disk_image_t *image, unsigned int track,
                            unsigned int sector)
{
    if (image->device == DISK_IMAGE_DEVICE_FS) {
        return fsimage_check_sector(image, track, sector);
    }

    return 0;
}

/*-----------------------------------------------------------------------*/

static const char *disk_image_type(const disk_image_t *image)
{
    switch (image->type) {
        case DISK_IMAGE_TYPE_D80: return "D80";
        case DISK_IMAGE_TYPE_D82: return "D82";
        case DISK_IMAGE_TYPE_D64: return "D64";
        case DISK_IMAGE_TYPE_D67: return "D67";
        case DISK_IMAGE_TYPE_G64: return "G64";
        case DISK_IMAGE_TYPE_P64: return "P64";
        case DISK_IMAGE_TYPE_X64: return "X64";
        case DISK_IMAGE_TYPE_D71: return "D71";
        case DISK_IMAGE_TYPE_D81: return "D81";
        case DISK_IMAGE_TYPE_D1M: return "D1M";
        case DISK_IMAGE_TYPE_D2M: return "D2M";
        case DISK_IMAGE_TYPE_D4M: return "D4M";
        default: return NULL;
    }
}

void disk_image_attach_log(const disk_image_t *image, signed int lognum,
                           unsigned int unit)
{
    const char *type = disk_image_type(image);

    if (type == NULL) {
        return;
    }

    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            log_verbose("Unit %d: %s disk image attached: %s.",
                        unit, type, fsimage_name_get(image));
            break;
#ifdef HAVE_RAWDRIVE
        case DISK_IMAGE_DEVICE_RAW:
            log_verbose("Unit %d: %s disk attached (drive: %s).",
                        unit, type, rawimage_name_get(image));
            break;
#endif
    }
}

void disk_image_detach_log(const disk_image_t *image, signed int lognum,
                           unsigned int unit)
{
    const char *type = disk_image_type(image);

    if (type == NULL) {
        return;
    }

    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            log_verbose("Unit %d: %s disk image detached: %s.",
                        unit, type, fsimage_name_get(image));
            break;
#ifdef HAVE_RAWDRIVE
        case DISK_IMAGE_DEVICE_RAW:
            log_verbose("Unit %d: %s disk detached (drive: %s).",
                        unit, type, rawimage_name_get(image));
            break;
#endif
    }
}
/*-----------------------------------------------------------------------*/

void disk_image_fsimage_name_set(disk_image_t *image, char *name)
{
    fsimage_name_set(image, name);
}

char *disk_image_fsimage_name_get(const disk_image_t *image)
{
    return fsimage_name_get(image);
}

void *disk_image_fsimage_fd_get(const disk_image_t *image)
{
    return fsimage_fd_get(image);
}

int disk_image_fsimage_create(const char *name, unsigned int type)
{
    return fsimage_create(name, type);
}

/*-----------------------------------------------------------------------*/

void disk_image_rawimage_name_set(disk_image_t *image, char *name)
{
#ifdef HAVE_RAWDRIVE
    rawimage_name_set(image, name);
#endif
}

void disk_image_rawimage_driver_name_set(disk_image_t *image)
{
#ifdef HAVE_RAWDRIVE
    rawimage_driver_name_set(image);
#endif
}

/*-----------------------------------------------------------------------*/

void disk_image_name_set(disk_image_t *image, char *name)
{
    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            fsimage_name_set(image, name);
            break;
#ifdef HAVE_RAWDRIVE
        case DISK_IMAGE_DEVICE_RAW:
            rawimage_name_set(image, name);
            break;
#endif
    }
}

char *disk_image_name_get(const disk_image_t *image)
{
    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            return fsimage_name_get(image);
            break;
    }

    return NULL;
}

/*-----------------------------------------------------------------------*/

disk_image_t *disk_image_create(void)
{
    return (disk_image_t *)lib_malloc(sizeof(disk_image_t));
}

void disk_image_destroy(disk_image_t *image)
{
    lib_free(image);
}

/*-----------------------------------------------------------------------*/

void disk_image_media_create(disk_image_t *image)
{
    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            fsimage_media_create(image);
            break;
#ifdef HAVE_OPENCBM
        case DISK_IMAGE_DEVICE_REAL:
            realimage_media_create(image);
            break;
#endif
#ifdef HAVE_RAWDRIVE
        case DISK_IMAGE_DEVICE_RAW:
            rawimage_media_create(image);
            break;
#endif
        default:
            log_error(disk_image_log, "Unknown image device %i.", image->device);
    }
}

void disk_image_media_destroy(disk_image_t *image)
{
    if (image == NULL) {
        return;
    }

    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            fsimage_media_destroy(image);
            break;
#ifdef HAVE_OPENCBM
        case DISK_IMAGE_DEVICE_REAL:
            realimage_media_destroy(image);
            break;
#endif
#ifdef HAVE_RAWDRIVE
        case DISK_IMAGE_DEVICE_RAW:
            rawimage_media_destroy(image);
            break;
#endif
        default:
            log_error(disk_image_log, "Unknown image device %i.", image->device);
    }
}

/*-----------------------------------------------------------------------*/

int disk_image_open(disk_image_t *image)
{
    int rc = 0;

    DBG(("disk_image_open"));

    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            rc = fsimage_open(image);
            break;
#ifdef HAVE_OPENCBM
        case DISK_IMAGE_DEVICE_REAL:
            rc = realimage_open(image);
            break;
#endif
#ifdef HAVE_RAWDRIVE
        case DISK_IMAGE_DEVICE_RAW:
            rc = rawimage_open(image);
            break;
#endif
        default:
            log_error(disk_image_log, "Unknown image device %i.", image->device);
            rc = -1;
    }

    return rc;
}


int disk_image_close(disk_image_t *image)
{
    int rc = 0;

    if (image == NULL) {
        return 0;
    }

    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            rc = fsimage_close(image);
            break;
#ifdef HAVE_OPENCBM
        case DISK_IMAGE_DEVICE_REAL:
            rc = realimage_close(image);
            break;
#endif
#ifdef HAVE_RAWDRIVE
        case DISK_IMAGE_DEVICE_RAW:
            rc = rawimage_close(image);
            break;
#endif
        default:
            log_error(disk_image_log, "Unknown image device %i.", image->device);
            rc = -1;
    }

    return rc;
}

/*-----------------------------------------------------------------------*/

int disk_image_read_sector(const disk_image_t *image, BYTE *buf, const disk_addr_t *dadr)
{
    int rc = 0;

    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            rc = fsimage_read_sector(image, buf, dadr);
            break;
#ifdef HAVE_OPENCBM
        case DISK_IMAGE_DEVICE_REAL:
            rc = realimage_read_sector(image, buf, dadr);
            break;
#endif
#ifdef HAVE_RAWDRIVE
        case DISK_IMAGE_DEVICE_RAW:
            rc = rawimage_read_sector(image, buf, dadr);
            break;
#endif
        default:
            log_error(disk_image_log, "Unknown image device %i.", image->device);
            rc = -1;
    }

    return rc;
}

int disk_image_write_sector(disk_image_t *image, const BYTE *buf, const disk_addr_t *dadr)
{
    int rc = 0;

    if (image->read_only != 0) {
        log_error(disk_image_log, "Attempt to write to read-only disk image.");
        return -1;
    }

    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            rc = fsimage_write_sector(image, buf, dadr);
            break;
#ifdef HAVE_OPENCBM
        case DISK_IMAGE_DEVICE_REAL:
            rc = realimage_write_sector(image, buf, dadr);
            break;
#endif
#ifdef HAVE_RAWDRIVE
        case DISK_IMAGE_DEVICE_RAW:
            rc = rawimage_write_sector(image, buf, dadr);
            break;
#endif
        default:
            log_error(disk_image_log, "Unknow image device %i.", image->device);
            rc = -1;
    }

    return rc;
}

/*-----------------------------------------------------------------------*/

int disk_image_write_half_track(disk_image_t *image, unsigned int half_track,
                                const struct disk_track_s *raw)
{
    if (half_track > image->max_half_tracks) {
        log_error(disk_image_log, "Attempt to write beyond extension limit of disk image.");
        return -1;
    }
    if (image->read_only != 0) {
        log_error(disk_image_log, "Attempt to write to read-only disk image.");
        return -1;
    }

    switch (image->type) {
        case DISK_IMAGE_TYPE_P64:
            return fsimage_p64_write_half_track(image, half_track, raw);
        case DISK_IMAGE_TYPE_G64:
            return fsimage_gcr_write_half_track(image, half_track, raw);
        default:
            return fsimage_dxx_write_half_track(image, half_track, raw);
    }
}

int disk_image_read_image(const disk_image_t *image)
{
    switch (image->type) {
        case DISK_IMAGE_TYPE_P64:
            return fsimage_read_p64_image(image);
        case DISK_IMAGE_TYPE_G64:
            return fsimage_read_gcr_image(image);
        default:
            return fsimage_read_dxx_image(image);
    }
}

int disk_image_write_p64_image(const disk_image_t *image)
{
    return fsimage_write_p64_image(image);
}

/*-----------------------------------------------------------------------*/
/* Initialization.  */

void disk_image_init(void)
{
    disk_image_log = log_open("Disk Access");
    fsimage_create_init();
    fsimage_init();
#ifdef HAVE_OPENCBM
    realimage_init();
#endif
#ifdef HAVE_RAWDRIVE
    rawimage_init();
#endif
}

int disk_image_resources_init(void)
{
#ifdef HAVE_RAWDRIVE
    if (rawimage_resources_init() < 0) {
        return -1;
    }
#endif
    return 0;
}

void disk_image_resources_shutdown(void)
{
#ifdef HAVE_RAWDRIVE
    rawimage_resources_shutdown();
#endif
}

int disk_image_cmdline_options_init(void)
{
#ifdef HAVE_RAWDRIVE
    if (rawimage_cmdline_options_init() < 0) {
        return -1;
    }
#endif
    return 0;
}
