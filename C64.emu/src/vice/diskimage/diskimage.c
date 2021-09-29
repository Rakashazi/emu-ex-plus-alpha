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
#include "realimage.h"
#include "types.h"
#include "p64.h"


/** \brief  Number of speed zones for a given floppy
 */
#define SPEED_ZONE_COUNT    4


/** \brief  Log destination for module
 */
static log_t disk_image_log = LOG_DEFAULT;


/*-----------------------------------------------------------------------*/
/* Speed zones */

/** \brief  Determine the speed zone for \a track in \a format
 *
 * The speed zone determines the number of sectors containing in a track for
 * a given image type. Speed zone 0 is closest to the disk center (containing
 * the smallest number of sectors for a speed zone), 3 is farthest away
 * (containing the largest number of sectors for a speed zone).
 *
 * \param[in]   format  disk image type enumerator
 * \param[in]   track   track number
 *
 * \return  speed zone (0-3)
 */

unsigned int disk_image_speed_map(unsigned int format, unsigned int track)
{
    switch (format) {

        /* single sided format, these are straight forward */
        case DISK_IMAGE_TYPE_D67:
        case DISK_IMAGE_TYPE_D64:
#ifdef HAVE_X64_IMAGE
        case DISK_IMAGE_TYPE_X64:
#endif
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_P64:
            return (track < 31) + (track < 25) + (track < 18);
        case DISK_IMAGE_TYPE_D80:
            return (track < 65) + (track < 54) + (track < 40);

        /* double sided formats - these need extra care */
        case DISK_IMAGE_TYPE_D82:
            /* FIXME: what about 80 track D82s? */
            if (track > 77) {
                track -= 77;
            }
            return (track < 65) + (track < 54) + (track < 40);
        case DISK_IMAGE_TYPE_D71:
            /* FIXME: what about 40 track D71s? */
            if (track > 35) {
                track -= 35;
            }
            return (track < 31) + (track < 25) + (track < 18);
        case DISK_IMAGE_TYPE_G71:
            /* FIXME: is this correct? */
            if (track > 42) {
                track -= 42;
            }
            return (track < 31) + (track < 25) + (track < 18);

        default:
            log_message(disk_image_log,
                        "Unknown disk type %u. Cannot calculate zone speed",
                        format);
            break;
    }
    return 0;
}

/*-----------------------------------------------------------------------*/
/* Number of sectors per track */

/** \brief  Sectors per speed zone for D64/D71 images
 */
static const unsigned int sector_map_d64[SPEED_ZONE_COUNT] = {
    17, /* tracks 31-[35-40], 66-70 (D71 only) */
    18, /* track 25-30, 60-65 (D71 only) */
    19, /* tracks 18-24, 53-59 (D71 only)  */
    21  /* tracks 1-17, 36-52 (D71 only) */
};


/** \brief  Sectors per speed zone for D67 images
 */
/* the 2040 tried to squeeze one more sector into speed zone 2 */
static const unsigned int sector_map_d67[SPEED_ZONE_COUNT] = {
    17, /* tracks 31-[35-40} */
    18, /* track 25-30 */
    20, /* tracks 18-24 */
    21  /* tracks 1-17 */
};


/** \brief  Sectors per speed zone for D80/D82 images
 */
static const unsigned int sector_map_d80[SPEED_ZONE_COUNT] = {
    23, /* tracks 65-77, 142-154 (D82 only) */
    25, /* tracks 54-64, 131,141 (D82 only) */
    27, /* tracks 40-53, 117-130 (D82 only) */
    29  /* tracks 1-39, 78-116 (D82 only) */
};


/** \brief  Get number of sectors for \a track in \a format
 *
 * Determines the number of sectors for a given track and disk image format.
 *
 * \param[in]   format  disk image type enumerator
 * \param[in]   track   track number
 *
 * \return  number of sectors or 0 on error
 */
unsigned int disk_image_sector_per_track(unsigned int format,
                                         unsigned int track)
{
    switch (format) {
        case DISK_IMAGE_TYPE_D64:
#ifdef HAVE_X64_IMAGE
        case DISK_IMAGE_TYPE_X64:
#endif
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_P64:
        case DISK_IMAGE_TYPE_D71:
        case DISK_IMAGE_TYPE_G71:
            return sector_map_d64[disk_image_speed_map(format, track)];
        case DISK_IMAGE_TYPE_D67:
            return sector_map_d67[disk_image_speed_map(format, track)];
        case DISK_IMAGE_TYPE_D80:
        case DISK_IMAGE_TYPE_D82:
            return sector_map_d80[disk_image_speed_map(format, track)];
        default:
            log_message(disk_image_log,
                        "Unknown disk type %u.  Cannot calculate sectors per track",
                        format);
    }
    return 0;
}

/*-----------------------------------------------------------------------*/
/* Bytes per track */

/** \brief  Size of a raw (GCR) track in bytes in a D64 image per speed zone
 */
static const unsigned int raw_track_size_d64[SPEED_ZONE_COUNT] = {
    6250, /* (50000 bits) tracks 31-[35-40], 66-70 (D71 only) */
    6666, /* (53333 bits) track 25-30, 60-65 (D71 only) */
    7142, /* (57143 bits) tracks 18-24, 53-59 (D71 only)  */
    7692  /* (61538 bits) tracks 1-17, 36-52 (D71 only) */
};

/** \brief  Size of a raw (GCR) track in bytes in a D67 image per speed zone
 */
/* the 2040 tried to squeeze one more sector into speed zone 2 */
/* FIXME: D67 has 20 sectors per track for speed zone 2,
          D64/D71 has 19 sectors for that speed zone 

          note: the number of GCR bits per track "should" be the same as in D64,
                as that is pretty much defined by the clock and rotation speed.
 */
static const unsigned int raw_track_size_d67[SPEED_ZONE_COUNT] = {
    6250, /* tracks 31-[35-40} */
    6666, /* track 25-30 */
    7142, /* tracks 18-24 */ /* FIXME: is this correct? */
    7692  /* tracks 1-17 */
};

/** \brief  Size of a raw (GCR) track in bytes in a D80 image per speed zone
 */
static const unsigned int raw_track_size_d80[SPEED_ZONE_COUNT] = {
    9375,  /* tracks 65-77, 142-154 (D82 only) */
    10000, /* tracks 54-64, 131,141 (D82 only) */
    10714, /* tracks 40-53, 117-130 (D82 only) */
    11538  /* tracks 1-39, 78-116 (D82 only) */
};


/** \brief  Get the raw (GCR) track size
 *
 * \param[in]   format  disk image format enumerator
 * \param[in]   track   track number
 *
 * \return  number of bytes for \a track
 */
unsigned int disk_image_raw_track_size(unsigned int format,
                                       unsigned int track)
{
    switch (format) {
        case DISK_IMAGE_TYPE_D64:
#ifdef HAVE_X64_IMAGE
        case DISK_IMAGE_TYPE_X64:   /* FIXME: X64 can contain a lot of different
                                              formats, not just D64 (BW) */
#endif
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_P64:
        case DISK_IMAGE_TYPE_D71:
        case DISK_IMAGE_TYPE_G71:
            return raw_track_size_d64[disk_image_speed_map(format, track)];
        case DISK_IMAGE_TYPE_D67:
            return raw_track_size_d67[disk_image_speed_map(format, track)];
        case DISK_IMAGE_TYPE_D80:
        case DISK_IMAGE_TYPE_D82:
            return raw_track_size_d80[disk_image_speed_map(format, track)];
        default:
            log_message(disk_image_log,
                        "Unknown disk type %u.  Cannot calculate raw size of track",
                        format);
    }
    return 1;
}

/*-----------------------------------------------------------------------*/
/* Gap between sectors */

static const unsigned int gap_size_d64[4] = {
    9,  /* (72 bits) tracks 31-[35-40], 66-70 (D71 only) */
    12, /* (96 bits) track 25-30, 60-65 (D71 only) */
    17, /* (136 bits) tracks 18-24, 53-59 (D71 only)  */
    8   /* (64 bits) tracks 1-17, 36-52 (D71 only) */
};

/* the 2040 tried to squeeze one more sector into speed zone 2 */
/* FIXME: D67 has 20 sectors per track for speed zone 2,
 *        D64/D71 has 19 sectors for that speed zone */
static const unsigned int gap_size_d67[4] = {
    9,  /* tracks 31-[35-40} */ 
    12, /* track 25-30 */
    4,  /* tracks 18-24 */ /* FIXME: is this correct? */
    8   /* tracks 1-17 */
};

unsigned int disk_image_gap_size(unsigned int format, unsigned int track)
{
    switch (format) {
        case DISK_IMAGE_TYPE_D64:
#ifdef HAVE_X64_IMAGE
        case DISK_IMAGE_TYPE_X64:   /* FIXME: X64 can contain a lot of different
                                              formats, not just D64 (BW) */
#endif
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_P64:
        case DISK_IMAGE_TYPE_D71:
        case DISK_IMAGE_TYPE_G71:
            return gap_size_d64[disk_image_speed_map(format, track)];
        case DISK_IMAGE_TYPE_D67:
            return gap_size_d67[disk_image_speed_map(format, track)];
        case DISK_IMAGE_TYPE_D80:
        case DISK_IMAGE_TYPE_D82:
            return 25;
        default:
            log_message(disk_image_log,
                        "Unknown disk type %u.  Cannot calculate gap size",
                        format);
    }
    return 1;
}

/*-----------------------------------------------------------------------*/
/* Gap between header and sector */

unsigned int disk_image_header_gap_size(unsigned int format, unsigned int track)
{
    switch (format) {
        case DISK_IMAGE_TYPE_D64:
#ifdef HAVE_X64_IMAGE
        case DISK_IMAGE_TYPE_X64:   /* FIXME: X64 can contain a lot of different
                                              formats, not just D64 (BW) */
#endif
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_P64:
        case DISK_IMAGE_TYPE_D71:
        case DISK_IMAGE_TYPE_G71:
            return 9;
        case DISK_IMAGE_TYPE_D67:
            return 4;               /* FIXME: is this correct ? */
        case DISK_IMAGE_TYPE_D80:
        case DISK_IMAGE_TYPE_D82:
            /* FIXME */
        default:
            log_message(disk_image_log,
                        "Unknown disk type %u.  Cannot calculate header gap size",
                        format);
    }
    return 1;
}

/*-----------------------------------------------------------------------*/
/* length of a SYNC */

/* FIXME: the value of 5 bytes for sync is also hardcoded in gcr.c */
unsigned int disk_image_sync_size(unsigned int format, unsigned int track)
{
    switch (format) {
        case DISK_IMAGE_TYPE_D64:
#ifdef HAVE_X64_IMAGE
        case DISK_IMAGE_TYPE_X64:   /* FIXME: X64 can contain a lot of different
                                              formats, not just D64 (BW) */
#endif
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_P64:
        case DISK_IMAGE_TYPE_D71:
        case DISK_IMAGE_TYPE_G71:
            return 5; /* 40 bits */
        case DISK_IMAGE_TYPE_D67:
            return 5;               /* FIXME: is this correct ? */
        case DISK_IMAGE_TYPE_D80:
        case DISK_IMAGE_TYPE_D82:
            /* FIXME */
        default:
            log_message(disk_image_log,
                        "Unknown disk type %u.  Cannot calculate sync size",
                        format);
    }
    return 1;
}

/*-----------------------------------------------------------------------*/
/* Check out of bound */

/** \brief  Check if a (\a track, \a sector) is inside bounds for \a image
 *
 * \param[in]   image   disk image
 * \param[in]   track   track number
 * \param[in]   sector  sector number
 *
 * \return  bool
 */
int disk_image_check_sector(const disk_image_t *image, unsigned int track,
                            unsigned int sector)
{
    if (image->device == DISK_IMAGE_DEVICE_FS) {
        return fsimage_check_sector(image, track, sector);
    }

    return 0;
}

/*-----------------------------------------------------------------------*/

/** \brief  Get image type identifier string for \a image
 *
 * \param[in]   image   disk image
 *
 * \return  disk image identifier (nul-terminated 3-char string)
 */
static const char *disk_image_type(const disk_image_t *image)
{
    switch (image->type) {
        case DISK_IMAGE_TYPE_D80: return "D80";
        case DISK_IMAGE_TYPE_D82: return "D82";
        case DISK_IMAGE_TYPE_D64: return "D64";
        case DISK_IMAGE_TYPE_D67: return "D67";
        case DISK_IMAGE_TYPE_G64: return "G64";
        case DISK_IMAGE_TYPE_P64: return "P64";
#ifdef HAVE_X64_IMAGE
        case DISK_IMAGE_TYPE_X64: return "X64";
#endif
        case DISK_IMAGE_TYPE_D71: return "D71";
        case DISK_IMAGE_TYPE_G71: return "G71";
        case DISK_IMAGE_TYPE_D81: return "D81";
        case DISK_IMAGE_TYPE_D1M: return "D1M";
        case DISK_IMAGE_TYPE_D2M: return "D2M";
        case DISK_IMAGE_TYPE_D4M: return "D4M";
        case DISK_IMAGE_TYPE_DHD: return "DHD";
        case DISK_IMAGE_TYPE_D90: return "D90";
        default: return NULL;
    }
}


/** \brief  Log attachment of disk image
 *
 * \param[in]   image   disk image attached
 * \param[in]   lognum  (unused)
 * \param[in]   unit    unit the image is attached to
 */
void disk_image_attach_log(const disk_image_t *image, signed int lognum,
                           unsigned int unit, unsigned int drive)
{
    const char *type = disk_image_type(image);

    if (type == NULL) {
        return;
    }

    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            log_verbose("Unit %u drive %u: %s disk image attached: %s.",
                        unit, drive, type, fsimage_name_get(image));
            break;
    }
}


/** \brief  Log detachment of disk image
 *
 * \param[in]   image   disk image detached
 * \param[in]   lognum  (unused)
 * \param[in]   unit    unit the image is detached from
 */
void disk_image_detach_log(const disk_image_t *image, signed int lognum,
                           unsigned int unit, unsigned int drive)
{
    const char *type = disk_image_type(image);

    if (type == NULL) {
        return;
    }

    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            log_verbose("Unit %u drive %u: %s disk image detached: %s.",
                        unit, drive, type, fsimage_name_get(image));
            break;
    }
}
/*-----------------------------------------------------------------------*/

/** \brief  Set disk image name
 *
 * \param[in,out]   image   disk image
 * \param[in]       name    disk image name
 */
void disk_image_fsimage_name_set(disk_image_t *image, const char *name)
{
    fsimage_name_set(image, name);
}


/** \brief  Get disk image name
 *
 * \param[in]   image   disk image
 *
 * \return  disk image name
 */
const char *disk_image_fsimage_name_get(const disk_image_t *image)
{
    return fsimage_name_get(image);
}


/** \brief  Get file descriptor for \a image
 *
 * \param[in]   image   disk image
 *
 * \return  file descriptor
 */
void *disk_image_fsimage_fd_get(const disk_image_t *image)
{
    return fsimage_fd_get(image);
}


int disk_image_fsimage_create(const char *name, unsigned int type)
{
    return fsimage_create(name, type);
}

int disk_image_fsimage_create_dxm(const char *name, const char *diskname, unsigned int type)
{
    return fsimage_create_dxm(name, diskname, type);
}

/*-----------------------------------------------------------------------*/

void disk_image_name_set(disk_image_t *image, const char *name)
{
    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            fsimage_name_set(image, name);
            break;
    }
}

const char *disk_image_name_get(const disk_image_t *image)
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
    disk_image_t *image = lib_malloc(sizeof *image);
    image->p64 = NULL;
    return image;
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
#ifdef HAVE_REALDEVICE
        case DISK_IMAGE_DEVICE_REAL:
            realimage_media_create(image);
            break;
#endif
        default:
            log_error(disk_image_log, "Unknown image device %u.", image->device);
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
#ifdef HAVE_REALDEVICE
        case DISK_IMAGE_DEVICE_REAL:
            realimage_media_destroy(image);
            break;
#endif
        default:
            log_error(disk_image_log, "Unknown image device %u.", image->device);
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
#ifdef HAVE_REALDEVICE
        case DISK_IMAGE_DEVICE_REAL:
            rc = realimage_open(image);
            break;
#endif
        default:
            log_error(disk_image_log, "Unknown image device %u.", image->device);
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
#ifdef HAVE_REALDEVICE
        case DISK_IMAGE_DEVICE_REAL:
            rc = realimage_close(image);
            break;
#endif
        default:
            log_error(disk_image_log, "Unknown image device %u.", image->device);
            rc = -1;
    }

    return rc;
}

/*-----------------------------------------------------------------------*/

int disk_image_read_sector(const disk_image_t *image, uint8_t *buf, const disk_addr_t *dadr)
{
    int rc = 0;

    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            rc = fsimage_read_sector(image, buf, dadr);
            break;
#ifdef HAVE_REALDEVICE
        case DISK_IMAGE_DEVICE_REAL:
            rc = realimage_read_sector(image, buf, dadr);
            break;
#endif
        default:
            log_error(disk_image_log, "Unknown image device %u.", image->device);
            rc = -1;
    }

    return rc;
}

int disk_image_write_sector(disk_image_t *image, const uint8_t *buf, const disk_addr_t *dadr)
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
#ifdef HAVE_REALDEVICE
        case DISK_IMAGE_DEVICE_REAL:
            rc = realimage_write_sector(image, buf, dadr);
            break;
#endif
        default:
            log_error(disk_image_log, "Unknow image device %u.", image->device);
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
        case DISK_IMAGE_TYPE_G71:
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
        case DISK_IMAGE_TYPE_G71:
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
#ifdef HAVE_REALDEVICE
    realimage_init();
#endif
}

int disk_image_resources_init(void)
{
    return 0;
}

void disk_image_resources_shutdown(void)
{
}

int disk_image_cmdline_options_init(void)
{
    return 0;
}

/*-----------------------------------------------------------------------*/

uint32_t disk_image_size(const disk_image_t *image)
{
    switch (image->device) {
        case DISK_IMAGE_DEVICE_FS:
            return fsimage_size(image);
#ifdef HAVE_REALDEVICE
        case DISK_IMAGE_DEVICE_REAL:
            break;
#endif
        default:
            log_error(disk_image_log, "Unknown image device %u.", image->device);
    }
    return 0;
}
