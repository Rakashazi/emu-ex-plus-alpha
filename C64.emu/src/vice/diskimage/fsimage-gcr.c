/*
 * fsimage-gcr.c
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
#include <string.h>

#include "diskconstants.h"
#include "diskimage.h"
#include "fsimage-gcr.h"
#include "fsimage.h"
#include "gcr.h"
#include "cbmdos.h"
#include "log.h"
#include "lib.h"
#include "types.h"
#include "util.h"


static log_t fsimage_gcr_log = LOG_ERR;
static const BYTE gcr_image_header_expected_1541[] =
    { 0x47, 0x43, 0x52, 0x2D, 0x31, 0x35, 0x34, 0x31, 0x00 };
static const BYTE gcr_image_header_expected_1571[] =
    { 0x47, 0x43, 0x52, 0x2D, 0x31, 0x35, 0x37, 0x31, 0x00 };

/*-----------------------------------------------------------------------*/
/* Intial GCR buffer setup.  */

int fsimage_read_gcr_image(const disk_image_t *image)
{
    unsigned int half_track;

    for (half_track = 0; half_track < MAX_GCR_TRACKS; half_track++) {
        if (image->gcr->tracks[half_track].data) {
            lib_free(image->gcr->tracks[half_track].data);
            image->gcr->tracks[half_track].data = NULL;
            image->gcr->tracks[half_track].size = 0;
        }
        if (half_track < image->max_half_tracks) {
            fsimage_gcr_read_half_track(image, half_track + 2, &image->gcr->tracks[half_track]);
        }
    }
    return 0;
}
/*-----------------------------------------------------------------------*/
/* Seek to half track */

static long fsimage_gcr_seek_half_track(fsimage_t *fsimage, unsigned int half_track,
                                        WORD *max_track_length, BYTE *num_half_tracks)
{
    BYTE buf[12];

    if (fsimage->fd == NULL) {
        log_error(fsimage_gcr_log, "Attempt to read without disk image.");
        return -1;
    }
    if (util_fpread(fsimage->fd, buf, 12, 0) < 0) {
        log_error(fsimage_gcr_log, "Could not read GCR disk image.");
        return -1;
    }
    if ((memcmp(gcr_image_header_expected_1541, buf, sizeof(gcr_image_header_expected_1541)) != 0)
        && (memcmp(gcr_image_header_expected_1571, buf, sizeof(gcr_image_header_expected_1571)) != 0)) {
        log_error(fsimage_gcr_log, "Unexpected GCR header found." );
        return -1;
    }

    *num_half_tracks = buf[9];
    if (*num_half_tracks > MAX_GCR_TRACKS) {
        log_error(fsimage_gcr_log, "Too many half tracks." );
        return -1;
    }

    *max_track_length = util_le_buf_to_word(&buf[10]);
#if 0
    /* if 0'ed because:
       *max_track_length is of type WORD and NUM_MAX_MEM_BYTES_TRACK is 65536
     */
    if (*max_track_length > NUM_MAX_MEM_BYTES_TRACK) {
        log_error(fsimage_gcr_log, "Too large max track length.");
        return -1;
    }
#endif

    if (util_fpread(fsimage->fd, buf, 4, 12 + (half_track - 2) * 4) < 0) {
        log_error(fsimage_gcr_log, "Could not read GCR disk image.");
        return -1;
    }
    return util_le_buf_to_dword(buf);
}

/*-----------------------------------------------------------------------*/
/* Read an entire GCR track from the disk image.  */

int fsimage_gcr_read_half_track(const disk_image_t *image, unsigned int half_track,
                                disk_track_t *raw)
{
    WORD track_len;
    BYTE buf[4];
    long offset;
    fsimage_t *fsimage;
    WORD max_track_length;
    BYTE num_half_tracks;

    fsimage = image->media.fsimage;

    raw->data = NULL;
    raw->size = 0;

    offset = fsimage_gcr_seek_half_track(fsimage, half_track, &max_track_length, &num_half_tracks);

    if (offset < 0) {
        return -1;
    }

    if (offset != 0) {
        if (util_fpread(fsimage->fd, buf, 2, offset) < 0) {
            log_error(fsimage_gcr_log, "Could not read GCR disk image.");
            return -1;
        }

        track_len = util_le_buf_to_word(buf);

        if ((track_len < 1) || (track_len > max_track_length)) {
            log_error(fsimage_gcr_log,
                      "Track field length %u is not supported.",
                      track_len);
            return -1;
        }

        raw->data = lib_calloc(1, track_len);
        raw->size = track_len;

        if (fread(raw->data, track_len, 1, fsimage->fd) < 1) {
            log_error(fsimage_gcr_log, "Could not read GCR disk image.");
            return -1;
        }
    } else {
        raw->size = disk_image_raw_track_size(image->type, half_track / 2);
        raw->data = lib_malloc(raw->size);
        memset(raw->data, 0x55, raw->size);
    }
    return 0;
}

static int fsimage_gcr_read_track(const disk_image_t *image, unsigned int track,
                                  disk_track_t *raw)
{
    return fsimage_gcr_read_half_track(image, track << 1, raw);
}

/*-----------------------------------------------------------------------*/
/* Write an entire GCR track to the disk image.  */

int fsimage_gcr_write_half_track(disk_image_t *image, unsigned int half_track,
                                 const disk_track_t *raw)
{
    int gap, extend = 0, res;
    WORD max_track_length;
    BYTE buf[4];
    long offset;
    fsimage_t *fsimage;
    BYTE num_half_tracks;

    fsimage = image->media.fsimage;

    offset = fsimage_gcr_seek_half_track(fsimage, half_track, &max_track_length, &num_half_tracks);
    if (offset < 0) {
        return -1;
    }
    if (image->read_only != 0) {
        log_error(fsimage_gcr_log,
                  "Attempt to write to read-only disk image.");
        return -1;
    }

    if (raw->size > max_track_length) {
        log_error(fsimage_gcr_log,
                  "Track too long for image.");
        return -1;
    }

    if (offset == 0) {
        offset = fseek(fsimage->fd, 0, SEEK_END);
        if (offset == 0) {
            offset = ftell(fsimage->fd);
        }
        if (offset < 0) {
            log_error(fsimage_gcr_log, "Could not extend GCR disk image.");
            return -1;
        }
        extend = 1;
    }

    if (raw->data != NULL) {
        util_word_to_le_buf(buf, (WORD)raw->size);

        if (util_fpwrite(fsimage->fd, buf, 2, offset) < 0) {
            log_error(fsimage_gcr_log, "Could not write GCR disk image.");
            return -1;
        }

        /* Clear gap between the end of the actual track and the start of
           the next track.  */
        if (fwrite(raw->data, raw->size, 1, fsimage->fd) < 1) {
            log_error(fsimage_gcr_log, "Could not write GCR disk image.");
            return -1;
        }
        gap = max_track_length - raw->size;

        if (gap > 0) {
            BYTE *padding = lib_calloc(1, gap);
            res = fwrite(padding, gap, 1, fsimage->fd);
            lib_free(padding);
            if (res < 1) {
                log_error(fsimage_gcr_log, "Could not write GCR disk image.");
                return -1;
            }
        }

        if (extend) {
            util_dword_to_le_buf(buf, offset);
            if (util_fpwrite(fsimage->fd, buf, 4, 12 + (half_track - 2) * 4) < 0) {
                log_error(fsimage_gcr_log, "Could not write GCR disk image.");
                return -1;
            }

            util_dword_to_le_buf(buf, disk_image_speed_map(image->type, half_track / 2));
            if (util_fpwrite(fsimage->fd, buf, 4, 12 + (half_track - 2 + num_half_tracks) * 4) < 0) {
                log_error(fsimage_gcr_log, "Could not write GCR disk image.");
                return -1;
            }
        }
    }

    /* Make sure the stream is visible to other readers.  */
    fflush(fsimage->fd);

    return 0;
}

static int fsimage_gcr_write_track(disk_image_t *image, unsigned int track,
                                   const disk_track_t *raw)
{
    return fsimage_gcr_write_half_track(image, track << 1, raw);
}

/*-----------------------------------------------------------------------*/
/* Read a sector from the GCR disk image.  */

int fsimage_gcr_read_sector(const disk_image_t *image, BYTE *buf, const disk_addr_t *dadr)
{
    fdc_err_t rf;

    if (dadr->track > image->tracks) {
        log_error(fsimage_gcr_log,
                  "Track %i out of bounds.  Cannot read GCR track.",
                  dadr->track);
        return -1;
    }

    if (image->gcr == NULL) {
        disk_track_t raw;
        if (fsimage_gcr_read_track(image, dadr->track, &raw) < 0) {
            return -1;
        }
        if (raw.data == NULL) {
            return CBMDOS_IPE_NOT_READY;
        }
        rf = gcr_read_sector(&raw, buf, (BYTE)dadr->sector);
        lib_free(raw.data);
    } else {
        rf = gcr_read_sector(&image->gcr->tracks[(dadr->track * 2) - 2], buf, (BYTE)dadr->sector);
    }
    if (rf != CBMDOS_FDC_ERR_OK) {
        log_error(fsimage_gcr_log,
                  "Cannot find track: %i sector: %i within GCR image.",
                  dadr->track, dadr->sector);
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
/* Write a sector to the GCR disk image.  */

int fsimage_gcr_write_sector(disk_image_t *image, const BYTE *buf,
                             const disk_addr_t *dadr)
{
    if (dadr->track > image->tracks) {
        log_error(fsimage_gcr_log,
                  "Track %i out of bounds.  Cannot write GCR sector",
                  dadr->track);
        return -1;
    }

    if (image->gcr == NULL) {
        disk_track_t raw;
        if (fsimage_gcr_read_track(image, dadr->track, &raw) < 0
            || raw.data == NULL) {
            return -1;
        }
        if (gcr_write_sector(&raw, buf, (BYTE)dadr->sector) != CBMDOS_FDC_ERR_OK) {
            log_error(fsimage_gcr_log,
                      "Could not find track %i sector %i in disk image",
                      dadr->track, dadr->sector);
            lib_free(raw.data);
            return -1;
        }
        if (fsimage_gcr_write_track(image, dadr->track, &raw) < 0) {
            lib_free(raw.data);
            return -1;
        }
        lib_free(raw.data);
    } else {
        if (gcr_write_sector(&image->gcr->tracks[(dadr->track * 2) - 2], buf, (BYTE)dadr->sector) != CBMDOS_FDC_ERR_OK) {
            log_error(fsimage_gcr_log,
                      "Could not find track %i sector %i in disk image",
                      dadr->track, dadr->sector);
            return -1;
        }
        if (fsimage_gcr_write_track(image, dadr->track, &image->gcr->tracks[(dadr->track * 2) - 2]) < 0) {
            log_error(fsimage_gcr_log,
                      "Failed writing track %i to disk image.", dadr->track);
            return -1;
        }
    }

    return 0;
}

/*-----------------------------------------------------------------------*/

void fsimage_gcr_init(void)
{
    fsimage_gcr_log = log_open("Filesystem Image GCR");
}
