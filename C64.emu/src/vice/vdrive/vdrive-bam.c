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

/* #define DEBUG_DRIVE */

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
            return 39;
        case VDRIVE_IMAGE_FORMAT_8250:
            return 39 + 78;
        case VDRIVE_IMAGE_FORMAT_4000:
            return vdrive->num_tracks - 1;
        case VDRIVE_IMAGE_FORMAT_9000:
            return vdrive->num_tracks - 1;
        default:
            log_error(LOG_ERR,
                      "Unknown disk type %u.  Cannot calculate disk half.",
                      vdrive->image_format);
    }
    return -1;
}

/*
This function is used by the next 3 to find an available sector in
a single track. Typically this would be a simple loop, but the D9090/60
adds another dimension (heads) to the search. 
It only updates the sector if it finds something (returns 0).
*/
static int vdrive_bam_alloc_worker(vdrive_t *vdrive,
                                   unsigned int track, unsigned int *sector)
{
    unsigned int max_sector, max_sector_all, s, h, s2, h2;

    max_sector = vdrive_get_max_sectors_per_head(vdrive, track);
    max_sector_all = vdrive_get_max_sectors(vdrive, track);
    /* start at supplied sector - but it is usually always 0 */
    s = *sector % max_sector;
    h = (*sector / max_sector) * max_sector;
    /* go through all groups, 1 round for most CBM drives */
    for (h2 = 0; h2 < max_sector_all; h2 += max_sector) {
        /* scan sectors in group */
        for (s2 = 0; s2 < max_sector; s2++) {
            /* skip the first 64 sectors of track 1 on DNP */
            if (s < 64 && vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000
                && track == vdrive->Dir_Track) {
                s = 64; 
            }
            if (vdrive_bam_allocate_sector(vdrive, track, s + h)) {
                *sector = s + h;
                return 0;
            }
            s++;
            if (s >= max_sector) {
                s = 0;
            }
        }
        /* for D9090/60 only move on to next track if we scanned all
            the sector groups */
        h += max_sector;
        if (h >= max_sector_all) {
            h = 0;
        }
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
    unsigned int d, max_tracks;
    unsigned int origt = *track, origs = *sector;
    int t;

    /* For D9090/60, it simply uses a toggle to decide on which side of
       the directory to start, and then uses the "next sector" routine.
       see $fb81 in ROM */
    /* this obviously means the behavior selecting the first sector of the
       drive depends on what it did in the past since reset */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_9000) {
        vdrive->d90toggle ^= 1;
        *track = vdrive->Dir_Track;
        *sector = 0;
        if (vdrive->d90toggle) {
            (*track)++;
        } else {
            (*track)--;
        }
        return vdrive_bam_alloc_next_free_sector(vdrive, track, sector);
    }

    max_tracks = vdrive_calculate_disk_half(vdrive);
    /* this is okay since the D9090/60 code doesn't use this algorithm */
    *sector = 0;

    /* For everything else, do the basic butterfly search */
    for (d = 0; d <= max_tracks; d++) {
        t = vdrive->Dir_Track - d;
#ifdef DEBUG_DRIVE
        log_message(LOG_DEFAULT, "Allocate first free sector on track %d.", t);
#endif
        if (d && t >= 1) {
            if (!vdrive_bam_alloc_worker(vdrive, t, sector)) {
                *track = t;
#ifdef DEBUG_DRIVE
                log_message(LOG_DEFAULT,
                            "Allocate first free sector: %d,%u.", t, s);
#endif
                return 0;
            }
        }
        t = vdrive->Dir_Track + d;
#ifdef DEBUG_DRIVE
        log_message(LOG_DEFAULT, "Allocate first free sector on track %d.", t);
#endif
        if (t <= (int)(vdrive->num_tracks)) {
            if (d || vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000) {
                if (!vdrive_bam_alloc_worker(vdrive, t, sector)) {
                    *track = t;
#ifdef DEBUG_DRIVE
                    log_message(LOG_DEFAULT,
                                "Allocate first free sector: %d,%u.", t, s);
#endif
                    return 0;
                }
            }
        }
    }

    /* nothing, no space, recover saved variables and leave */
    *track = origt;
    *sector = origs;
    return -1;
}

/* add interleave to current sector and adjust for overflow */
int vdrive_bam_alloc_add_interleave(vdrive_t *vdrive,
                                    unsigned int track,
                                    unsigned int sector,
                                    unsigned int interleave)
{
    unsigned int max_sector, max_sector_all, s, h;

    /* Calculate the next sector for the current interleave */
    max_sector = vdrive_get_max_sectors_per_head(vdrive, track);
    max_sector_all = vdrive_get_max_sectors(vdrive, track);
    /* the starting sector may be out of bounds on the new track, so
       adjust head accordingly and follow through */
    if (sector >= max_sector_all) {
        s = sector;
        h = 0;
    } else {
        s = sector % max_sector;
        h = (sector / max_sector) * max_sector;
    }
    /* add the interleave and adjust if we go over */
    s = s + interleave;
    if (s >= max_sector) {
        s -= max_sector;
        if (s != 0) {
            s--;
        }
    }
    return s + h;
}

/* starting from the currently used track/sector, look for a new sector
downwards */
static int vdrive_bam_alloc_down(vdrive_t *vdrive,
                                 unsigned int *track, unsigned int *sector,
                                 unsigned int interleave)
{
    unsigned int t, s;

    /* scan downwards */
    for (t = *track; t >= 1; t--) {
        /* find next sector on this track based on interleave */
        s = vdrive_bam_alloc_add_interleave(vdrive, t, *sector, interleave);
        if (!vdrive_bam_alloc_worker(vdrive, t, &s)) {
            *track = t;
            *sector = s;
            return 0;
        }
    }
    return -1;
}

/* starting from the currently used track/sector, look for a new sector
upwards */
static int vdrive_bam_alloc_up(vdrive_t *vdrive,
                               unsigned int *track, unsigned int *sector,
                               unsigned int interleave)
{
    unsigned int t, s;

    /* scan upwards */
    for (t = *track; t <= vdrive->num_tracks; t++) {
        /* find next sector on this track based on interleave */
        s = vdrive_bam_alloc_add_interleave(vdrive, t, *sector, interleave);
        if (!vdrive_bam_alloc_worker(vdrive, t, &s)) {
            *track = t;
            *sector = s;
            return 0;
        }
    }
    return -1;
}

/* resets the "sector" to zero, but keeps the "head" value; for D9090/60 */
static int vdrive_bam_alloc_next_free_sector_reset(vdrive_t *vdrive,
                                                   unsigned int track,
                                                   unsigned int sector,
                                                   unsigned int interleave)
{
    unsigned int max_sector, s, h;

    s = vdrive_bam_alloc_add_interleave(vdrive, track, sector, interleave);
    max_sector = vdrive_get_max_sectors_per_head(vdrive, track);
    h = (s / max_sector) * max_sector;
    return h;
}

/*
    FIXME: partition support
*/
/* function reworked to use smaller functions above and to behave like DOS
code */
int vdrive_bam_alloc_next_free_sector_interleave(vdrive_t *vdrive,
                                                 unsigned int *track,
                                                 unsigned int *sector,
                                                 unsigned int interleave)
{
    unsigned int split = vdrive->Dir_Track;
    unsigned int origt = *track, origs = *sector;
    unsigned int s;
    int pass;

    /* Check if we are dealing with the directory track */
    /* I dislike inverse logic, we will just make it readable */
    if (*track == vdrive->Dir_Track) {
        if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_9000) {
            /* allowed on D9090/60 */
        } else if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000 && *sector >= 64) {
            /* allowed on DNP as long as sector >= 64 */
        } else {
            /* everything else: no */
            return -1;
        }
    }

    /* find next sector on this track based on interleave */
    s = vdrive_bam_alloc_add_interleave(vdrive, *track, *sector, interleave);
    /* starting from there, see if there is an available sector */
    if (!vdrive_bam_alloc_worker(vdrive, *track, &s)) {
        *sector = s;
        return 0;
    }

    /* nothing here; if DNP, move on to next track if we are track 1 */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000
        && *track == vdrive->Dir_Track) {
        (*track)++;
        /* reset sector position */
        *sector = vdrive_bam_alloc_next_free_sector_reset(vdrive, *track, *sector, interleave);
    }

    /* use a multi-pass approach here just like the DOS code */
    for (pass = 0; pass < 3; pass++) {
        /* on the first pass, look downward if we are already below the dir track */
        /* on subsequence passes, we start one below the dir track and look down */
        /* DNP goes here second */
        if (*track > 0 && *track < split) {
            if (vdrive_bam_alloc_down(vdrive, track, sector, interleave) == 0) {
               return 0;
            }
            /* For DNP, at this point, there is no space, leave search */
            if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000) {
                break;
            }
            /* after first pass, reset the starting track, and set sector to 0 */
            *track = split + 1;
            /* reset sector position */
            *sector = vdrive_bam_alloc_next_free_sector_reset(vdrive, *track, *sector, interleave);
        } else if (*track >= split) {
        /* on the first pass, look upward if we are already above the dir track */
        /* on subsequence passes, we start one above the dir track and look up */
        /* DNP goes here first */
            if (vdrive_bam_alloc_up(vdrive, track, sector, interleave) == 0) {
                return 0;
            }
            /* For DNP, set the new split point to the original start track */
            if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000) {
                split = *track;
            }
            /* after first pass, reset the starting track, and set sector to 0 */
            *track = split - 1;
            /* reset sector position */
            *sector = vdrive_bam_alloc_next_free_sector_reset(vdrive, *track, *sector, interleave);
        }
    }

    /* For D9090/60, when all the other space is full, we can use the
       directory track */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_9000) {
        /* start at sector 10 (interleave), as head gets set back to 0 */
        *sector = 10;
        *track = vdrive->Dir_Track;
        if (!vdrive_bam_alloc_worker(vdrive, *track, sector)) {
            return 0;
        }
    }

    /* nothing, no space, recover saved variables and leave */
    *track = origt;
    *sector = origs;
    return -1;
}

/*
    FIXME: partition support
*/
/* function reworked to use smaller functions above and to behave like DOS
code */
int vdrive_bam_alloc_next_free_sector(vdrive_t *vdrive,
                                      unsigned int *track,
                                      unsigned int *sector)
{
    return vdrive_bam_alloc_next_free_sector_interleave(vdrive, track,
        sector, vdrive_bam_get_interleave(vdrive->image_format));
}

static void vdrive_bam_set(uint8_t *bamp, unsigned int sector)
{
    bamp[1 + sector / 8] |= (1 << (sector % 8));
    return;
}

static void vdrive_bam_clr(uint8_t *bamp, unsigned int sector)
{
    bamp[1 + sector / 8] &= ~(1 << (sector % 8));
    return;
}

/* this function used to be used by c1541, but it is too particular to
drives which have the block count in the bam entries */
static int vdrive_bam_isset(uint8_t *bamp, unsigned int sector)
{
    return bamp[1 + sector / 8] & (1 << (sector % 8));
}

/* allocate the chain of sectors given a track and sector starting point */
int vdrive_bam_allocate_chain(vdrive_t *vdrive, unsigned int t, unsigned int s)
{
    uint8_t tmp[256];
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

/* save as above, but stops when track is 255; this is what they used in the
D9090/60 for the BAM */
int vdrive_bam_allocate_chain_255(vdrive_t *vdrive, unsigned int t, unsigned int s)
{
    uint8_t tmp[256];
    int rc;

    while (t != 255) {
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

/** \brief  Get pointer to the BAM entry for \a track in \a vdrive's image
 *
 * A CBMDOS BAM entry consists of a single byte indication the number of free
 * sectors (unreliable since there's a lot of disks out there which set this
 * to 0 to get a "0 blocks free."), followed by a bitmap of free/used sectors.
 * The size of the bitmap depends on the image type, 1541 and 1571 have three
 * bytes (enough for 21 sectors), while 1581 has 5 bytes (40 sectors).
 *
 * \param[in]   vdrive  vdrive object
 * \param[in]   track   track number
 * \param[in]   sector  sector number ( needed for D9090/60)
 *
 * \return      pointer to BAM entry
 *
 *(  FIXME: partition support
 */
static uint8_t *vdrive_bam_get_track_entry(vdrive_t *vdrive, unsigned int track,
                                           unsigned int sector)
{
    uint8_t *bamp = NULL;
    uint8_t *bam = vdrive->bam;

    /* D9090/60 has track 0, and it has a BAM entry */
    if (track == 0 && vdrive->image_format != VDRIVE_IMAGE_FORMAT_9000) {
        log_error(LOG_ERR, "invalid track number: 0");
        return NULL;
    }

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
        case VDRIVE_IMAGE_FORMAT_9000:
            {
                int i;
                sector = sector >> 5;
                for (i = 1; i < (vdrive->bam_size >> 8); i++) {
                    if (track >= bam[(i * 0x100) + 4] && track < bam[(i * 0x100) + 5]) {
                        bamp = &bam[(i * 0x100) + BAM_BIT_MAP_9000 + 5
                            * ((track - bam[(i * 0x100) + 4])
                            * (vdrive->image->sectors >> 5) + sector)];
                        break;
                    }
                }
            }
            break;
        default:
            log_error(LOG_ERR, "Unknown disk type %u.  Cannot calculate BAM track.",
                    vdrive->image_format);
    }
    return bamp;
}

static void vdrive_bam_sector_free(vdrive_t *vdrive, uint8_t *bamp,
                                   unsigned int track, int add)
{
    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_2040:
        case VDRIVE_IMAGE_FORMAT_1581:
        case VDRIVE_IMAGE_FORMAT_8050:
        case VDRIVE_IMAGE_FORMAT_8250:
        case VDRIVE_IMAGE_FORMAT_9000:
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
            log_error(LOG_ERR, "Unknown disk type %u.  Cannot find free sector.",
                    vdrive->image_format);
    }
}

int vdrive_bam_allocate_sector(vdrive_t *vdrive,
                               unsigned int track, unsigned int sector)
{
    uint8_t *bamp;

    /* Tracks > 70 don't go into the (regular) BAM on 1571 */
    if ((track > NUM_TRACKS_1571) && (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1571)) {
        return 0;
    }
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000) {
        sector ^= 7;
    }

    bamp = vdrive_bam_get_track_entry(vdrive, track, sector);
    /* D9090/60 groups 32 sectors per bam group */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_9000) {
        sector &= 31;
    }
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
    uint8_t *bamp;

    /* Tracks > 70 don't go into the (regular) BAM on 1571 */
    if ((track > NUM_TRACKS_1571) && (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1571)) {
        return 0;
    }
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000) {
        sector ^= 7;
    }

    bamp = vdrive_bam_get_track_entry(vdrive, track, sector);
    /* D9090/60 groups 32 sectors per bam group */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_9000) {
        sector &= 31;
    }
    if (!(vdrive_bam_isset(bamp, sector))) {
        vdrive_bam_set(bamp, sector);
        vdrive_bam_sector_free(vdrive, bamp, track, 1);
        return 1;
    }
    return 0;
}

/* check to see if a sector is allocated */
/* made for c1541 so it can keep out of the bitmaps since they aren't standard
   between devices. */
/* return of 1 if yes, 0 if not, -1 if failed (1571 > 70 tracks) */
int vdrive_bam_is_sector_allocated(struct vdrive_s *vdrive,
                                  unsigned int track, unsigned int sector)
{
    uint8_t *bamp;

    /* Tracks > 70 don't go into the (regular) BAM on 1571 */
    if ((track > NUM_TRACKS_1571) && (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1571)) {
        return -1;
    }
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000) {
        sector ^= 7;
    }
    bamp = vdrive_bam_get_track_entry(vdrive, track, sector);
    /* D9090/60 groups 32 sectors per bam group */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_9000) {
        sector &= 31;
    }
    if (bamp && !vdrive_bam_isset(bamp, sector)) return 1;
    return 0;
}

void vdrive_bam_clear_all(vdrive_t *vdrive)
{
    uint8_t *bam = vdrive->bam;

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
        case VDRIVE_IMAGE_FORMAT_9000:
            {
                int i;
                for (i = 0x100; i < vdrive->bam_size; i += 0x100) {
                    memset(bam + i + BAM_BIT_MAP_9000, 0, 0x100 - BAM_BIT_MAP_9000);
                }
            }
            break;
        default:
            log_error(LOG_ERR,
                      "Unknown disk type %u.  Cannot clear BAM.",
                      vdrive->image_format);
    }
}

/* FIXME:   Should be removed some day.
 *
 * Fixed up a little bit to behave more like strncpy(3), but it's still screwed.
 */
static uint8_t *mystrncpy(uint8_t *d, const uint8_t *s, size_t n)
{
    while (n > 0 && *s != 0) {
        *d++ = *s++;
        n--;
    }
    /* libc's strncpy(3) would add a 0x00 here if there's space for it, so the
     * 'mystrncpy() name is a bit misleading and might lead to incorrect
     * assumptions when using this code.
     * Maybe call this `petscii_strncpy` or so?
     */
    return d;
}

void vdrive_bam_create_empty_bam(vdrive_t *vdrive, const char *name, uint8_t *id)
{
    /* Create Disk Format for 1541/1571/1581/2040/4000 disks.  */
    memset(vdrive->bam, 0, vdrive->bam_size);
    if (vdrive->image_format != VDRIVE_IMAGE_FORMAT_8050
        && vdrive->image_format != VDRIVE_IMAGE_FORMAT_8250
        && vdrive->image_format != VDRIVE_IMAGE_FORMAT_9000) {
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
        mystrncpy(vdrive->bam + vdrive->bam_name, (const uint8_t *)name, 16U);
        mystrncpy(vdrive->bam + vdrive->bam_id, id, 2U);
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
            vdrive->bam[0] = BAM_TRACK_8050;
            vdrive->bam[1] = BAM_SECTOR_8050;
            vdrive->bam[2] = 67;
            /* byte 3-5 unused */
            /* bytes 6- disk name + id + version */
            memset(vdrive->bam + vdrive->bam_name, 0xa0, 27);
            mystrncpy(vdrive->bam + vdrive->bam_name, (const uint8_t *)name, 16U);
            mystrncpy(vdrive->bam + vdrive->bam_id, id, 2U);
            vdrive->bam[BAM_VERSION_8050] = 50;
            vdrive->bam[BAM_VERSION_8050 + 1] = 67;
            /* rest of first block unused */

            /* first bitmap block at 38/0 */
            vdrive->bam[0x100] = BAM_TRACK_8050;
            vdrive->bam[0x100 + 1] = 3;
            vdrive->bam[0x100 + 2] = 67;
            vdrive->bam[0x100 + 4] = 1; /* In this block from track ... */
            vdrive->bam[0x100 + 5] = 51; /* till excluding track ... */

            if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_8050) {
                /* second bitmap block at 38/3 */
                vdrive->bam[0x200] = DIR_TRACK_8050;
                vdrive->bam[0x200 + 1] = 1;
                vdrive->bam[0x200 + 2] = 67;
                vdrive->bam[0x200 + 4] = 51; /* In this block from track ... */
                vdrive->bam[0x200 + 5] = 78; /* till excluding track ... */
            } else
            if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_8250) {
                /* second bitmap block at 38/3 */
                vdrive->bam[0x200] = BAM_TRACK_8050;
                vdrive->bam[0x200 + 1] = 6;
                vdrive->bam[0x200 + 2] = 67;
                vdrive->bam[0x200 + 4] = 51; /* In this block from track ... */
                vdrive->bam[0x200 + 5] = 101; /* till excluding track ... */
                /* third bitmap block at 38/6 */
                vdrive->bam[0x300] = BAM_TRACK_8050;
                vdrive->bam[0x300 + 1] = 9;
                vdrive->bam[0x300 + 2] = 67;
                vdrive->bam[0x300 + 4] = 101; /* In this block from track ... */
                vdrive->bam[0x300 + 5] = 151; /* till excluding track ... */
                /* fourth bitmap block at 38/9 */
                vdrive->bam[0x400] = DIR_TRACK_8050;
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
        case VDRIVE_IMAGE_FORMAT_9000:
            {
                uint8_t tmp[256];
                int i;
                unsigned int t, tp, sp, td, tn;

                /* build track 0, sector 0 */
                memset(tmp, 0, 256);
                /* tmp[0] is track for bad blocks is 0 */
                tmp[1] = 1; /* sector for bad blocks */
                tmp[3] = 0xff; /* DOS 3.0 signature */
                /* everything starts at sector 10 as sector 0 might have a
                   BAM entry */
                tmp[4] = vdrive->num_tracks / 2; /* dir track */
                tmp[5] = 10; /* dir sector */
                tmp[6] = tmp[4]; /* header track */
                tmp[7] = tmp[5] + 10; /* header sector */
                tmp[8] = 1; /* bam track */
                /* tmp[9] is bam sector */
                tmp[10] = id[0];
                tmp[11] = id[1];
                vdrive_write_sector(vdrive, tmp, 0, 0);
                /* just incase the vdrive settings are wrong */
                vdrive->Bam_Track = tmp[8];
                vdrive->Bam_Sector = tmp[9];
                vdrive->Header_Track = tmp[6];
                vdrive->Header_Sector = tmp[7];
                vdrive->Dir_Track = tmp[4];
                vdrive->Dir_Sector = tmp[5];

                /* build track 0, sector 1 */
                memset(tmp, 0xff, 256);
                vdrive_write_sector(vdrive, tmp, 0, 1);

                /* build bam */
                vdrive->bam[0] = vdrive->Dir_Track;
                vdrive->bam[1] = vdrive->Dir_Sector;
                /* byte 2-5 unused */
                /* bytes 6- disk name + id + version */
                memset(vdrive->bam + vdrive->bam_name, 0xa0, 27);
                mystrncpy(vdrive->bam + vdrive->bam_name, (const uint8_t *)name, 16U);
                mystrncpy(vdrive->bam + vdrive->bam_id, id, 2U);
                vdrive->bam[BAM_VERSION_9000] = 51;
                vdrive->bam[BAM_VERSION_9000 + 1] = 65;

                t = 1;
                tp = 255;
                sp = 255;
                td = 240 / ( vdrive->image->sectors >> 5) / 5;
                /* fill all BAM entries */
                for (i = 0x100; i < vdrive->bam_size; i += 0x100 ) {
                    if (i + 0x100 >= vdrive->bam_size) {
                        vdrive->bam[i + 0] = 255;
                        vdrive->bam[i + 1] = 255;
                        vdrive->bam[i + 4] = t - 1;
                        vdrive->bam[i + 5] = vdrive->num_tracks + 1;
                    } else {
                        tn = t + td;
                        if (tn > vdrive->num_tracks) {
                            tn = vdrive->num_tracks;
                        }
                        vdrive->bam[i + 0] = tn;
                        vdrive->bam[i + 1] = 0;
                        vdrive->bam[i + 4] = t - 1;
                        vdrive->bam[i + 5] = t + td - 1;
                    }
                    vdrive->bam[i + 2] = tp;
                    vdrive->bam[i + 3] = sp;
                    tp = t;
                    sp = 0;
                    t += td;
                }
            }
            break;
        default:
            log_error(LOG_ERR,
                      "Unknown disk type %u.  Cannot create BAM.",
                      vdrive->image_format);
    }
    return;
}

int vdrive_bam_get_disk_id(unsigned int unit, unsigned int drive, uint8_t *id)
{
    vdrive_t *vdrive;

    vdrive = file_system_get_vdrive(unit, drive);

    if (vdrive == NULL || id == NULL || vdrive->bam == NULL) {
        return -1;
    }

    memcpy(id, vdrive->bam + vdrive->bam_id, 2);

    return 0;
}

int vdrive_bam_set_disk_id(unsigned int unit, unsigned int drive, uint8_t *id)
{
    vdrive_t *vdrive;

    vdrive = file_system_get_vdrive(unit, drive);

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
            return 5;
        case VDRIVE_IMAGE_FORMAT_4000:
            return 1;
        case VDRIVE_IMAGE_FORMAT_9000:
            return 10;
        default:
            log_error(LOG_ERR, "Unknown disk type %u.  Using interleave 10.",
                    type);
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
    unsigned int t, s;

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
            err = vdrive_read_sector(vdrive, vdrive->bam, HDR_TRACK_8050, HDR_SECTOR_8050);
            if (err != 0) {
                break;
            }
            err = vdrive_read_sector(vdrive, vdrive->bam + 256, BAM_TRACK_8050, BAM_SECTOR_8050);
            if (err != 0) {
                break;
            }
            err = vdrive_read_sector(vdrive, vdrive->bam + 512, BAM_TRACK_8050, BAM_SECTOR_8050 + 3);
            if (err != 0) {
                break;
            }

            if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_8050) {
                break;
            }

            err = vdrive_read_sector(vdrive, vdrive->bam + 768, BAM_TRACK_8050, BAM_SECTOR_8050 + 6);
            if (err != 0) {
                break;
            }
            err = vdrive_read_sector(vdrive, vdrive->bam + 1024, BAM_TRACK_8050, BAM_SECTOR_8050 + 9);
            break;
        case VDRIVE_IMAGE_FORMAT_4000:
            for (i = 0; i < 33; i++) {
                err = vdrive_read_sector(vdrive, vdrive->bam + i * 256, BAM_TRACK_4000, BAM_SECTOR_4000 + i);
                if (err != 0) {
                    break;
                }
            }
            break;
        case VDRIVE_IMAGE_FORMAT_9000:
            err = vdrive_read_sector(vdrive, vdrive->bam, vdrive->Header_Track,
                vdrive->Header_Sector);
            if (err != 0) {
                break;
            }
            /* follow chain to load bam */
            t = vdrive->Bam_Track;
            s = vdrive->Bam_Sector;
            /* use a for here to read it as t/s links could be bad */
            for (i = 0x100; i < vdrive->bam_size; i+=0x100) {
                if (t > vdrive->num_tracks
                    || s > vdrive_get_max_sectors(vdrive, t)) {
                    break;
                }
                err = vdrive_read_sector(vdrive, vdrive->bam + i, t, s);
                if (err != 0) {
                    break;
                }
                t = *(vdrive->bam + i);
                s = *(vdrive->bam + i + 1);
            }
            break;
        default:
            log_error(LOG_ERR, "Unknown disk type %u.  Cannot read BAM.",
                    vdrive->image_format);
    }

    if (err < 0) {
        return CBMDOS_IPE_NOT_READY;
    }

    return err;
}

/* Temporary hack.  */
int vdrive_bam_reread_bam(unsigned int unit, unsigned int drive)
{
    return vdrive_bam_read_bam(file_system_get_vdrive(unit, drive));
}
/*
    FIXME: partition support
*/

int vdrive_bam_write_bam(vdrive_t *vdrive)
{
    int err = -1, i;
    unsigned int t, s;

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_2040:
            err = vdrive_write_sector(vdrive, vdrive->bam, BAM_TRACK_1541, BAM_SECTOR_1541);
            break;
        case VDRIVE_IMAGE_FORMAT_1571:
            err = vdrive_write_sector(vdrive, vdrive->bam, BAM_TRACK_1571, BAM_SECTOR_1571);
            err |= vdrive_write_sector(vdrive, vdrive->bam + 256, BAM_TRACK_1571 + (vdrive->num_tracks / 2), BAM_SECTOR_1571);
            break;
        case VDRIVE_IMAGE_FORMAT_1581:
            err = vdrive_write_sector(vdrive, vdrive->bam, BAM_TRACK_1581, BAM_SECTOR_1581);
            err |= vdrive_write_sector(vdrive, vdrive->bam + 256, BAM_TRACK_1581, BAM_SECTOR_1581 + 1);
            err |= vdrive_write_sector(vdrive, vdrive->bam + 512, BAM_TRACK_1581, BAM_SECTOR_1581 + 2);
            break;
        case VDRIVE_IMAGE_FORMAT_8050:
        case VDRIVE_IMAGE_FORMAT_8250:
            err = vdrive_write_sector(vdrive, vdrive->bam, DIR_TRACK_8050, BAM_SECTOR_8050);
            err |= vdrive_write_sector(vdrive, vdrive->bam + 256, BAM_TRACK_8050, BAM_SECTOR_8050);
            err |= vdrive_write_sector(vdrive, vdrive->bam + 512, BAM_TRACK_8050, BAM_SECTOR_8050 + 3);

            if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_8050) {
                break;
            }

            err |= vdrive_write_sector(vdrive, vdrive->bam + 768, BAM_TRACK_8050, BAM_SECTOR_8050 + 6);
            err |= vdrive_write_sector(vdrive, vdrive->bam + 1024, BAM_TRACK_8050, BAM_SECTOR_8050 + 9);
            break;
        case VDRIVE_IMAGE_FORMAT_4000:
            err = 0;
            for (i = 0; i < 33; i++) {
                err |= vdrive_write_sector(vdrive, vdrive->bam + i * 256, BAM_TRACK_4000, BAM_SECTOR_4000 + i);
            }
            break;
        case VDRIVE_IMAGE_FORMAT_9000:
            err = vdrive_write_sector(vdrive, vdrive->bam, vdrive->Header_Track,
                vdrive->Header_Sector);
            if (err != 0) {
                break;
            }
            /* follow chain to save bam */
            t = vdrive->Bam_Track;
            s = vdrive->Bam_Sector;
            /* use a for here to write it as t/s links could be bad */
            for (i = 0x100; i < vdrive->bam_size; i+=0x100) {
                if (t > vdrive->num_tracks
                    || s > vdrive_get_max_sectors(vdrive, t)) {
                    break;
                }
                err = vdrive_write_sector(vdrive, vdrive->bam + i, t, s);
                if (err != 0) {
                    break;
                }
                t = *(vdrive->bam + i);
                s = *(vdrive->bam + i + 1);
            }
            break;
        default:
            log_error(LOG_ERR, "Unknown disk type %u.  Cannot read BAM.",
                    vdrive->image_format);
    }
    return err;
}

/* ------------------------------------------------------------------------- */

/*
 * Return the number of free blocks on disk.
 */

unsigned int vdrive_bam_free_block_count(vdrive_t *vdrive)
{
    unsigned int blocks;
    unsigned int i;
    unsigned int j; /* FIXME: j looks a lot like i or l */

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
            case VDRIVE_IMAGE_FORMAT_9000:
                {
                    uint8_t *bamp;
                    /* above should use vdrive_bam_get_track_entry as well */
                    for (j = 0; j < vdrive->image->sectors ; j += 32 ) {
                        bamp = vdrive_bam_get_track_entry(vdrive, i, j);
                        blocks += *bamp;
                    }
                    break;
                }
            default:
                log_error(LOG_ERR,
                          "Unknown disk type %u.  Cannot calculate free sectors.",
                          vdrive->image_format);
        }
    }
    return blocks;
}
