/*
 * fsimage-p64.c
 *
 * Written by
 *  Benjamin 'BeRo' Rosseaux <benjamin@rosseaux.com>
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
#include <string.h>

#include "diskconstants.h"
#include "diskimage.h"
#include "fsimage-p64.h"
#include "fsimage.h"
#include "cbmdos.h"
#include "gcr.h"
#include "log.h"
#include "lib.h"
#include "types.h"
#include "util.h"
#include "p64.h"

static log_t fsimage_p64_log = LOG_ERR;

/*-----------------------------------------------------------------------*/
/* Intial P64 buffer setup.  */

int fsimage_read_p64_image(const disk_image_t *image)
{
    TP64MemoryStream P64MemoryStreamInstance;
    PP64Image P64Image = (void*)image->p64;
    int lSize, rc;
    void *buffer;

    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    lSize = util_file_length(fsimage->fd);
    buffer = lib_malloc(lSize);
    if (util_fpread(fsimage->fd, buffer, lSize, 0) < 0) {
        lib_free(buffer);
        log_error(fsimage_p64_log, "Could not read P64 disk image.");
        return -1;
    }

    /*num_tracks = image->tracks;*/

    P64MemoryStreamCreate(&P64MemoryStreamInstance);
    P64MemoryStreamWrite(&P64MemoryStreamInstance, buffer, lSize);
    P64MemoryStreamSeek(&P64MemoryStreamInstance, 0);
    if (P64ImageReadFromStream(P64Image, &P64MemoryStreamInstance)) {
        rc = 0;
    } else {
        rc = -1;
        log_error(fsimage_p64_log, "Could not read P64 disk image stream.");
    }
    P64MemoryStreamDestroy(&P64MemoryStreamInstance);

    lib_free(buffer);

    return rc;
}

int fsimage_write_p64_image(const disk_image_t *image)
{
    TP64MemoryStream P64MemoryStreamInstance;
    PP64Image P64Image = (void*)image->p64;
    int rc;

    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    P64MemoryStreamCreate(&P64MemoryStreamInstance);
    P64MemoryStreamClear(&P64MemoryStreamInstance);
    if (P64ImageWriteToStream(P64Image, &P64MemoryStreamInstance)) {
        if (util_fpwrite(fsimage->fd, P64MemoryStreamInstance.Data, P64MemoryStreamInstance.Size, 0) < 0) {
            rc = -1;
            log_error(fsimage_p64_log, "Could not write P64 disk image.");
        } else {
            fflush(fsimage->fd);
            rc = 0;
        }
    } else {
        rc = -1;
        log_error(fsimage_p64_log, "Could not write P64 disk image stream.");
    }
    P64MemoryStreamDestroy(&P64MemoryStreamInstance);

    return rc;
}

/*-----------------------------------------------------------------------*/
/* Read an entire P64 track from the disk image.  */

int fsimage_p64_read_half_track(const disk_image_t *image, unsigned int half_track,
                                disk_track_t *raw)
{
    PP64Image P64Image = (void*)image->p64;
    unsigned int track;

    raw->data = NULL;
    raw->size = 0;
    if (P64Image == NULL) {
        log_error(fsimage_p64_log, "P64 image not loaded.");
        return -1;
    }

    if (half_track > 84) {
        log_error(fsimage_p64_log, "Half track %i out of bounds.  Cannot read P64 track.", half_track);
        return -1;
    }

    track = half_track / 2;

    raw->data = lib_malloc(NUM_MAX_MEM_BYTES_TRACK);
    raw->size = (P64PulseStreamConvertToGCRWithLogic(&P64Image->PulseStreams[half_track], (void*)raw->data, NUM_MAX_MEM_BYTES_TRACK, disk_image_speed_map(image->type, track)) + 7) >> 3;

    if (raw->size < 1) {
        raw->size = disk_image_raw_track_size(image->type, track);
        memset(raw->data, 0x55, raw->size);
    }

    return 0;
}

static int fsimage_p64_read_track(const disk_image_t *image, unsigned int track,
                                  disk_track_t *raw)
{
    return fsimage_p64_read_half_track(image, track << 1, raw);
}

/*-----------------------------------------------------------------------*/
/* Write an entire P64 track to the disk image.  */

int fsimage_p64_write_half_track(disk_image_t *image, unsigned int half_track,
                                 const disk_track_t *raw)
{
    PP64Image P64Image = (void*)image->p64;

    if (P64Image == NULL) {
        log_error(fsimage_p64_log, "P64 image not loaded.");
        return -1;
    }

    if (half_track > 84) {
        log_error(fsimage_p64_log, "Half track %i out of bounds.  Cannot write P64 track.", half_track);
        return -1;
    }
    if (raw->data == NULL) {
        return 0;
    }

    P64PulseStreamConvertFromGCR(&P64Image->PulseStreams[half_track], (void*)raw->data, raw->size << 3);

    return fsimage_write_p64_image(image);
}

static int fsimage_p64_write_track(disk_image_t *image, unsigned int track,
                                   int gcr_track_size, BYTE *gcr_track_start_ptr)
{
    PP64Image P64Image = (void*)image->p64;

    if (P64Image == NULL) {
        log_error(fsimage_p64_log, "P64 image not loaded.");
        return -1;
    }

    if (track > 42) {
        log_error(fsimage_p64_log, "Track %i out of bounds.  Cannot write P64 track.", track);
        return -1;
    }

    P64PulseStreamConvertFromGCR(&P64Image->PulseStreams[track << 1], (void*)gcr_track_start_ptr, gcr_track_size << 3);

    return fsimage_write_p64_image(image);
}

/*-----------------------------------------------------------------------*/
/* Read a sector from the P64 disk image.  */

int fsimage_p64_read_sector(const disk_image_t *image, BYTE *buf,
                            const disk_addr_t *dadr)
{
    fdc_err_t rf;
    disk_track_t raw;

    if (dadr->track > 42) {
        log_error(fsimage_p64_log, "Track %i out of bounds.  Cannot read P64 track.", dadr->track);
        return -1;
    }

    if (fsimage_p64_read_track(image, dadr->track, &raw) < 0) {
        return -1;
    }
    if (raw.data == NULL) {
        return CBMDOS_IPE_NOT_READY;
    }

    rf = gcr_read_sector(&raw, buf, (BYTE)dadr->sector);
    lib_free(raw.data);
    if (rf != CBMDOS_FDC_ERR_OK) {
        log_error(fsimage_p64_log, "Cannot find track: %i sector: %i within P64 image.", dadr->track, dadr->sector);
        switch (rf) {
            case CBMDOS_FDC_ERR_HEADER:
                return CBMDOS_IPE_READ_ERROR_BNF; /* 20 */
            case CBMDOS_FDC_ERR_SYNC:
                return CBMDOS_IPE_READ_ERROR_SYNC; /* 21 */
            case CBMDOS_FDC_ERR_NOBLOCK:
                return CBMDOS_IPE_READ_ERROR_DATA; /* 22 */
            case CBMDOS_FDC_ERR_DCHECK:
                return CBMDOS_IPE_READ_ERROR_CHK; /* 23 */
            case CBMDOS_FDC_ERR_VERIFY:
                return CBMDOS_IPE_WRITE_ERROR_VER; /* 25 */
            case CBMDOS_FDC_ERR_WPROT:
                return CBMDOS_IPE_WRITE_PROTECT_ON; /* 26 */
            case CBMDOS_FDC_ERR_HCHECK:
                return CBMDOS_IPE_READ_ERROR_BCHK; /* 27 */
            case CBMDOS_FDC_ERR_BLENGTH:
                return CBMDOS_IPE_WRITE_ERROR_BIG; /* 28 */
            case CBMDOS_FDC_ERR_ID:
                return CBMDOS_IPE_DISK_ID_MISMATCH; /* 29 */
            case CBMDOS_FDC_ERR_DRIVE:
                return CBMDOS_IPE_NOT_READY;    /* 74 */
            case CBMDOS_FDC_ERR_DECODE:
                return CBMDOS_IPE_READ_ERROR_GCR; /* 24 */
            default:
                return CBMDOS_IPE_NOT_READY;
        }
    }
    return CBMDOS_IPE_OK;
}


/*-----------------------------------------------------------------------*/
/* Write a sector to the P64 disk image.  */

int fsimage_p64_write_sector(disk_image_t *image, const BYTE *buf,
                             const disk_addr_t *dadr)
{
    disk_track_t raw;

    if (dadr->track > 42) {
        log_error(fsimage_p64_log, "Track %i out of bounds.  Cannot write P64 sector", dadr->track);
        return -1;
    }

    if (fsimage_p64_read_track(image, dadr->track, &raw) < 0
        || raw.data == NULL) {
        log_error(fsimage_p64_log, "Cannot read track %i from P64 image.", dadr->track);
        return -1;
    }

    if (gcr_write_sector(&raw, buf, (BYTE)dadr->sector) != CBMDOS_FDC_ERR_OK) {
        log_error(fsimage_p64_log, "Could not find track %i sector %i in disk image", dadr->track, dadr->sector);
        lib_free(raw.data);
        return -1;
    }

    if (fsimage_p64_write_track(image, dadr->track, raw.size, raw.data) < 0) {
        log_error(fsimage_p64_log, "Failed writing track %i to disk image.", dadr->track);
        lib_free(raw.data);
        return -1;
    }

    lib_free(raw.data);
    return 0;
}

/*-----------------------------------------------------------------------*/

void fsimage_p64_init(void)
{
    fsimage_p64_log = log_open("Filesystem Image P64");
}
