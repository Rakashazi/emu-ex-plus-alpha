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
#include "drive.h"
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
    uint8_t *buffer;
    fsimage_t *fsimage = image->media.fsimage;
    fdc_err_t rf;

    track = half_track / 2;

    max_sector = disk_image_sector_per_track(image->type, track);
    sectors = disk_image_check_sector(image, track, 0);
    if (sectors < 0) {
        log_error(fsimage_dxx_log, "Track: %u out of bounds.", track);
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
        rf = gcr_read_sector(raw, &buffer[sector * 256], (uint8_t)sector);
        if (rf != CBMDOS_FDC_ERR_OK) {
            log_error(fsimage_dxx_log,
                      "Could not find data sector of T:%u S:%u.",
                      track, sector);
            if (fsimage->error_info.map == NULL) { /* create map if does not exists */
                int newlen = disk_image_check_sector(image, image->tracks, 0);
                if (newlen >= 0) {
                    newlen += disk_image_sector_per_track(image->type, image->tracks);
                    fsimage->error_info.map = lib_malloc(newlen);
                    memset(fsimage->error_info.map, (uint8_t)CBMDOS_FDC_ERR_OK, newlen);
                    fsimage->error_info.len = newlen;
                    fsimage->error_info.dirty = 1;
                    error_info_created = 1;
                }
            }
        }
        if (fsimage->error_info.map != NULL) {
            if (fsimage->error_info.map[sectors + sector] != (uint8_t)rf) {
                fsimage->error_info.map[sectors + sector] = (uint8_t)rf;
                fsimage->error_info.dirty = 1;
            }
        }
    }
    offset = sectors * 256;

#ifdef HAVE_X64_IMAGE
    if (image->type == DISK_IMAGE_TYPE_X64) {
        offset += X64_HEADER_LENGTH;
    }
#endif
    if (util_fpwrite(fsimage->fd, buffer, max_sector * 256, offset) < 0) {
        log_error(fsimage_dxx_log, "Error writing T:%u to disk image.",
                  track);
        lib_free(buffer);
        return -1;
    }
    lib_free(buffer);
    if (fsimage->error_info.map) {
        if (fsimage->error_info.dirty) {
            offset = fsimage->error_info.len * 256 + sectors;

#ifdef HAVE_X64_IMAGE
            if (image->type == DISK_IMAGE_TYPE_X64) {
                offset += X64_HEADER_LENGTH;
            }
#endif
            fsimage->error_info.dirty = 0;
            if (error_info_created) {
                res = util_fpwrite(fsimage->fd, fsimage->error_info.map,
                                   fsimage->error_info.len, fsimage->error_info.len * 256);
            } else {
                res = util_fpwrite(fsimage->fd, fsimage->error_info.map + sectors,
                                   max_sector, offset);
            }
            if (res < 0) {
                log_error(fsimage_dxx_log,
                        "Error writing T:%u error info to disk image.",
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
    uint8_t buffer[256], *bam_id;
    int gap, headergap, synclen;
    unsigned int track, sector, track_size;
    gcr_header_t header;
    fdc_err_t rf;
    int image_has_two_single_sides = 0;
    int double_sided_drive = 0;
    fsimage_t *fsimage = image->media.fsimage;
    unsigned int max_sector;
    uint8_t *ptr;
    int half_track;
    int sectors;
    long offset;
    unsigned long trackoffset = 0;
    uint8_t *tempgcr;

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
    } else {
        return -1;
    }
    header.id1 = bam_id[0];
    header.id2 = bam_id[1];

    /* check double sided images */
    image_has_two_single_sides = (image->type == DISK_IMAGE_TYPE_D71) && !(buffer[0x03] & 0x80);
    double_sided_drive = (drive_get_disk_drive_type(image->device) == DRIVE_TYPE_1571) ||
                         (drive_get_disk_drive_type(image->device) == DRIVE_TYPE_1571CR);

    /* special case for 1571: if we are inserting a d64 image into a 1571, fill
       the second side with "unformatted" data */
    if (double_sided_drive && (image->type != DISK_IMAGE_TYPE_D71)) {
        for (header.track = track = 1; track <= image->max_half_tracks / 2; track++, header.track++) {
            half_track = (36 + track) * 2 - 2;

            track_size = disk_image_raw_track_size(image->type, track);
            if (image->gcr->tracks[half_track].data == NULL) {
                image->gcr->tracks[half_track].data = lib_malloc(track_size);
            } else if (image->gcr->tracks[half_track].size != (int)track_size) {
                image->gcr->tracks[half_track].data = lib_realloc(image->gcr->tracks[half_track].data, track_size);
            }
            ptr = image->gcr->tracks[half_track].data;
            image->gcr->tracks[half_track].size = track_size;
            /* regular track */
            memset(ptr, 0, track_size);

            /* Clear odd track */
            half_track++;

            /* create an (empty) half track */
            if (image->gcr->tracks[half_track].data == NULL) {
                image->gcr->tracks[half_track].data = lib_malloc(track_size);
            } else if (image->gcr->tracks[half_track].size != (int)track_size) {
                image->gcr->tracks[half_track].data = lib_realloc(image->gcr->tracks[half_track].data, track_size);
            }
            image->gcr->tracks[half_track].size = track_size;
            ptr = image->gcr->tracks[half_track].data;
            memset(ptr, 0, track_size);
        }
    }

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
            /* get temp buffer */
            ptr = tempgcr = lib_malloc(track_size);

            /* special case for second side of the 1571. If each side was formatted
               separately in one-sided mode, we must start from track 1 again and use
               the ID from the BAM on the second side. */
            if (image_has_two_single_sides && track == 36) {
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
            headergap = disk_image_header_gap_size(image->type, track);
            synclen = disk_image_sync_size(image->type, track);

            max_sector = disk_image_sector_per_track(image->type, track);

            /* Clear track to avoid read errors.  */
            memset(ptr, 0x55, track_size);

            for (sector = 0; sector < max_sector; sector++) {
                sectors = disk_image_check_sector(image, track, sector);
                offset = sectors * 256;

#ifdef HAVE_X64_IMAGE
                if (image->type == DISK_IMAGE_TYPE_X64) {
                    offset += X64_HEADER_LENGTH;
                }
#endif
                if (sectors >= 0) {
                    rf = CBMDOS_FDC_ERR_DRIVE;
                    if (util_fpread(fsimage->fd, buffer, 256, offset) >= 0) {
                        if (fsimage->error_info.map != NULL) {
                            rf = fsimage->error_info.map[sectors];
                        }
                    }
                    header.sector = sector;
                    gcr_convert_sector_to_GCR(buffer, ptr, &header, headergap, synclen, rf);
                }

                ptr += SECTOR_GCR_SIZE_WITH_HEADER + headergap + gap + (synclen * 2);
            }

#if 0
            /* copy gcr data to buffer (this creates perfectly aligned tracks) */
            ptr = image->gcr->tracks[half_track].data;
            memcpy(ptr, tempgcr, track_size);
#else
            /* copy gcr data to final buffer with offset + wraparound */
            /* On real disks, the track skew depends on many factors of which
               none is exactly defined: the mechanical properties of the drive,
               and last not least the code used for formatting the disk. Thus
               the offset we use here is somewhat arbitrary, the choosen values
               are tweaked to be somewhat close to what the skew1.prg program
               shows for the first few tracks. */
            trackoffset += (ptr - tempgcr) - gap; /* bytes we have written */
            trackoffset += (track_size * 100) / 270; /* time it takes to step */
            trackoffset %= track_size;
            /*printf("track: %2u sectors: %2u size: %5u offset: %5lu\n", track, max_sector, track_size, trackoffset);*/
            ptr = image->gcr->tracks[half_track].data;
            memset(ptr, 0x55, track_size);
            memcpy(ptr + trackoffset, tempgcr, track_size - trackoffset);
            memcpy(ptr, tempgcr + (track_size - trackoffset), track_size - (track_size - trackoffset));
#endif
            lib_free(tempgcr);
        } else {
            memset(ptr, 0x55, track_size);
        }

        /* Clear odd track */
        half_track++;
#if 0
        /* this does not work for some reason (skew.d64 fails) */
        if (image->gcr->tracks[half_track].data) {
            image->gcr->tracks[half_track].size = track_size;
            ptr = image->gcr->tracks[half_track].data;
            memset(ptr, 0, track_size);
        }
#else
        /* create an (empty) half track */
        if (image->gcr->tracks[half_track].data == NULL) {
            image->gcr->tracks[half_track].data = lib_malloc(track_size);
        } else if (image->gcr->tracks[half_track].size != (int)track_size) {
            image->gcr->tracks[half_track].data = lib_realloc(image->gcr->tracks[half_track].data, track_size);
        }
        image->gcr->tracks[half_track].size = track_size;
        ptr = image->gcr->tracks[half_track].data;
        memset(ptr, 0, track_size);
#endif

    }
    return 0;
}

int fsimage_dxx_read_sector(const disk_image_t *image, uint8_t *buf, const disk_addr_t *dadr)
{
    int sectors;
    long offset;
    fsimage_t *fsimage = image->media.fsimage;
    fdc_err_t rf;
    int harderror;

    sectors = disk_image_check_sector(image, dadr->track, dadr->sector);

    /* printf("%d:%d = %d sectors image->gcr:%p\n", dadr->track, dadr->sector, sectors, image->gcr); */

    if (sectors < 0) {
        log_error(fsimage_dxx_log, "Track %u, Sector %u out of bounds.",
                  dadr->track, dadr->sector);
        return -1;
    }

    offset = sectors * 256;

#ifdef HAVE_X64_IMAGE
    if (image->type == DISK_IMAGE_TYPE_X64) {
        offset += X64_HEADER_LENGTH;
    }
#endif

    /* first check hard errors, if there is such hard error, then skip the
       reading and do not update the buffer */
    harderror = 0;
    if (fsimage->error_info.map) {
        harderror = 1;
        rf = fsimage->error_info.map[sectors];
        /* these are soft errors, let rf=0 pass for a read */
        if ((rf == 0) || (rf == 1) || (rf == 5) || (rf == 7) || (rf == 8)) {
            harderror = 0;
        }
    }

    if (harderror == 0) {
        if (image->gcr == NULL) {
            if (util_fpread(fsimage->fd, buf, 256, offset) < 0) {
                log_error(fsimage_dxx_log,
                        "Error reading T:%u S:%u from disk image.",
                        dadr->track, dadr->sector);
                return -1;
            } else {
                rf = fsimage->error_info.map ? fsimage->error_info.map[sectors] : CBMDOS_FDC_ERR_OK;
            }
        } else {
            rf = gcr_read_sector(&image->gcr->tracks[(dadr->track * 2) - 2], buf, (uint8_t)dadr->sector);
            /* HACK: if the image has an error map, and the "FDC" did not detect an
            error in the GCR stream, use the error from the error map instead.
            FIXME: what should really be done is encoding the errors from the
            error map into the GCR stream. this is a lot more effort and will
            give the exact same results, so i will leave it to someone else :)
            */
            /* printf("read sector %d:%d returned: %d", dadr->track, dadr->sector, rf); */
            if (fsimage->error_info.map && (rf == CBMDOS_FDC_ERR_OK)) {
                rf = fsimage->error_info.map[sectors];
                /* printf(" error map: %d", rf); */
            }
            /* printf("\n"); */
        }
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

int fsimage_dxx_write_sector(disk_image_t *image, const uint8_t *buf, const disk_addr_t *dadr)
{
    int sectors;
    long offset;
    fsimage_t *fsimage;

    fsimage = image->media.fsimage;

    sectors = disk_image_check_sector(image, dadr->track, dadr->sector);

    if (sectors < 0) {
        log_error(fsimage_dxx_log, "Track: %u, Sector: %u out of bounds.",
                  dadr->track, dadr->sector);
        return -1;
    }
    offset = sectors * 256;

#ifdef HAVE_X64_IMAGE
    if (image->type == DISK_IMAGE_TYPE_X64) {
        offset += X64_HEADER_LENGTH;
    }
#endif
    if (util_fpwrite(fsimage->fd, buf, 256, offset) < 0) {
        log_error(fsimage_dxx_log, "Error writing T:%u S:%u to disk image.",
                  dadr->track, dadr->sector);
        return -1;
    }
    if (image->gcr != NULL) {
        gcr_write_sector(&image->gcr->tracks[(dadr->track * 2) - 2], buf, (uint8_t)dadr->sector);
    }

    if ((fsimage->error_info.map != NULL)
        && (fsimage->error_info.map[sectors] != CBMDOS_FDC_ERR_OK)) {
        offset = fsimage->error_info.len * 256 + sectors;

#ifdef HAVE_X64_IMAGE
        if (image->type == DISK_IMAGE_TYPE_X64) {
            offset += X64_HEADER_LENGTH;
        }
#endif
        fsimage->error_info.map[sectors] = CBMDOS_FDC_ERR_OK;
        if (util_fpwrite(fsimage->fd, &fsimage->error_info.map[sectors], 1, offset) < 0) {
            log_error(fsimage_dxx_log,
                    "Error writing T:%u S:%u error info to disk image.",
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
