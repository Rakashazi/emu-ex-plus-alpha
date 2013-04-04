/*
 * fsimage-dxx.c
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

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "diskconstants.h"
#include "diskimage.h"
#include "cbmdos.h"
#include "fsimage-dxx.h"
#include "fsimage.h"
#include "gcr.h"
#include "log.h"
#include "lib.h"
#include "types.h"
#include "util.h"
#include "x64.h"

static log_t fsimage_dxx_log = LOG_ERR;

int fsimage_dxx_write_half_track(disk_image_t *image, unsigned int half_track,
                                 const disk_track_t *raw)
{
    unsigned int track, sector, max_sector = 0, error_info_created = 0;
    int sectors, res;
    long offset;
    BYTE *buffer;
    fsimage_t *fsimage = image->media.fsimage;
    fdc_err_t rf;

    track = half_track / 2;

    max_sector = disk_image_sector_per_track(image->type, track);
    sectors = disk_image_check_sector(image, track, 0);
    if (sectors < 0) {
        log_error(fsimage_dxx_log, "Track: %i out of bounds.", track);
        return -1;
    }

    if (track > image->tracks) {
        if (fsimage->error_info.map) {
            int newlen = sectors + max_sector;
            fsimage->error_info.map = lib_realloc(fsimage->error_info.map, newlen);
            memset(fsimage->error_info.map + fsimage->error_info.len, 0,
                   newlen - fsimage->error_info.len);
            fsimage->error_info.len = newlen;
            fsimage->error_info.dirty = 1;
        }
        image->tracks = track;
    }

    buffer = lib_calloc(max_sector, 256);
    for (sector = 0; sector < max_sector; sector++) {
        rf = gcr_read_sector(raw, &buffer[sector * 256], (BYTE)sector);
        if (rf != CBMDOS_FDC_ERR_OK) {
            log_error(fsimage_dxx_log,
                      "Could not find data sector of T:%d S:%d.",
                      track, sector);
            if (fsimage->error_info.map == NULL) { /* create map if does not exists */
                int newlen = disk_image_check_sector(image, image->tracks, 0);
                if (newlen >= 0) {
                    newlen += disk_image_sector_per_track(image->type, image->tracks);
                    fsimage->error_info.map = lib_malloc(newlen);
                    memset(fsimage->error_info.map, (BYTE)CBMDOS_FDC_ERR_OK, newlen);
                    fsimage->error_info.len = newlen;
                    fsimage->error_info.dirty = 1;
                    error_info_created = 1;
                }
            }
        }
        if (fsimage->error_info.map != NULL) {
            if (fsimage->error_info.map[sectors + sector] != (BYTE)rf) {
                fsimage->error_info.map[sectors + sector] = (BYTE)rf;
                fsimage->error_info.dirty = 1;
            }
        }
    }
    offset = sectors * 256;

    if (image->type == DISK_IMAGE_TYPE_X64) {
        offset += X64_HEADER_LENGTH;
    }

    if (util_fpwrite(fsimage->fd, buffer, max_sector * 256, offset) < 0) {
        log_error(fsimage_dxx_log, "Error writing T:%i to disk image.",
                  track);
        lib_free(buffer);
        return -1;
    }
    lib_free(buffer);
    if (fsimage->error_info.map) {
        if (fsimage->error_info.dirty) {
            offset = fsimage->error_info.len * 256 + sectors;

            if (image->type == DISK_IMAGE_TYPE_X64) {
                offset += X64_HEADER_LENGTH;
            }

            fsimage->error_info.dirty = 0;
            if (error_info_created) {
                res = util_fpwrite(fsimage->fd, fsimage->error_info.map,
                                   fsimage->error_info.len, fsimage->error_info.len * 256);
            } else {
                res = util_fpwrite(fsimage->fd, fsimage->error_info.map + sectors,
                                   max_sector, offset);
            }
            if (res < 0) {
                log_error(fsimage_dxx_log, "Error writing T:%i error info to disk image.",
                          track);
                return -1;
            }
        }
    }

    /* Make sure the stream is visible to other readers.  */
    fflush(fsimage->fd);
    return 0;
}

int fsimage_read_dxx_image(const disk_image_t *image)
{
    BYTE buffer[256], *bam_id;
    int gap;
    unsigned int track, sector, track_size;
    gcr_header_t header;
    fdc_err_t rf;
    int double_sided = 0;
    fsimage_t *fsimage = image->media.fsimage;
    unsigned int max_sector;
    BYTE *ptr;
    int half_track;
    int sectors;
    long offset;

    if (image->type == DISK_IMAGE_TYPE_D80
        || image->type == DISK_IMAGE_TYPE_D82) {
        sectors = disk_image_check_sector(image, BAM_TRACK_8050, BAM_SECTOR_8050);
        bam_id = &buffer[BAM_ID_8050];
    } else {
        sectors = disk_image_check_sector(image, BAM_TRACK_1541, BAM_SECTOR_1541);
        bam_id = &buffer[BAM_ID_1541];
    }

    bam_id[0] = bam_id[1] = 0xa0;
    if (sectors >= 0) {
        util_fpread(fsimage->fd, buffer, 256, sectors << 8);
    }
    header.id1 = bam_id[0];
    header.id2 = bam_id[1];

    /* check double sided images */
    double_sided = (image->type == DISK_IMAGE_TYPE_D71) && !(buffer[0x03] & 0x80);

    for (header.track = track = 1; track <= image->max_half_tracks / 2; track++, header.track++) {
        half_track = track * 2 - 2;

        track_size = disk_image_raw_track_size(image->type, track);
        if (image->gcr->tracks[half_track].data == NULL) {
            image->gcr->tracks[half_track].data = lib_malloc(track_size);
        } else if (image->gcr->tracks[half_track].size != (int)track_size) {
            image->gcr->tracks[half_track].data = lib_realloc(image->gcr->tracks[half_track].data, track_size);
        }
        ptr = image->gcr->tracks[half_track].data;
        image->gcr->tracks[half_track].size = track_size;

        if (track <= image->tracks) {
            if (double_sided && track == 36) {
                sectors = disk_image_check_sector(image, BAM_TRACK_1571 + 35, BAM_SECTOR_1571);

                buffer[BAM_ID_1571] = buffer[BAM_ID_1571 + 1] = 0xa0;
                if (sectors >= 0) {
                    util_fpread(fsimage->fd, buffer, 256, sectors << 8);
                }
                header.id1 = buffer[BAM_ID_1571]; /* second side, update id and track */
                header.id2 = buffer[BAM_ID_1571 + 1];
                header.track = 1;
            }

            gap = disk_image_gap_size(image->type, track);

            max_sector = disk_image_sector_per_track(image->type, track);

            /* Clear track to avoid read errors.  */
            memset(ptr, 0x55, track_size);

            for (sector = 0; sector < max_sector; sector++) {
                sectors = disk_image_check_sector(image, track, sector);
                offset = sectors * 256;

                if (image->type == DISK_IMAGE_TYPE_X64) {
                    offset += X64_HEADER_LENGTH;
                }

                if (sectors >= 0) {
                    rf = CBMDOS_FDC_ERR_DRIVE;
                    if (util_fpread(fsimage->fd, buffer, 256, offset) >= 0) {
                        if (fsimage->error_info.map != NULL) {
                            rf = fsimage->error_info.map[sectors];
                        }
                    }
                    header.sector = sector;
                    gcr_convert_sector_to_GCR(buffer, ptr, &header, 9, 5, rf);
                }

                ptr += SECTOR_GCR_SIZE_WITH_HEADER + 9 + gap + 5;
            }
        } else {
            memset(ptr, 0x55, track_size);
        }

        /* Clear odd track */
        half_track++;
        if (image->gcr->tracks[half_track].data) {
            lib_free(image->gcr->tracks[half_track].data);
            image->gcr->tracks[half_track].data = NULL;
            image->gcr->tracks[half_track].size = 0;
        }
    }
    return 0;
}

int fsimage_dxx_read_sector(const disk_image_t *image, BYTE *buf, const disk_addr_t *dadr)
{
    int sectors;
    long offset;
    fsimage_t *fsimage = image->media.fsimage;
    fdc_err_t rf;

    sectors = disk_image_check_sector(image, dadr->track, dadr->sector);

    if (sectors < 0) {
        log_error(fsimage_dxx_log, "Track %i, Sector %i out of bounds.",
                  dadr->track, dadr->sector);
        return -1;
    }

    offset = sectors * 256;

    if (image->type == DISK_IMAGE_TYPE_X64) {
        offset += X64_HEADER_LENGTH;
    }

    if (image->gcr == NULL) {
        if (util_fpread(fsimage->fd, buf, 256, offset) < 0) {
            log_error(fsimage_dxx_log,
                      "Error reading T:%i S:%i from disk image.",
                      dadr->track, dadr->sector);
            return -1;
        } else {
            rf = fsimage->error_info.map ? fsimage->error_info.map[sectors] : CBMDOS_FDC_ERR_OK;
        }
    } else {
        rf = gcr_read_sector(&image->gcr->tracks[(dadr->track * 2) - 2], buf, (BYTE)dadr->sector);
    }

    switch (rf) {
        case CBMDOS_FDC_ERR_OK:
            return CBMDOS_IPE_OK;           /* 0 */
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
            return CBMDOS_IPE_OK;
    }
}

int fsimage_dxx_write_sector(disk_image_t *image, const BYTE *buf, const disk_addr_t *dadr)
{
    int sectors;
    long offset;
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    sectors = disk_image_check_sector(image, dadr->track, dadr->sector);

    if (sectors < 0) {
        log_error(fsimage_dxx_log, "Track: %i, Sector: %i out of bounds.",
                  dadr->track, dadr->sector);
        return -1;
    }
    offset = sectors * 256;

    if (image->type == DISK_IMAGE_TYPE_X64) {
        offset += X64_HEADER_LENGTH;
    }

    if (util_fpwrite(fsimage->fd, buf, 256, offset) < 0) {
        log_error(fsimage_dxx_log, "Error writing T:%i S:%i to disk image.",
                  dadr->track, dadr->sector);
        return -1;
    }
    if (image->gcr != NULL) {
        gcr_write_sector(&image->gcr->tracks[(dadr->track * 2) - 2], buf, (BYTE)dadr->sector);
    }

    if ((fsimage->error_info.map != NULL)
        && (fsimage->error_info.map[sectors] != CBMDOS_FDC_ERR_OK)) {
        offset = fsimage->error_info.len * 256 + sectors;

        if (image->type == DISK_IMAGE_TYPE_X64) {
            offset += X64_HEADER_LENGTH;
        }

        fsimage->error_info.map[sectors] = CBMDOS_FDC_ERR_OK;
        if (util_fpwrite(fsimage->fd, &fsimage->error_info.map[sectors], 1, offset) < 0) {
            log_error(fsimage_dxx_log, "Error writing T:%i S:%i error info to disk image.",
                      dadr->track, dadr->sector);
        }
    }

    /* Make sure the stream is visible to other readers.  */
    fflush(fsimage->fd);
    return 0;
}

void fsimage_dxx_init(void)
{
    fsimage_dxx_log = log_open("Filesystem Image DXX");
}
