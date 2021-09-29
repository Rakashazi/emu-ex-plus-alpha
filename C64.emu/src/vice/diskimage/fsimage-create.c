/** \file   src/diskimage/fsimage-create.c
 *
 * \brief   Create disk images on the host file system
 */

/*
 * fsimage-create.c - Create a disk image.
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
#include <string.h>

#include "archdep.h"
#include "diskconstants.h"
#include "diskimage.h"
#include "fsimage-create.h"
#include "fsimage.h"
#include "gcr.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "util.h"
#include "x64.h"
#include "p64.h"
#include "cbmdos.h"

/* #define DBGIMGCREATE */

#ifdef DBGIMGCREATE
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/** \brief  Log instance for this module
 */
static log_t createdisk_log = LOG_DEFAULT;


/** \brief  Create a Dxx disk image on the host file system
 *
 * This handles any 'normal'/non-GCR disk image supported by VICE. The only
 * image contained inside a X64 that is supported is a D64.
 *
 * \param[in,out]   image   disk image
 *
 * \return  0 on success, < 0 on failure
 */
static int fsimage_create_dxx(disk_image_t *image)
{
    unsigned int size, i;
    uint8_t block[256];
    fsimage_t *fsimage = image->media.fsimage;
    int rc = 0;

    memset(block, 0, sizeof(block));
    size = 0;

    switch (image->type) {
        case DISK_IMAGE_TYPE_D64:
#ifdef HAVE_X64_IMAGE
        case DISK_IMAGE_TYPE_X64:
#endif
            size = D64_FILE_SIZE_35;
            break;
        case DISK_IMAGE_TYPE_D67:
            size = D67_FILE_SIZE;
            break;
        case DISK_IMAGE_TYPE_D71:
            size = D71_FILE_SIZE;
            break;
        case DISK_IMAGE_TYPE_D81:
            size = D81_FILE_SIZE;
            break;
        case DISK_IMAGE_TYPE_D80:
            size = D80_FILE_SIZE;
            break;
        case DISK_IMAGE_TYPE_D82:
            size = D82_FILE_SIZE;
            break;
        case DISK_IMAGE_TYPE_D90:
            /* use D9090 for default D90 size */
            size = D9090_FILE_SIZE;
            break;
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_G71:
            break;
        case DISK_IMAGE_TYPE_P64:
            break;
        default:
            log_error(createdisk_log,
                      "Wrong image type.  Cannot create disk image.");
            return -1;
    }

#ifdef HAVE_X64_IMAGE
    if (image->type == DISK_IMAGE_TYPE_X64) {
        uint8_t header[X64_HEADER_LENGTH];

        memset(header, 0, X64_HEADER_LENGTH);

        header[X64_HEADER_MAGIC_OFFSET + 0] = X64_HEADER_MAGIC_1;
        header[X64_HEADER_MAGIC_OFFSET + 1] = X64_HEADER_MAGIC_2;
        header[X64_HEADER_MAGIC_OFFSET + 2] = X64_HEADER_MAGIC_3;
        header[X64_HEADER_MAGIC_OFFSET + 3] = X64_HEADER_MAGIC_4;
        header[X64_HEADER_VERSION_OFFSET + 0] = X64_HEADER_VERSION_MAJOR;
        header[X64_HEADER_VERSION_OFFSET + 1] = X64_HEADER_VERSION_MINOR;
        header[X64_HEADER_FLAGS_OFFSET + 0] = 1;
        header[X64_HEADER_FLAGS_OFFSET + 1] = NUM_TRACKS_1541;
        header[X64_HEADER_FLAGS_OFFSET + 2] = 1;
        header[X64_HEADER_FLAGS_OFFSET + 3] = 0;
        if (fwrite(header, X64_HEADER_LENGTH, 1, fsimage->fd) < 1) {
            log_error(createdisk_log,
                      "Cannot write X64 header to disk image `%s'.",
                      fsimage->name);
        }
    }
#endif
    for (i = 0; i < (size / 256); i++) {
        if (fwrite(block, 256, 1, fsimage->fd) < 1) {
            log_error(createdisk_log,
                      "Cannot seek to end of disk image `%s'.",
                      fsimage->name);
            rc = -1;
            break;
        }
    }
    return rc;
}


/** \brief  Create a DxM disk image on the host file system
 *
 * This handles any 'FD2000 or FD4000 disk image supported by VICE.
 *
 * \param[in]   name      disk image name/path
 * \param[in]   diskname  disk name and id
 * \param[in]   type      disk image type
 *
 * \return  0 on success, < 0 on failure
 */
int fsimage_create_dxm(const char *name, const char *diskname, unsigned int type)
{
    FILE *fd = NULL;
    unsigned int size = 0;
    unsigned int partblock = 0;
    uint8_t block[256];
    int rc = 0;
    char *dname, *comma;
    uint8_t id[2];
    unsigned int i, j;

    memset(block, 0, sizeof(block));
    size = 0;

    fd = fopen(name, MODE_WRITE);

    if (fd == NULL) {
        log_error(createdisk_log, "Cannot create disk image `%s'.", name);
        return -1;
    }

    switch (type) {
        case DISK_IMAGE_TYPE_D1M:
            size = D1M_FILE_SIZE;
            partblock = 0x0c85;
            break;
        case DISK_IMAGE_TYPE_D2M:
            size = D2M_FILE_SIZE;
            partblock = 0x1905;
            break;
        case DISK_IMAGE_TYPE_D4M:
            size = D4M_FILE_SIZE;
            partblock = 0x3205;
            break;
        default:
            log_error(createdisk_log,
                      "Wrong image type.  Cannot create disk image.");
            return -1;
    }

    comma = strchr(diskname, ',');
    if (comma != NULL) {
        if (comma != diskname) {
            dname = lib_malloc(comma - diskname + 1);
            memcpy(dname, diskname, comma - diskname);
            dname[comma - diskname] = '\0';
        } else {
            dname = lib_strdup(" ");
        }
        if (comma[1] != '\0') {
            id[0] = comma[1];
            if (comma[2] != '\0') {
                id[1] = comma[2];
            } else {
                id[1] = ' ';
            }
        } else {
            id[1] = id[0] = ' ';
        }
    } else {
        dname = lib_strdup(diskname);
        id[1] = id[0] = ' ';
    }

    for (i = 0; i < size / 256; i++) {
        memset(block, 0, 256);
        /* block 1 */
        if (i == 1) {
                block[0x00] = 0x01;
                block[0x01] = 0x22;
                block[0x02] = 0x48;
                for (j = 0; dname[j]; j++) {
                    block[0x04 + j] = dname[j];
                }
                for (;j < 18; j++) {
                    block[0x04 + j] = 0xa0;
                }
                block[0x16] = id[0];
                block[0x17] = id[1];
                block[0x18] = 0xa0;
                block[0x19] = 0x31;
                block[0x1a] = 0x48;
                block[0x1b] = 0xa0;
                block[0x1c] = 0xa0;
                block[0x20] = 0x01;
                block[0x21] = 0x01;
            } else if (i == 2) {
                block[0x02] = 0x48;
                block[0x03] = 0xb7;
                block[0x04] = id[0];
                block[0x05] = id[1];
                block[0x06] = 0xc0;
                switch (type) {
                    case DISK_IMAGE_TYPE_D1M:
                        block[0x08] = 0x0c;
                        break;
                    case DISK_IMAGE_TYPE_D2M:
                        block[0x08] = 0x19;
                        break;
                    default:
                        block[0x08] = 0x32;
                }
                block[0x24] = 0x1f;
                for (j = 0x25; j < 256; j++) {
                    block[j] = 0xff;
                }
            } else if (i >= 3 && i < 0x22) {
                memset(block, 0xff, 256);
            } else if (i == 0x22) {
                block[0x01] = 0xff;
            } else if (i == partblock) {
                memset(block, 0xff, 224);
                block[0x00] = 0x00;
                block[0x38] = 0x00;
                block[0x39] = 0x00;
                block[0x70] = 0x00;
                switch (type) {
                    case DISK_IMAGE_TYPE_D1M:
                        block[0x71] = 0x06;
                        block[0xa9] = 0x40;
                        break;
                    case DISK_IMAGE_TYPE_D2M:
                        block[0x71] = 0x0c;
                        block[0xa9] = 0x80;
                        break;
                    default:
                        block[0x71] = 0x19;
                        block[0xa9] = 0x00;
                }
                block[0xa8] = 0x00;
                block[0xe2] = 0x01;
                block[0xe3] = 0x01;
                block[0xf0] = 0x43;
                block[0xf1] = 0x4d;
                block[0xf2] = 0x44;
                block[0xf3] = 0x20;
                block[0xf4] = 0x46;
                block[0xf5] = 0x44;
                block[0xf6] = 0x20;
                block[0xf7] = 0x53;
                block[0xf8] = 0x45;
                block[0xf9] = 0x52;
                block[0xfa] = 0x49;
                block[0xfb] = 0x45;
                block[0xfc] = 0x53;
                block[0xfd] = 0x20;
                block[0xfe] = 0x20;
                block[0xff] = 0x20;
            } else if (i == partblock + 3) {
                block[0x00] = 0x01;
                block[0x01] = 0x01;
                block[0x02] = 0xff;
                block[0x05] = 0x53;
                block[0x06] = 0x59;
                block[0x07] = 0x53;
                block[0x08] = 0x54;
                block[0x09] = 0x45;
                block[0x0a] = 0x4d;
                for (j = 0; j < 10; j++) {
                    block[0x0b + j] = 0xa0;
                }
                block[0x22] = 0x01;
                block[0x25] = 0x50;
                block[0x26] = 0x41;
                block[0x27] = 0x52;
                block[0x28] = 0x54;
                block[0x29] = 0x49;
                block[0x2a] = 0x54;
                block[0x2b] = 0x49;
                block[0x2c] = 0x4f;
                block[0x2d] = 0x4e;
                block[0x2e] = 0x20;
                block[0x2f] = 0x31;
                for (j = 0; j < 5; j++) {
                    block[0x30 + j] = 0xa0;
                }
                switch (type) {
                    case DISK_IMAGE_TYPE_D1M:
                        block[0x3e] = 0x06;
                        block[0x3f] = 0x00;
                        break;
                    case DISK_IMAGE_TYPE_D2M:
                        block[0x3e] = 0x0c;
                        block[0x3f] = 0x80;
                        break;
                    default:
                        block[0x3e] = 0x19;
                        block[0x3f] = 0x00;
                }
            } else if (i == partblock + 4) {
                block[0x00] = 0x01;
                block[0x01] = 0x02;
            } else if (i == partblock + 5) {
                block[0x00] = 0x01;
                block[0x01] = 0x03;
            } else if (i == partblock + 6) {
                block[0x01] = 0xff;
            }
            if (fwrite(block, 256, 1, fd) < 1) {
                log_error(createdisk_log,
                          "Cannot seek to end of disk image `%s'.",
                          name);
                rc = -1;
                break;
            }
    }
    lib_free(dname);
    fclose(fd);

    return rc;
}

/** \brief  Create a G64 disk image on the host file system
 *
 * \param[in,out]   image   disk image
 *
 * return   0 on success, < 0 on failure
 */
static int fsimage_create_gcr(disk_image_t *image)
{
    uint8_t gcr_header[12], gcr_track[NUM_MAX_BYTES_TRACK + 2], *gcrptr;
    uint8_t gcr_track_p[84 * 2 * 4];
    uint8_t gcr_speed_p[84 * 2 * 4];
    unsigned int track, sector, num_tracks, max_tracks;
    fsimage_t *fsimage;
    gcr_header_t header;
    uint8_t rawdata[256];
    int gap, headergap, synclen;

    fsimage = image->media.fsimage;

    if (image->type == DISK_IMAGE_TYPE_G64) {
        strcpy((char *)gcr_header, "GCR-1541");
        num_tracks = NUM_TRACKS_1541;
        max_tracks = MAX_TRACKS_1541;
    } else if (image->type == DISK_IMAGE_TYPE_G71) {
        strcpy((char *)gcr_header, "GCR-1571");
        num_tracks = 84; /* FIXME: why is NUM_TRACKS_1571 different? */
        max_tracks = 84; /* FIXME: why is MAX_TRACKS_1571 different? */
    } else {
        return -1;
    }

    gcr_header[8] = 0;
    gcr_header[9] = max_tracks * 2;
    util_word_to_le_buf(&gcr_header[10], NUM_MAX_BYTES_TRACK);

    if (fwrite(gcr_header, sizeof(gcr_header), 1, fsimage->fd) < 1) {
        log_error(createdisk_log, "Cannot write GCR header.");
        return -1;
    }

    memset(gcr_track_p, 0, (max_tracks * 2 * 4));
    memset(gcr_speed_p, 0, (max_tracks * 2 * 4));
    for (track = 0; track < num_tracks; track++) {
        util_dword_to_le_buf(&gcr_track_p[track * 4 * 2], 12 + max_tracks * 16 + track * (NUM_MAX_BYTES_TRACK + 2));
        util_dword_to_le_buf(&gcr_speed_p[track * 4 * 2], disk_image_speed_map(image->type, track + 1));
    }
    /* write list of track offsets */
    if (fwrite(gcr_track_p, (max_tracks * 2 * 4), 1, fsimage->fd) < 1) {
        log_error(createdisk_log, "Cannot write track header.");
        return -1;
    }
    /* write speed table */
    if (fwrite(gcr_speed_p, (max_tracks * 2 * 4), 1, fsimage->fd) < 1) {
        log_error(createdisk_log, "Cannot write speed header.");
        return -1;
    }
    memset(rawdata, 0, sizeof(rawdata));

    header.id1 = 0xa0;
    header.id2 = 0xa0;
    for (track = 1; track <= num_tracks; track++) {
        gap = disk_image_gap_size(image->type, track);
        headergap = disk_image_header_gap_size(image->type, track);
        synclen = disk_image_sync_size(image->type, track);
        gcrptr = gcr_track;
        util_word_to_le_buf(gcrptr, (uint16_t)disk_image_raw_track_size(image->type, track));
        gcrptr += 2;
        memset(gcrptr, 0x55, NUM_MAX_BYTES_TRACK);
        if (image->type == DISK_IMAGE_TYPE_G71) {
            /* the DOS would normally not touch the tracks > 35, we format them
               anyway and give them unique track numbers. This is NOT standard! */
            if (track > 77) {
                /* tracks 78 - 84 on side 2 get 78..84 */
                header.track = track;
            } else if (track > 42) {
                /* tracks on side 2 start with 36 */
                header.track = track - 7;
            } else if (track > 35) {
                /* tracks 36 - 42 on side 1 get 71..77 */
                header.track = track + 35;
            } else {
                header.track = track;
            }
        } else {
            header.track = track;
        }
        DBG(("track %d hdr track %d (%d) : ", track, header.track, disk_image_raw_track_size(image->type, track)));
        /* encode one track */
        for (sector = 0;
             sector < disk_image_sector_per_track(image->type, track);
             sector++) {
            DBG(("%d ", sector));
            header.sector = sector;
            gcr_convert_sector_to_GCR(rawdata, gcrptr, &header, headergap, synclen, CBMDOS_FDC_ERR_OK);
            gcrptr += SECTOR_GCR_SIZE_WITH_HEADER + headergap + gap + (synclen * 2);
        }
        DBG(("(gap: %d)\n", gap));
        if (fwrite((char *)gcr_track, sizeof(gcr_track), 1, fsimage->fd) < 1) {
            log_error(createdisk_log, "Cannot write track data.");
            return -1;
        }
    }
    return 0;
}


/** \brief  Create a P64 disk image on the host file system
 *
 * \param[in,out]   image   disk image
 *
 * \return 0 on success, < 0 on failure
 */
static int fsimage_create_p64(disk_image_t *image)
{
    TP64MemoryStream P64MemoryStreamInstance;
    TP64Image P64Image;
    uint8_t gcr_track[NUM_MAX_BYTES_TRACK], *gcrptr;
    unsigned int track, sector;
    fsimage_t *fsimage;
    int rc = -1;
    gcr_header_t header;
    uint8_t rawdata[256];
    int gap, headergap, synclen;

    fsimage = image->media.fsimage;

    P64ImageCreate(&P64Image);

    header.id1 = 0xa0;
    header.id2 = 0xa0;
    for (track = 1; track <= NUM_TRACKS_1541; track++) {
        gap = disk_image_gap_size(image->type, track);
        headergap = disk_image_header_gap_size(image->type, track);
        synclen = disk_image_sync_size(image->type, track);
        gcrptr = gcr_track;
        util_word_to_le_buf(gcrptr, (uint16_t)disk_image_raw_track_size(image->type, track));
        gcrptr += 2;
        memset(gcrptr, 0x55, NUM_MAX_BYTES_TRACK - 2);

        header.track = track;
        for (sector = 0;
             sector < disk_image_sector_per_track(image->type, track);
             sector++) {
            header.sector = sector;
            gcr_convert_sector_to_GCR(rawdata, gcrptr, &header, headergap, synclen, CBMDOS_FDC_ERR_OK);

            gcrptr += SECTOR_GCR_SIZE_WITH_HEADER + headergap + gap + (synclen * 2);
        }
        P64PulseStreamConvertFromGCR(&P64Image.PulseStreams[0][track << 1], (void*)&gcr_track[0], disk_image_raw_track_size(image->type, track) << 3);
    }

    P64MemoryStreamCreate(&P64MemoryStreamInstance);
    P64MemoryStreamClear(&P64MemoryStreamInstance);
    if (P64ImageWriteToStream(&P64Image, &P64MemoryStreamInstance)) {
        if (fwrite(P64MemoryStreamInstance.Data, P64MemoryStreamInstance.Size, 1, fsimage->fd) < 1) {
            log_error(createdisk_log, "Cannot write image data.");
            rc = -1;
        } else {
            rc = 0;
        }
    }
    P64MemoryStreamDestroy(&P64MemoryStreamInstance);

    P64ImageDestroy(&P64Image);

    return rc;
}

/*-----------------------------------------------------------------------*/


/** \brief  Create a disk image of type \a type called \a name on the host OS
 *
 * \param[in]   name    disk image name/path
 * \param[in]   type    disk image type
 *
 * \return  0 on success, < 0 on failure
 */
int fsimage_create(const char *name, unsigned int type)
{
    disk_image_t *image;
    fsimage_t *fsimage;
    int rc = -1;

    image = lib_malloc(sizeof(disk_image_t));
    fsimage = lib_malloc(sizeof(fsimage_t));

    image->media.fsimage = fsimage;
    image->device = DISK_IMAGE_DEVICE_FS;
    image->type = type;

    fsimage->name = lib_strdup(name);
    fsimage->fd = fopen(name, MODE_WRITE);

    if (fsimage->fd == NULL) {
        log_error(createdisk_log, "Cannot create disk image `%s'.",
                  fsimage->name);
        lib_free(fsimage->name);
        lib_free(fsimage);
        lib_free(image);
        return -1;
    }

    switch (type) {
#ifdef HAVE_X64_IMAGE
        case DISK_IMAGE_TYPE_X64:
#endif
        case DISK_IMAGE_TYPE_D64:
        case DISK_IMAGE_TYPE_D67:
        case DISK_IMAGE_TYPE_D71:
        case DISK_IMAGE_TYPE_D81:
        case DISK_IMAGE_TYPE_D80:
        case DISK_IMAGE_TYPE_D82:
        case DISK_IMAGE_TYPE_D1M:
        case DISK_IMAGE_TYPE_D2M:
        case DISK_IMAGE_TYPE_D4M:
        case DISK_IMAGE_TYPE_D90:
            rc = fsimage_create_dxx(image);
            break;
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_G71:
            rc = fsimage_create_gcr(image);
            break;
        case DISK_IMAGE_TYPE_P64:
            rc = fsimage_create_p64(image);
            break;
    }

    fclose(fsimage->fd);
    lib_free(fsimage->name);
    lib_free(fsimage);
    lib_free(image);
    return rc;
}


/** \brief  Initialize module
 *
 * In this case: just open a log
 */
void fsimage_create_init(void)
{
    createdisk_log = log_open("Disk Create");
}

