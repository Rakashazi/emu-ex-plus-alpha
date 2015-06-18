/*
 * vdrive-bam.c - Virtual disk-drive implementation. BAM specific functions.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ingo Korb <ingo@akana.de>
 *
 * Based on old code by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Jarkko Sonninen <sonninen@lut.fi>
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  Olaf Seibert <rhialto@mbfys.kun.nl>
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  pottendo <pottendo@gmx.net>
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

#include "attach.h"
#include "cbmdos.h"
#include "diskconstants.h"
#include "diskimage.h"
#include "log.h"
#include "types.h"
#include "vdrive-bam.h"
#include "vdrive-command.h"
#include "vdrive.h"

/*
    return Maximum distance from dir track to start/end of disk.

    FIXME: partition support
*/
static int vdrive_calculate_disk_half(vdrive_t *vdrive)
{
    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_2040:
            return 17 + 5;
        case VDRIVE_IMAGE_FORMAT_1571:
            return 17 + 35;
        case VDRIVE_IMAGE_FORMAT_1581:
            return 40;
        case VDRIVE_IMAGE_FORMAT_8050:
        case VDRIVE_IMAGE_FORMAT_8250:
            return 39;
        case VDRIVE_IMAGE_FORMAT_4000:
            return vdrive->num_tracks - 1;
        default:
            log_error(LOG_ERR,
                      "Unknown disk type %i.  Cannot calculate disk half.", vdrive->image_format);
    }
    return -1;
}

/*
    FIXME: partition support
*/
int vdrive_bam_alloc_first_free_sector(vdrive_t *vdrive,
                                       unsigned int *track,
                                       unsigned int *sector)
{
    unsigned int s, d, max_tracks;
    int t;

    max_tracks = vdrive_calculate_disk_half(vdrive);

    for (d = 0; d <= max_tracks; d++) {
        int max_sector;
        t = vdrive->Bam_Track - d;
#ifdef DEBUG_DRIVE
        log_error(LOG_ERR, "Allocate first free sector on track %d.", t);
#endif
        if (d && t >= 1) {
            max_sector = vdrive_get_max_sectors(vdrive, t);
            for (s = 0; s < (unsigned int)max_sector; s++) {
                if (vdrive_bam_allocate_sector(vdrive, t, s)) {
                    *track = t;
                    *sector = s;
#ifdef DEBUG_DRIVE
                    log_error(LOG_ERR,
                              "Allocate first free sector: %d,%d.", t, s);
#endif
                    return 0;
                }
            }
        }
        t = vdrive->Bam_Track + d;
#ifdef DEBUG_DRIVE
        log_error(LOG_ERR, "Allocate first free sector on track %d.", t);
#endif
        if (t <= (int)(vdrive->num_tracks)) {
            max_sector = vdrive_get_max_sectors(vdrive, t);
            if (d) {
                s = 0;
            } else if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000) {
                s = 64; /* after root directory */
            } else {
                s = max_sector; /* skip bam track */
            }
            for (; s < (unsigned int)max_sector; s++) {
                if (vdrive_bam_allocate_sector(vdrive, t, s)) {
                    *track = t;
                    *sector = s;
#ifdef DEBUG_DRIVE
                    log_error(LOG_ERR,
                              "Allocate first free sector: %d,%d.", t, s);
#endif
                    return 0;
                }
            }
        }
    }
    return -1;
}

static int vdrive_bam_alloc_down(vdrive_t *vdrive,
                                 unsigned int *track, unsigned int *sector)
{
    unsigned int max_sector, t, s;

    for (t = *track; t >= 1; t--) {
        max_sector = vdrive_get_max_sectors(vdrive, t);
        for (s = 0; s < max_sector; s++) {
            if (vdrive_bam_allocate_sector(vdrive, t, s)) {
                *track = t;
                *sector = s;
                return 0;
            }
        }
    }
    return -1;
}

static int vdrive_bam_alloc_up(vdrive_t *vdrive,
                               unsigned int *track, unsigned int *sector)
{
    unsigned int max_sector, t, s;

    for (t = *track; t <= vdrive->num_tracks; t++) {
        max_sector = vdrive_get_max_sectors(vdrive, t);
        for (s = 0; s < max_sector; s++) {
            if (vdrive_bam_allocate_sector(vdrive, t, s)) {
                *track = t;
                *sector = s;
                return 0;
            }
        }
    }
    return -1;
}

/*
    FIXME: partition support
*/
int vdrive_bam_alloc_next_free_sector(vdrive_t *vdrive,
                                      unsigned int *track,
                                      unsigned int *sector)
{
    unsigned int max_sector, i, t, s;

    if (*track == vdrive->Bam_Track) {
        if (vdrive->image_format != VDRIVE_IMAGE_FORMAT_4000 || *sector < 64) {
            return -1;
        }
    }

    /* Calculate the next sector for the current interleave */
    s = *sector + vdrive_bam_get_interleave(vdrive->image_format);
    t = *track;
    max_sector = vdrive_get_max_sectors(vdrive, t);
    if (s >= max_sector) {
        s -= max_sector;
        if (s != 0) {
            s--;
        }
    }
    /* Look for a sector on the same track */
    for (i = 0; i < max_sector; i++) {
        if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000 && *track == vdrive->Bam_Track && s < 64) {
            s = 64;
        }
        if (vdrive_bam_allocate_sector(vdrive, t, s)) {
            *track = t;
            *sector = s;
            return 0;
        }
        s++;
        if (s >= max_sector) {
            s = 0;
        }
    }
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000 && *track == vdrive->Bam_Track) {
        (*track)++;
    }

    /* Look for a sector on a close track */
    *sector = 0;
    if (*track < vdrive->Dir_Track) {
        if (vdrive_bam_alloc_down(vdrive, track, sector) == 0) {
            return 0;
        }
        *track = vdrive->Dir_Track - 1;
        if (vdrive_bam_alloc_down(vdrive, track, sector) == 0) {
            return 0;
        }
        *track = vdrive->Dir_Track + 1;
        if (vdrive_bam_alloc_up(vdrive, track, sector) == 0) {
            return 0;
        }
    } else {
        if (vdrive_bam_alloc_up(vdrive, track, sector) == 0) {
            return 0;
        }
        *track = vdrive->Dir_Track + 1;
        if (vdrive_bam_alloc_up(vdrive, track, sector) == 0) {
            return 0;
        }
        *track = vdrive->Dir_Track - 1;
        if (vdrive_bam_alloc_down(vdrive, track, sector) == 0) {
            return 0;
        }
    }
    return -1;
}

static void vdrive_bam_set(BYTE *bamp, unsigned int sector)
{
    bamp[1 + sector / 8] |= (1 << (sector % 8));
    return;
}

static void vdrive_bam_clr(BYTE *bamp, unsigned int sector)
{
    bamp[1 + sector / 8] &= ~(1 << (sector % 8));
    return;
}

static int vdrive_bam_isset(BYTE *bamp, unsigned int sector)
{
    return bamp[1 + sector / 8] & (1 << (sector % 8));
}

int vdrive_bam_allocate_chain(vdrive_t *vdrive, unsigned int t, unsigned int s)
{
    BYTE tmp[256];
    int rc;

    while (t) {
        /* Check for illegal track or sector.  */
        if (disk_image_check_sector(vdrive->image, t, s) < 0) {
            vdrive_command_set_error(vdrive, CBMDOS_IPE_ILLEGAL_TRACK_OR_SECTOR,
                                     s, t);
            return CBMDOS_IPE_ILLEGAL_TRACK_OR_SECTOR;
        }
        if (!vdrive_bam_allocate_sector(vdrive, t, s)) {
            /* The real drive does not seem to catch this error.  */
            vdrive_command_set_error(vdrive, CBMDOS_IPE_NO_BLOCK, s, t);
            return CBMDOS_IPE_NO_BLOCK;
        }
        rc = vdrive_read_sector(vdrive, tmp, t, s);
        if (rc > 0) {
            return rc;
        }
        if (rc < 0) {
            return CBMDOS_IPE_NOT_READY;
        }

        t = (int)tmp[0];
        s = (int)tmp[1];
    }
    return CBMDOS_IPE_OK;
}

/*
    FIXME: partition support
*/
static BYTE *vdrive_bam_calculate_track(vdrive_t *vdrive,
                                        unsigned int track)
{
    BYTE *bamp = NULL;
    BYTE *bam = vdrive->bam;

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_2040:
            bamp = (track <= NUM_TRACKS_1541) ?
                   &bam[BAM_BIT_MAP + 4 * (track - 1)] :
                   &bam[BAM_EXT_BIT_MAP_1541 + 4 * (track - NUM_TRACKS_1541 - 1)];
            break;
        case VDRIVE_IMAGE_FORMAT_1571:
            bamp = (track <= NUM_TRACKS_1571 / 2) ?
                   &bam[BAM_BIT_MAP + 4 * (track - 1)] :
                   &bam[0x100 + 3 * (track - NUM_TRACKS_1571 / 2 - 1) - 1];
            break;
        case VDRIVE_IMAGE_FORMAT_1581:
            bamp = (track <= BAM_TRACK_1581) ?
                   &bam[0x100 + BAM_BIT_MAP_1581 + 6 * (track - 1)] :
                   &bam[0x200 + BAM_BIT_MAP_1581 + 6 *
                        (track - BAM_TRACK_1581 - 1)];
            break;
        case VDRIVE_IMAGE_FORMAT_8050:
            {
                int i;
                for (i = 1; i < 3; i++) {
                    if (track >= bam[(i * 0x100) + 4] && track < bam[(i * 0x100) + 5]) {
                        bamp = &bam[(i * 0x100) + BAM_BIT_MAP_8050 + 5 * (track - bam[(i * 0x100) + 4])];
                        break;
                    }
                }
            }
            break;
        case VDRIVE_IMAGE_FORMAT_8250:
            {
                int i;
                for (i = 1; i < 5; i++) {
                    if (track >= bam[(i * 0x100) + 4] && track < bam[(i * 0x100) + 5]) {
                        bamp = &bam[(i * 0x100) + BAM_BIT_MAP_8050 + 5 * (track - bam[(i * 0x100) + 4])];
                        break;
                    }
                }
            }
            break;
        case VDRIVE_IMAGE_FORMAT_4000:
            bamp = &bam[0x100 + BAM_BIT_MAP_4000 + 32 * (track - 1) - 1];
            break;
        default:
            log_error(LOG_ERR, "Unknown disk type %i.  Cannot calculate BAM track.", vdrive->image_format);
    }
    return bamp;
}

static void vdrive_bam_sector_free(vdrive_t *vdrive, BYTE *bamp,
                                   unsigned int track, int add)
{
    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_2040:
        case VDRIVE_IMAGE_FORMAT_1581:
        case VDRIVE_IMAGE_FORMAT_8050:
        case VDRIVE_IMAGE_FORMAT_8250:
            *bamp += add;
            break;
        case VDRIVE_IMAGE_FORMAT_1571:
            if (track <= NUM_TRACKS_1571 / 2) {
                *bamp += add;
            } else {
                vdrive->bam[BAM_EXT_BIT_MAP_1571 + track - NUM_TRACKS_1571 / 2 - 1] += add;
            }
            break;
        case VDRIVE_IMAGE_FORMAT_4000:
            break;
        default:
            log_error(LOG_ERR, "Unknown disk type %i.  Cannot find free sector.", vdrive->image_format);
    }
}

int vdrive_bam_allocate_sector(vdrive_t *vdrive,
                               unsigned int track, unsigned int sector)
{
    BYTE *bamp;

    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000) {
        sector ^= 7;
    }

    bamp = vdrive_bam_calculate_track(vdrive, track);
    if (vdrive_bam_isset(bamp, sector)) {
        vdrive_bam_sector_free(vdrive, bamp, track, -1);
        vdrive_bam_clr(bamp, sector);
        return 1;
    }
    return 0;
}

int vdrive_bam_free_sector(vdrive_t *vdrive, unsigned int track,
                           unsigned int sector)
{
    BYTE *bamp;

    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000) {
        sector ^= 7;
    }

    bamp = vdrive_bam_calculate_track(vdrive, track);
    if (!(vdrive_bam_isset(bamp, sector))) {
        vdrive_bam_set(bamp, sector);
        vdrive_bam_sector_free(vdrive, bamp, track, 1);
        return 1;
    }
    return 0;
}

void vdrive_bam_clear_all(vdrive_t *vdrive)
{
    BYTE *bam = vdrive->bam;

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1541:
            memset(bam + BAM_EXT_BIT_MAP_1541, 0, 4 * 5);
        /* fallthrough */
        case VDRIVE_IMAGE_FORMAT_2040:
            memset(bam + BAM_BIT_MAP, 0, 4 * NUM_TRACKS_1541);
            break;
        case VDRIVE_IMAGE_FORMAT_1571:
            memset(bam + BAM_BIT_MAP, 0, 4 * NUM_TRACKS_1571 / 2);
            memset(bam + BAM_EXT_BIT_MAP_1571, 0, NUM_TRACKS_1571 / 2);
            memset(bam + 0x100, 0, 3 * NUM_TRACKS_1571 / 2);
            break;
        case VDRIVE_IMAGE_FORMAT_1581:
            memset(bam + 0x100 + BAM_BIT_MAP_1581, 0, 6 * NUM_TRACKS_1581 / 2);
            memset(bam + 0x200 + BAM_BIT_MAP_1581, 0, 6 * NUM_TRACKS_1581 / 2);
            break;
        case VDRIVE_IMAGE_FORMAT_8050:
            memset(bam + 0x100 + BAM_BIT_MAP_8050, 0, 0x100 - BAM_BIT_MAP_8050);
            memset(bam + 0x200 + BAM_BIT_MAP_8050, 0, 0x100 - BAM_BIT_MAP_8050);
            break;
        case VDRIVE_IMAGE_FORMAT_8250:
            memset(bam + 0x100 + BAM_BIT_MAP_8250, 0, 0x100 - BAM_BIT_MAP_8250);
            memset(bam + 0x200 + BAM_BIT_MAP_8250, 0, 0x100 - BAM_BIT_MAP_8250);
            memset(bam + 0x300 + BAM_BIT_MAP_8250, 0, 0x100 - BAM_BIT_MAP_8250);
            memset(bam + 0x400 + BAM_BIT_MAP_8250, 0, 0x100 - BAM_BIT_MAP_8250);
            break;
        case VDRIVE_IMAGE_FORMAT_4000:
            memset(bam + 0x100 + BAM_BIT_MAP_4000, 255, 255 * 32);
            break;
        default:
            log_error(LOG_ERR,
                      "Unknown disk type %i.  Cannot clear BAM.", vdrive->image_format);
    }
}

/* Should be removed some day.  */
static int mystrncpy(BYTE *d, BYTE *s, int n)
{
    while (n-- && *s) {
        *d++ = *s++;
    }
    return (n);
}

void vdrive_bam_create_empty_bam(vdrive_t *vdrive, const char *name, BYTE *id)
{
    /* Create Disk Format for 1541/1571/1581/2040/4000 disks.  */
    memset(vdrive->bam, 0, vdrive->bam_size);
    if (vdrive->image_format != VDRIVE_IMAGE_FORMAT_8050
        && vdrive->image_format != VDRIVE_IMAGE_FORMAT_8250) {
        vdrive->bam[0] = vdrive->Dir_Track;
        vdrive->bam[1] = vdrive->Dir_Sector;
        /* position 2 will be overwritten later for 2040/1581/4000 */
        vdrive->bam[2] = 65;

        if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1571) {
            vdrive->bam[3] = 0x80;
        }

        memset(vdrive->bam + vdrive->bam_name, 0xa0,
               (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1581
                || vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000) ? 25 : 27);
        mystrncpy(vdrive->bam + vdrive->bam_name, (BYTE *)name, 16);
        mystrncpy(vdrive->bam + vdrive->bam_id, id, 2);
    }

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_2040:
            vdrive->bam[0x02] = 0x01;
            vdrive->bam[0xa4] = 0x20;
            vdrive->bam[0xa5] = 0x20;
            break;
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_1571:
            vdrive->bam[BAM_VERSION_1541] = 50;
            vdrive->bam[BAM_VERSION_1541 + 1] = 65;
            break;
        case VDRIVE_IMAGE_FORMAT_1581:
            vdrive->bam[0x02] = 68;
            /* Setup BAM linker.  */
            vdrive->bam[0x100] = vdrive->Bam_Track;
            vdrive->bam[0x100 + 1] = 2;
            vdrive->bam[0x200] = 0;
            vdrive->bam[0x200 + 1] = 0xff;
            /* Setup BAM version.  */
            vdrive->bam[BAM_VERSION_1581] = 51;
            vdrive->bam[BAM_VERSION_1581 + 1] = 68;
            vdrive->bam[0x100 + 2] = 68;
            vdrive->bam[0x100 + 3] = 0xbb;
            vdrive->bam[0x100 + 4] = id[0];
            vdrive->bam[0x100 + 5] = id[1];
            vdrive->bam[0x100 + 6] = 0xc0;
            vdrive->bam[0x200 + 2] = 68;
            vdrive->bam[0x200 + 3] = 0xbb;
            vdrive->bam[0x200 + 4] = id[0];
            vdrive->bam[0x200 + 5] = id[1];
            vdrive->bam[0x200 + 6] = 0xc0;
            break;
        case VDRIVE_IMAGE_FORMAT_8050:
        case VDRIVE_IMAGE_FORMAT_8250:
            /* the first BAM block with the disk name is at 39/0, but it
               points to the first bitmap BAM block at 38/0 ...
               Only the last BAM block at 38/3 resp. 38/9 points to the
               first dir block at 39/1 */
            vdrive->bam[0] = 38;
            vdrive->bam[1] = 0;
            vdrive->bam[2] = 67;
            /* byte 3-5 unused */
            /* bytes 6- disk name + id + version */
            memset(vdrive->bam + vdrive->bam_name, 0xa0, 27);
            mystrncpy(vdrive->bam + vdrive->bam_name, (BYTE *)name, 16);
            mystrncpy(vdrive->bam + vdrive->bam_id, id, 2);
            vdrive->bam[BAM_VERSION_8050] = 50;
            vdrive->bam[BAM_VERSION_8050 + 1] = 67;
            /* rest of first block unused */

            /* first bitmap block at 38/0 */
            vdrive->bam[0x100] = 38;
            vdrive->bam[0x100 + 1] = 3;
            vdrive->bam[0x100 + 2] = 67;
            vdrive->bam[0x100 + 4] = 1; /* In this block from track ... */
            vdrive->bam[0x100 + 5] = 51; /* till excluding track ... */

            if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_8050) {
                /* second bitmap block at 38/3 */
                vdrive->bam[0x200] = 39;
                vdrive->bam[0x200 + 1] = 1;
                vdrive->bam[0x200 + 2] = 67;
                vdrive->bam[0x200 + 4] = 51; /* In this block from track ... */
                vdrive->bam[0x200 + 5] = 78; /* till excluding track ... */
            } else
            if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_8250) {
                /* second bitmap block at 38/3 */
                vdrive->bam[0x200] = 38;
                vdrive->bam[0x200 + 1] = 6;
                vdrive->bam[0x200 + 2] = 67;
                vdrive->bam[0x200 + 4] = 51; /* In this block from track ... */
                vdrive->bam[0x200 + 5] = 101; /* till excluding track ... */
                /* third bitmap block at 38/6 */
                vdrive->bam[0x300] = 38;
                vdrive->bam[0x300 + 1] = 9;
                vdrive->bam[0x300 + 2] = 67;
                vdrive->bam[0x300 + 4] = 101; /* In this block from track ... */
                vdrive->bam[0x300 + 5] = 151; /* till excluding track ... */
                /* fourth bitmap block at 38/9 */
                vdrive->bam[0x400] = 39;
                vdrive->bam[0x400 + 1] = 1;
                vdrive->bam[0x400 + 2] = 67;
                vdrive->bam[0x400 + 4] = 151; /* In this block from track ... */
                vdrive->bam[0x400 + 5] = 155; /* till excluding track ... */
            }
            break;
        case VDRIVE_IMAGE_FORMAT_4000:
            vdrive->bam[0x02] = 72;
            /* Setup BAM version.  */
            vdrive->bam[BAM_VERSION_4000] = 49;
            vdrive->bam[BAM_VERSION_4000 + 1] = 72;
            vdrive->bam[0x20] = vdrive->Bam_Track;
            vdrive->bam[0x21] = vdrive->Bam_Sector;
            vdrive->bam[0x100 + 2] = 72;
            vdrive->bam[0x100 + 3] = ~72;
            vdrive->bam[0x100 + 4] = id[0];
            vdrive->bam[0x100 + 5] = id[1];
            vdrive->bam[0x100 + 6] = 0xc0;
            vdrive->bam[0x100 + 8] = vdrive->num_tracks;
            break;
        default:
            log_error(LOG_ERR,
                      "Unknown disk type %i.  Cannot create BAM.",
                      vdrive->image_format);
    }
    return;
}

int vdrive_bam_get_disk_id(unsigned int unit, BYTE *id)
{
    vdrive_t *vdrive;

    vdrive = file_system_get_vdrive(unit);

    if (vdrive == NULL || id == NULL || vdrive->bam == NULL) {
        return -1;
    }

    memcpy(id, vdrive->bam + vdrive->bam_id, 2);

    return 0;
}

int vdrive_bam_set_disk_id(unsigned int unit, BYTE *id)
{
    vdrive_t *vdrive;

    vdrive = file_system_get_vdrive(unit);

    if (vdrive == NULL || id == NULL || vdrive->bam == NULL) {
        return -1;
    }

    memcpy(vdrive->bam + vdrive->bam_id, id, 2);

    return 0;
}

int vdrive_bam_get_interleave(unsigned int type)
{
    /* Note: Values for 2040/8050/8250 determined empirically */
    switch (type) {
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_2040:
            return 10;
        case VDRIVE_IMAGE_FORMAT_1571:
            return 6;
        case VDRIVE_IMAGE_FORMAT_1581:
            return 1;
        case VDRIVE_IMAGE_FORMAT_8050:
            return 6;
        case VDRIVE_IMAGE_FORMAT_8250:
            return 7;
        case VDRIVE_IMAGE_FORMAT_4000:
            return 1;
        default:
            log_error(LOG_ERR, "Unknown disk type %i.  Using interleave 10.", type);
            return 10;
    }
}

/* ------------------------------------------------------------------------- */

/*
 * Load/Store BAM Image.
 */
/*
    FIXME: partition support
*/

/* probably we should make a list with BAM blocks for each drive type... (AF)*/
int vdrive_bam_read_bam(vdrive_t *vdrive)
{
    int err = -1, i;

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_2040:
        case VDRIVE_IMAGE_FORMAT_1541:
            err = vdrive_read_sector(vdrive, vdrive->bam, BAM_TRACK_1541, BAM_SECTOR_1541);
            break;
        case VDRIVE_IMAGE_FORMAT_1571:
            err = vdrive_read_sector(vdrive, vdrive->bam, BAM_TRACK_1571, BAM_SECTOR_1571);
            if (err != 0) {
                break;
            }
            err = vdrive_read_sector(vdrive, vdrive->bam + 256, BAM_TRACK_1571 + 35, BAM_SECTOR_1571);
            break;
        case VDRIVE_IMAGE_FORMAT_1581:
            err = vdrive_read_sector(vdrive, vdrive->bam, BAM_TRACK_1581, BAM_SECTOR_1581);
            if (err != 0) {
                break;
            }
            err = vdrive_read_sector(vdrive, vdrive->bam + 256, BAM_TRACK_1581, BAM_SECTOR_1581 + 1);
            if (err != 0) {
                break;
            }
            err = vdrive_read_sector(vdrive, vdrive->bam + 512, BAM_TRACK_1581, BAM_SECTOR_1581 + 2);
            break;
        case VDRIVE_IMAGE_FORMAT_8050:
        case VDRIVE_IMAGE_FORMAT_8250:
            err = vdrive_read_sector(vdrive, vdrive->bam, BAM_TRACK_8050, BAM_SECTOR_8050);
            if (err != 0) {
                break;
            }
            err = vdrive_read_sector(vdrive, vdrive->bam + 256, BAM_TRACK_8050 - 1, BAM_SECTOR_8050);
            if (err != 0) {
                break;
            }
            err = vdrive_read_sector(vdrive, vdrive->bam + 512, BAM_TRACK_8050 - 1, BAM_SECTOR_8050 + 3);
            if (err != 0) {
                break;
            }

            if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_8050) {
                break;
            }

            err = vdrive_read_sector(vdrive, vdrive->bam + 768, BAM_TRACK_8050 - 1, BAM_SECTOR_8050 + 6);
            if (err != 0) {
                break;
            }
            err = vdrive_read_sector(vdrive, vdrive->bam + 1024, BAM_TRACK_8050 - 1, BAM_SECTOR_8050 + 9);
            break;
        case VDRIVE_IMAGE_FORMAT_4000:
            for (i = 0; i < 33; i++) {
                err = vdrive_read_sector(vdrive, vdrive->bam + i * 256, BAM_TRACK_4000, BAM_SECTOR_4000 + i);
                if (err != 0) {
                    break;
                }
            }
            break;
        default:
            log_error(LOG_ERR, "Unknown disk type %i.  Cannot read BAM.", vdrive->image_format);
    }

    if (err < 0) {
        return CBMDOS_IPE_NOT_READY;
    }

    return err;
}

/* Temporary hack.  */
int vdrive_bam_reread_bam(unsigned int unit)
{
    return vdrive_bam_read_bam(file_system_get_vdrive(unit));
}
/*
    FIXME: partition support
*/

int vdrive_bam_write_bam(vdrive_t *vdrive)
{
    int err = -1, i;

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_2040:
            err = vdrive_write_sector(vdrive, vdrive->bam, BAM_TRACK_1541, BAM_SECTOR_1541);
            break;
        case VDRIVE_IMAGE_FORMAT_1571:
            err = vdrive_write_sector(vdrive, vdrive->bam, BAM_TRACK_1571, BAM_SECTOR_1571);
            err |= vdrive_write_sector(vdrive, vdrive->bam + 256, BAM_TRACK_1571 + 35, BAM_SECTOR_1571);
            break;
        case VDRIVE_IMAGE_FORMAT_1581:
            err = vdrive_write_sector(vdrive, vdrive->bam, BAM_TRACK_1581, BAM_SECTOR_1581);
            err |= vdrive_write_sector(vdrive, vdrive->bam + 256, BAM_TRACK_1581, BAM_SECTOR_1581 + 1);
            err |= vdrive_write_sector(vdrive, vdrive->bam + 512, BAM_TRACK_1581, BAM_SECTOR_1581 + 2);
            break;
        case VDRIVE_IMAGE_FORMAT_8050:
        case VDRIVE_IMAGE_FORMAT_8250:
            err = vdrive_write_sector(vdrive, vdrive->bam, BAM_TRACK_8050, BAM_SECTOR_8050);
            err |= vdrive_write_sector(vdrive, vdrive->bam + 256, BAM_TRACK_8050 - 1, BAM_SECTOR_8050);
            err |= vdrive_write_sector(vdrive, vdrive->bam + 512, BAM_TRACK_8050 - 1, BAM_SECTOR_8050 + 3);

            if (vdrive->image_format == 8050) {
                break;
            }

            err |= vdrive_write_sector(vdrive, vdrive->bam + 768, BAM_TRACK_8050 - 1, BAM_SECTOR_8050 + 6);
            err |= vdrive_write_sector(vdrive, vdrive->bam + 1024, BAM_TRACK_8050 - 1, BAM_SECTOR_8050 + 9);
            break;
        case VDRIVE_IMAGE_FORMAT_4000:
            err = 0;
            for (i = 0; i < 33; i++) {
                err |= vdrive_write_sector(vdrive, vdrive->bam + i * 256, BAM_TRACK_4000, BAM_SECTOR_4000 + i);
            }
            break;
        default:
            log_error(LOG_ERR, "Unknown disk type %i.  Cannot read BAM.", vdrive->image_format);
    }
    return err;
}

/* ------------------------------------------------------------------------- */

/*
 * Return the number of free blocks on disk.
 */

unsigned int vdrive_bam_free_block_count(vdrive_t *vdrive)
{
    unsigned int blocks, i, j;

    for (blocks = 0, i = 1; i <= vdrive->num_tracks; i++) {
        switch (vdrive->image_format) {
            case VDRIVE_IMAGE_FORMAT_2040:
            case VDRIVE_IMAGE_FORMAT_1541:
                if (i != vdrive->Dir_Track) {
                    blocks += (i <= NUM_TRACKS_1541) ?
                              vdrive->bam[BAM_BIT_MAP + 4 * (i - 1)] :
                              vdrive->bam[BAM_EXT_BIT_MAP_1541 + 4 * (i - NUM_TRACKS_1541 - 1)];
                }
                break;
            case VDRIVE_IMAGE_FORMAT_1571:
                if (i != vdrive->Dir_Track && i != vdrive->Dir_Track + 35) {
                    blocks += (i <= NUM_TRACKS_1571 / 2) ?
                              vdrive->bam[BAM_BIT_MAP + 4 * (i - 1)] :
                              vdrive->bam[BAM_EXT_BIT_MAP_1571 + i - 1 - NUM_TRACKS_1571 / 2];
                }
                break;
            case VDRIVE_IMAGE_FORMAT_1581:
                if (i != vdrive->Dir_Track) {
                    blocks += (i <= NUM_TRACKS_1581 / 2) ?
                              vdrive->bam[BAM_BIT_MAP_1581 + 256 + 6 * (i - 1)] :
                              vdrive->bam[BAM_BIT_MAP_1581 + 512 + 6 * (i - 1 - NUM_TRACKS_1581 / 2)];
                }
                break;
            case VDRIVE_IMAGE_FORMAT_8050:
                if (i != vdrive->Dir_Track) {
                    int j;
                    for (j = 1; j < 3; j++) {
                        if (i >= vdrive->bam[(j * 0x100) + 4] && i < vdrive->bam[(j * 0x100) + 5]) {
                            blocks += vdrive->bam[(j * 0x100) + BAM_BIT_MAP_8050 + 5 * (i - vdrive->bam[(j * 0x100) + 4])];
                            break;
                        }
                    }
                }
                break;
            case VDRIVE_IMAGE_FORMAT_8250:
                if (i != vdrive->Dir_Track) {
                    int j;
                    for (j = 1; j < 5; j++) {
                        if (i >= vdrive->bam[(j * 0x100) + 4] && i < vdrive->bam[(j * 0x100) + 5]) {
                            blocks += vdrive->bam[(j * 0x100) + BAM_BIT_MAP_8050 + 5 * (i - vdrive->bam[(j * 0x100) + 4])];
                            break;
                        }
                    }
                }
                break;
            case VDRIVE_IMAGE_FORMAT_4000:
                for (j = ((i == vdrive->Bam_Track) ? 64 : 0); j < 256; j++) {
                    blocks += (vdrive->bam[BAM_BIT_MAP_4000 + 256 + 32 * (i - 1) + j / 8] >> (j % 8)) & 1;
                }
                break;
            default:
                log_error(LOG_ERR,
                          "Unknown disk type %i.  Cannot calculate free sectors.",
                          vdrive->image_format);
        }
    }
    return blocks;
}
