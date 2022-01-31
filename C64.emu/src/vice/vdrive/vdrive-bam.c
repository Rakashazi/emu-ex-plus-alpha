 /*
 * vdrive-bam.c - Virtual disk-drive implementation. BAM specific functions.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ingo Korb <ingo@akana.de>
 *
 * Multi-drive and DHD enhancements by
 *  Roberto Muscedere <rmusced@uwindsor.ca>
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
#include "lib.h"
#include "log.h"
#include "types.h"
#include "vdrive-bam.h"
#include "vdrive-command.h"
#include "vdrive.h"

/* #define DEBUG_DRIVE */

/* 8250 has 2 extra BAM sectors; make sure they match up with the 8050 as we
    only use the 8050 parameters. */
#if BAM_TRACK_8050 != BAM_TRACK_8250
#error BAM_TRACK_8050 != BAM_TRACK_8250
#endif
#if BAM_SECTOR_8050 != BAM_SECTOR_8250
#error BAM_SECTOR != BAM_SECTOR_8250
#endif
#if BAM_BIT_MAP_8050 != BAM_BIT_MAP_8050
#error BAM_BIT_MAP_8050 != BAM_BIT_MAP_8250
#endif

/*
 * Load particular BAM blocks
 */

static int vdrive_bam_read_bam_block(vdrive_t *vdrive, unsigned int block)
{
    int err = -1, i;

    if (block >= VDRIVE_BAM_MAX_STATES) {
        return -1;
    }

    /* don't read in the block if it is already loaded */
    if (vdrive->bam_state[block] >= 0) {
        return CBMDOS_IPE_OK;
    }

    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_9000) {
        /* for 9000s we have to read in the bam up to the point desired as
            we don't know its location; we have to follow the t/s links.
            we know the first 2 though. */
        for (i = 2; i <= block; i++) {
            /* read bam sector to find t/s link if we don't already have it */
            if (vdrive->bam_tracks[i] < 0) {
                err = vdrive_bam_read_bam_block(vdrive, i - 1);
                if (err < 0) {
                    return CBMDOS_IPE_NOT_READY;
                } else if (err > 0) {
                    return err;
                }
                /* update links */
                vdrive->bam_tracks[i] = vdrive->bam[(i - 1) << 8];
                vdrive->bam_sectors[i] = vdrive->bam[((i - 1) << 8) | 1];
            }
        }
        /* follow through */
    }

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1581:
            /* 1581's have different BAMs for each partition */
            for (i = 0; i < 3; i++) {
                vdrive->bam_tracks[i] = vdrive->Bam_Track;
                vdrive->bam_sectors[i] = vdrive->Bam_Sector + i;
            }
            /* fall through */
        case VDRIVE_IMAGE_FORMAT_2040:
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_1571:
        case VDRIVE_IMAGE_FORMAT_8050:
        case VDRIVE_IMAGE_FORMAT_8250:
        case VDRIVE_IMAGE_FORMAT_9000:
        case VDRIVE_IMAGE_FORMAT_NP:
            if (vdrive->bam_tracks[block] >= 0 ) {
                err = vdrive_read_sector(vdrive, vdrive->bam + (block << 8),
                         vdrive->bam_tracks[block], vdrive->bam_sectors[block]);
            } else {
                log_error(LOG_ERR, "Trying to read beyond BAM limit (offset=0x%x).",
                    block << 8);
            }
            break;
        case VDRIVE_IMAGE_FORMAT_SYS:
            break;
        default:
            log_error(LOG_ERR, "Unknown disk type %u.  Cannot read BAM.",
                    vdrive->image_format);
    }

    /* set state bits to valid */
    if (!err) {
        vdrive->bam_state[block] = 0;
    }

    if (err < 0) {
        return CBMDOS_IPE_NOT_READY;
    }

    return err;
}

/* ------------------------------------------------------------------------- */

/*
    return Maximum distance from dir track to start/end of disk.

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
        case VDRIVE_IMAGE_FORMAT_NP:
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

/* lookup interleave value for supported drive types */
static int vdrive_bam_get_interleave(vdrive_t *vdrive)
{
    /* Note: Values for 2040/8050/8250 determined empirically */
    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1541:
            /* on DHDs, the interleave becomes 1 */
            if (vdrive->haspt) {
                return 1;
            }
            return 10;
        case VDRIVE_IMAGE_FORMAT_1571:
            /* on DHDs, the interleave becomes 1 */
            if (vdrive->haspt) {
                return 1;
            }
            return 6;
        case VDRIVE_IMAGE_FORMAT_2040:
            return 10;
        case VDRIVE_IMAGE_FORMAT_1581:
            return 1;
        case VDRIVE_IMAGE_FORMAT_8050:
            return 6;
        case VDRIVE_IMAGE_FORMAT_8250:
            return 5;
        case VDRIVE_IMAGE_FORMAT_NP:
            return 1;
        case VDRIVE_IMAGE_FORMAT_9000:
            return 10;
        default:
            log_error(LOG_ERR, "Unknown disk type %u.  Using interleave 10.",
                    vdrive->image_format);
            return 10;
    }
}

/*
This function is used by the next 3 to find an available sector in
a single track. Typically this would be a simple loop, but the D9090/60
adds another dimension (heads) to the search.
It only updates the sector if it finds something (returns 0).
returns -1 if nothing found.
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
This function looks for the first free sector of a new file. Depending
on the drive type, it closely matches the original drive algorithms.
It only updates the track/sector if it finds something (returns 0).
returns -1 if nothing found.
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
        if (!vdrive_bam_alloc_next_free_sector(vdrive, track, sector)) {
           return 0;
        }
        *track = origt;
        *sector = origs;
        return -1;
    /* for DNP, do the same, but no toggle */
    } else if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP) {
        *track = vdrive->Dir_Track;
        *sector = vdrive->Dir_Sector;
        if (!vdrive_bam_alloc_next_free_sector(vdrive, track, sector)) {
           return 0;
        }
        *track = origt;
        *sector = origs;
        return -1;
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
            if (d) {
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
static int vdrive_bam_alloc_add_interleave(vdrive_t *vdrive,
                                           unsigned int track,
                                           unsigned int sector,
                                           unsigned int interleave)
{
    unsigned int max_sector;
    unsigned int max_sector_all;
    unsigned int s;
    unsigned int h;

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
downwards. Returns 0 when a track/sector is found, otherwise -1. */
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
upwards. Returns 0 when a track/sector is found, otherwise -1. */
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
This function find the next suitable sector based on the one passed.
function reworked to use smaller functions above and to behave like DOS
code. Interleave is options as it is used by extended directory creation.
return 0 when sector found, otherwise -1. */
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
    if (*track == vdrive->Dir_Track) {
        if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_9000) {
            /* allowed on D9090/60 */
        } else if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP) {
            /* Adjust sector on DNP if < 63 on track 1 */
            if (*sector < 63) {
                *sector = 63;
            }
        } else {
            /* everything else: no */
            return -1;
        }
    }

    /* DNPs allocate space different than CBM drives; it simply looks upwards
        sector by sector, and when it hits the maximum, it goes back to track 1. */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP) {
        unsigned int max_sector = vdrive_get_max_sectors_per_head(vdrive, *track);
        /* use counter to check all sectors in partition*/
        s = max_sector * vdrive->num_tracks;
        while (s) {
            s--;
            /* increment and go to next */
            (*sector)++;
            /* if at the end of the track */
            if (*sector >= max_sector) {
                /* reset sector */
                *sector = 0;
                (*track)++;
                /* end of partition? */
                if (*track > vdrive->num_tracks) {
                    /* go back to track 1 */
                    *track = 1;
                }
            }
            /* skip the first 64 sectors of track 1 */
            if (*track == DIR_TRACK_NP && *sector < 64) {
                *sector = 64;
            }
            /* try the sector */
            if (vdrive_bam_allocate_sector(vdrive, *track, *sector)) {
                /* it is good, leave */
                return 0;
            }
        }
        /* leave if we done scanning all of the sectors */
        *track = origt;
        *sector = origs;
        return -1;
    }

    /* find next sector on this track based on interleave */
    s = vdrive_bam_alloc_add_interleave(vdrive, *track, *sector, interleave);
    /* starting from there, see if there is an available sector */
    if (!vdrive_bam_alloc_worker(vdrive, *track, &s)) {
        *sector = s;
        return 0;
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
            /* after first pass, reset the starting track, and set sector to 0 */
            *track = split + 1;
            /* reset sector position */
            *sector = vdrive_bam_alloc_next_free_sector_reset(vdrive, *track,
                *sector, interleave);
        } else if (*track >= split) {
        /* on the first pass, look upward if we are already above the dir track */
        /* on subsequence passes, we start one above the dir track and look up */
        /* DNP goes here first */
            if (vdrive_bam_alloc_up(vdrive, track, sector, interleave) == 0) {
                return 0;
            }
            /* after first pass, reset the starting track, and set sector to 0 */
            *track = split - 1;
            /* reset sector position */
            *sector = vdrive_bam_alloc_next_free_sector_reset(vdrive, *track,
                *sector, interleave);
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

/* similar to above, but the default interleave is used. */
int vdrive_bam_alloc_next_free_sector(vdrive_t *vdrive,
                                      unsigned int *track,
                                      unsigned int *sector)
{
    return vdrive_bam_alloc_next_free_sector_interleave(vdrive, track,
        sector, vdrive_bam_get_interleave(vdrive));
}

/* update bam entry by setting bit based on sector value; offset calculated by
    vdrive_bam_get_track_entry() */
/* bamp should point to "sectors available", not the bam bits for the track */
static void vdrive_bam_set(vdrive_t *vdrive, uint8_t *bamp, unsigned int sector)
{
    uint8_t *t = &(bamp[1 + (sector >> 3)]);
    int p = (int)((t - vdrive->bam) >> 8);

    /* make sure bam data is loaded */
    vdrive_bam_read_bam_block(vdrive, p);
    /* update */
    *t |= (1 << (sector & 7));
    /* tag as dirty */
    vdrive->bam_state[p] = 1;

    return;
}

/* update bam entry by clearing bit based on sector value; offset calculated by
    vdrive_bam_get_track_entry() */
/* bamp should point to "sectors available", not the bam bits for the track */
static void vdrive_bam_clr(vdrive_t *vdrive, uint8_t *bamp, unsigned int sector)
{
    uint8_t *t = &(bamp[1 + (sector >> 3)]);
    int p = (int)((t - vdrive->bam) >> 8);

    /* make sure bam data is loaded */
    vdrive_bam_read_bam_block(vdrive, p);
    /* update */
    *t &= ~(1 << (sector & 7));
    /* tag as dirty */
    vdrive->bam_state[p] = 1;

    return;
}

/* return bam entry bit based on sector value; offset calculated by
    vdrive_bam_get_track_entry() */
/* bamp should point to "sectors available", not the bam bits for the track */
/* this function used to be used by c1541, but it is too particular to
    drives which have the block count in the bam entries */
/* return of 0 is allocated, anything else is unallocated */
static int vdrive_bam_isset(vdrive_t *vdrive, uint8_t *bamp, unsigned int sector)
{
    uint8_t *t = &(bamp[1 + (sector >> 3)]);
    int p = (int)((t - vdrive->bam) >> 8);

    /* make sure bam data is loaded */
    vdrive_bam_read_bam_block(vdrive, p);

    /* read */
    return *t & (1 << (sector & 7));
}

/* follow a passed t/s location and allocate all used sectors (used for validate) */
/* returns DOS error values, 0 = okay, etc */
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
/* returns DOS error values, 0 = okay, etc */
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
        case VDRIVE_IMAGE_FORMAT_8250:
            {
                int i;
                for (i = 1; i < 5; i++) {
                    if (vdrive->bam_tracks[i] > 0) {
                        if (vdrive_bam_read_bam_block(vdrive, i)) {
                            break;
                        }
                        if (track >= bam[(i * 0x100) + 4] && track < bam[(i * 0x100) + 5]) {
                            bamp = &bam[(i * 0x100) + BAM_BIT_MAP_8050 + 5 * (track - bam[(i * 0x100) + 4])];
                            break;
                        }
                    }
                }
            }
            break;
        case VDRIVE_IMAGE_FORMAT_NP:
            /* NP's don't use this, so return a pointer one less the where the bitmap is */
            bamp = &bam[0x100 + BAM_BIT_MAP_NP + 32 * (track - 1) - 1];
            break;
        case VDRIVE_IMAGE_FORMAT_9000:
            {
                int i;
                sector = sector >> 5;
                for (i = 1; i < (vdrive->bam_size >> 8); i++) {
                    if (vdrive_bam_read_bam_block(vdrive, i)) {
                        break;
                    }
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

/* adjust "blocks available" based on drive type, 1571 and NPs are exceptions */
static void vdrive_bam_sector_free(vdrive_t *vdrive, uint8_t *bamp,
                                   unsigned int track, int add)
{
    int p = (int)((bamp - vdrive->bam) >> 8);

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_2040:
        case VDRIVE_IMAGE_FORMAT_1581:
        case VDRIVE_IMAGE_FORMAT_8050:
        case VDRIVE_IMAGE_FORMAT_8250:
        case VDRIVE_IMAGE_FORMAT_9000:
            /* make sure bam data is loaded */
            vdrive_bam_read_bam_block(vdrive, p);
            /* update */
            *bamp += add;
            /* tag as dirty */
            vdrive->bam_state[p] = 1;
            break;
        case VDRIVE_IMAGE_FORMAT_1571:
            /* make sure bam data is loaded */
            vdrive_bam_read_bam_block(vdrive, p);
            /* tag as dirty */
            vdrive->bam_state[p] = 1;
            if (track <= NUM_TRACKS_1571 / 2) {
                /* update */
                *bamp += add;
            } else {
                int t = BAM_EXT_BIT_MAP_1571 + track - NUM_TRACKS_1571 / 2 - 1;
                /* update */
                /* make sure bam data is loaded */
                vdrive_bam_read_bam_block(vdrive, t >> 8);
                vdrive->bam[t] += add;
                /* tag as dirty */
                vdrive->bam_state[t >> 8] = 1;
            }
            break;
        case VDRIVE_IMAGE_FORMAT_NP:
            /* NP's don't keep track of number of sectors available */
            break;
        default:
            log_error(LOG_ERR, "Unknown disk type %u.  Cannot find free sector.",
                    vdrive->image_format);
    }
}

/* allocate a t/s in the bam */
/* return of 1 if okay, 0 if failed (1571 > 70 tracks) or already allocated */
int vdrive_bam_allocate_sector(vdrive_t *vdrive,
                               unsigned int track, unsigned int sector)
{
    uint8_t *bamp;

    /* Tracks > 70 don't go into the (regular) BAM on 1571 */
    if ((track > NUM_TRACKS_1571) && (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1571)) {
        return 0;
    }
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP) {
        sector ^= 7;
    }

    bamp = vdrive_bam_get_track_entry(vdrive, track, sector);
    /* D9090/60 groups 32 sectors per bam group */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_9000) {
        sector &= 31;
    }
    if (bamp && vdrive_bam_isset(vdrive, bamp, sector)) {
        vdrive_bam_clr(vdrive, bamp, sector); /* clear bit */
        vdrive_bam_sector_free(vdrive, bamp, track, -1); /* update count */
        return 1;
    }

    return 0;
}

/* unallocate a t/s in the bam */
/* return of 1 if okay, 0 if failed (1571 > 70 tracks) or already allocated */
int vdrive_bam_free_sector(vdrive_t *vdrive, unsigned int track,
                           unsigned int sector)
{
    uint8_t *bamp;

    /* Tracks > 70 don't go into the (regular) BAM on 1571 */
    if ((track > NUM_TRACKS_1571) && (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1571)) {
        return 0;
    }
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP) {
        sector ^= 7;
    }

    bamp = vdrive_bam_get_track_entry(vdrive, track, sector);
    /* D9090/60 groups 32 sectors per bam group */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_9000) {
        sector &= 31;
    }
    if (bamp && !(vdrive_bam_isset(vdrive, bamp, sector))) {
        vdrive_bam_set(vdrive, bamp, sector); /* set bit */
        vdrive_bam_sector_free(vdrive, bamp, track, 1); /* update count */
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
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP) {
        sector ^= 7;
    }
    bamp = vdrive_bam_get_track_entry(vdrive, track, sector);
    /* D9090/60 groups 32 sectors per bam group */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_9000) {
        sector &= 31;
    }
    if (bamp && !vdrive_bam_isset(vdrive, bamp, sector)) {
        return 1;
    }

    return 0;
}

/* used by validate to clear out bam - sets all blocks as used */
void vdrive_bam_clear_all(vdrive_t *vdrive)
{
    uint8_t *bam = vdrive->bam;
    int i;

    vdrive_bam_read_bam(vdrive);

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1541:
            memset(bam + BAM_EXT_BIT_MAP_1541, 0, 4 * 5);
        /* fallthrough */
        case VDRIVE_IMAGE_FORMAT_2040:
            memset(bam + BAM_BIT_MAP, 0, 4 * NUM_TRACKS_1541);
            vdrive->bam_state[0] = 1;
            break;
        case VDRIVE_IMAGE_FORMAT_1571:
            memset(bam + BAM_BIT_MAP, 0, 4 * NUM_TRACKS_1571 / 2);
            memset(bam + BAM_EXT_BIT_MAP_1571, 0, NUM_TRACKS_1571 / 2);
            vdrive->bam_state[0] = 1;
            memset(bam + 0x100, 0, 3 * NUM_TRACKS_1571 / 2);
            vdrive->bam_state[1] = 1;
            break;
        case VDRIVE_IMAGE_FORMAT_1581:
            memset(bam + 0x100 + BAM_BIT_MAP_1581, 0, 6 * NUM_TRACKS_1581 / 2);
            vdrive->bam_state[1] = 1;
            memset(bam + 0x200 + BAM_BIT_MAP_1581, 0, 6 * NUM_TRACKS_1581 / 2);
            vdrive->bam_state[2] = 1;
            break;
        case VDRIVE_IMAGE_FORMAT_8250:
            memset(bam + 0x300 + BAM_BIT_MAP_8250, 0, 0x100 - BAM_BIT_MAP_8250);
            vdrive->bam_state[3] = 1;
            memset(bam + 0x400 + BAM_BIT_MAP_8250, 0, 0x100 - BAM_BIT_MAP_8250);
            vdrive->bam_state[4] = 1;
        /* fallthrough */
        case VDRIVE_IMAGE_FORMAT_8050:
            memset(bam + 0x100 + BAM_BIT_MAP_8050, 0, 0x100 - BAM_BIT_MAP_8050);
            vdrive->bam_state[1] = 1;
            memset(bam + 0x200 + BAM_BIT_MAP_8050, 0, 0x100 - BAM_BIT_MAP_8050);
            vdrive->bam_state[2] = 1;
            break;
        case VDRIVE_IMAGE_FORMAT_NP:
            /* CMD sets all bits to 1 on unused tracks */
            memset(bam + 0x100 + BAM_BIT_MAP_NP, 255, 255 * 32);
            for (i = 1; i < 33; i++) {
                vdrive->bam_state[i] = 1;
            }
            break;
        case VDRIVE_IMAGE_FORMAT_9000:
            for (i = 0x100; i < vdrive->bam_size; i += 0x100) {
                memset(bam + i + BAM_BIT_MAP_9000, 0, 0x100 - BAM_BIT_MAP_9000);
                vdrive->bam_state[i >> 8] = 1;
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

/* used by format to setup bam/dir values */
void vdrive_bam_create_empty_bam(vdrive_t *vdrive, const char *name, uint8_t *id)
{
    /* Reinitialize BAM structre */
    vdrive_bam_setup_bam(vdrive);

    /* Create Disk Format for 1541/1571/1581/2040/NP disks.  */
    memset(vdrive->bam, 0, vdrive->bam_size);
    vdrive->bam_state[0] = 1;

    if (vdrive->image_format != VDRIVE_IMAGE_FORMAT_8050
        && vdrive->image_format != VDRIVE_IMAGE_FORMAT_8250
        && vdrive->image_format != VDRIVE_IMAGE_FORMAT_9000) {
        vdrive->bam[0] = vdrive->Dir_Track;
        vdrive->bam[1] = vdrive->Dir_Sector;
        /* position 2 will be overwritten later for 2040/1581/NP */
        vdrive->bam[2] = 65;

        if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1571) {
            vdrive->bam[3] = 0x80;
        }

        memset(vdrive->bam + vdrive->bam_name, 0xa0,
               (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1581
                || vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP) ? 25 : 27);
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
            vdrive->bam_state[1] = 1;
            vdrive->bam_state[2] = 1;
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
            vdrive->bam_state[1] = 1;
            vdrive->bam_state[2] = 1;

            if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_8050) {
                /* second bitmap block at 38/3 */
                vdrive->bam[0x200] = DIR_TRACK_8050;
                vdrive->bam[0x200 + 1] = 1;
                vdrive->bam[0x200 + 2] = 67;
                vdrive->bam[0x200 + 4] = 51; /* In this block from track ... */
                vdrive->bam[0x200 + 5] = 78; /* till excluding track ... */
            } else if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_8250) {
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
                vdrive->bam_state[3] = 1;
                vdrive->bam_state[4] = 1;
            }
            break;
        case VDRIVE_IMAGE_FORMAT_NP:
            vdrive->bam[0x02] = 72;
            /* Setup BAM version.  */
            vdrive->bam[BAM_VERSION_NP] = 49;
            vdrive->bam[BAM_VERSION_NP + 1] = 72;
            vdrive->bam[0x20] = vdrive->Bam_Track;
            vdrive->bam[0x21] = vdrive->Bam_Sector;
            vdrive->bam[0x100 + 2] = 72;
            vdrive->bam[0x100 + 3] = ~72;
            vdrive->bam[0x100 + 4] = id[0];
            vdrive->bam[0x100 + 5] = id[1];
            vdrive->bam[0x100 + 6] = 0xc0;
            vdrive->bam[0x100 + 8] = vdrive->num_tracks;
            vdrive->bam_state[1] = 1;
            break;
        case VDRIVE_IMAGE_FORMAT_9000:
            {
                uint8_t tmp[256];
                int i;
                unsigned int t, tp, sp, td, tn, tb;

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
                tb = 0;
                tp = 255;
                sp = 255;
                td = 240 / ( vdrive->image->sectors >> 5) / 5;
                /* fill all BAM entries */
                for (i = 0x100; i < vdrive->bam_size; i += 0x100 ) {
                    tn = t + td;
                    if (i + 0x100 >= vdrive->bam_size) {
                        vdrive->bam[i + 0] = 255;
                        vdrive->bam[i + 1] = 255;
                        vdrive->bam[i + 4] = tb;
                        vdrive->bam[i + 5] = vdrive->num_tracks + 1;
                    } else {
                        if (tn > vdrive->num_tracks) {
                            tn = vdrive->num_tracks;
                        }
                        vdrive->bam[i + 0] = tn;
                        vdrive->bam[i + 1] = 0;
                        vdrive->bam[i + 4] = tb;
                        vdrive->bam[i + 5] = tb + td;
                    }
                    vdrive->bam[i + 2] = tp;
                    vdrive->bam[i + 3] = sp;
                    tp = t;
                    sp = 0;
                    vdrive->bam_tracks[i >> 8] = t;
                    vdrive->bam_sectors[i >> 8] = 0;
                    t = tn;
                    tb += td;
                    vdrive->bam_state[i >> 8] = 1;
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

/* get disk id internally */
int vdrive_bam_int_get_disk_id(vdrive_t *vdrive, uint8_t *id)
{
    /* make sure bam data is loaded */
    vdrive_bam_read_bam_block(vdrive, vdrive->bam_id >> 8);

    memcpy(id, vdrive->bam + vdrive->bam_id, 2);

    return 0;
}

/* get disk id */
int vdrive_bam_get_disk_id(unsigned int unit, unsigned int drive, uint8_t *id)
{
    vdrive_t *vdrive;

    if (id == NULL) {
        return -1;
    }

    vdrive = file_system_get_vdrive(unit);

    if (vdrive == NULL) {
        return -1;
    }

    /* leave with error if we can't get to that drive */
    if (vdrive_switch(vdrive, drive)) {
        return -1;
    }

    if (vdrive->bam == NULL) {
        return -1;
    }

    return vdrive_bam_int_get_disk_id(vdrive, id);
}

/* set disk id */
int vdrive_bam_set_disk_id(unsigned int unit, unsigned int drive, uint8_t *id)
{
    vdrive_t *vdrive;

    if (id == NULL) {
        return -1;
    }

    vdrive = file_system_get_vdrive(unit);

    if (vdrive == NULL) {
        return -1;
    }

    /* leave with error if we can't get to that drive */
    if (vdrive_switch(vdrive, drive)) {
        return -1;
    }

    if (vdrive->bam == NULL) {
        return -1;
    }

    /* make sure bam data is loaded */
    vdrive_bam_read_bam_block(vdrive, vdrive->bam_id >> 8);

    memcpy(vdrive->bam + vdrive->bam_id, id, 2);

    vdrive->bam_state[vdrive->bam_id >> 8] = 1;

    return vdrive_bam_write_bam(vdrive);
}

/* ------------------------------------------------------------------------- */

/*
 * Load/Store BAM Image.
 */

/* probably we should make a list with BAM blocks for each drive type... (AF)*/
int vdrive_bam_read_bam(vdrive_t *vdrive)
{
    int err = -1, i;

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_2040:
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_1571:
        case VDRIVE_IMAGE_FORMAT_1581:
        case VDRIVE_IMAGE_FORMAT_8050:
        case VDRIVE_IMAGE_FORMAT_8250:
        case VDRIVE_IMAGE_FORMAT_NP:
        case VDRIVE_IMAGE_FORMAT_9000:
            /* read all the individual BAM blocks based on the BAM size */
            for (i = 0; i < vdrive->bam_size >> 8; i++) {
                err = vdrive_bam_read_bam_block(vdrive, i);
                if (err) {
                   break;
                }
            }
            break;
        case VDRIVE_IMAGE_FORMAT_SYS:
            /* so we can attach empty D?M disks */
            err = 0;
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

/* Update ALL bam data back to image */
int vdrive_bam_write_bam(vdrive_t *vdrive)
{
    int err = CBMDOS_IPE_OK, i;

    /* make sure bam is allocated first */
    if (!vdrive->bam) {
        return -1;
    }

    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1581) {
        /* 1581's have different BAMs for each partition */
        for (i = 0; i < 3; i++) {
            vdrive->bam_tracks[i] = vdrive->Bam_Track;
            vdrive->bam_sectors[i] = vdrive->Bam_Sector + i;
        }
        /* fall through */
    }

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1581:
        case VDRIVE_IMAGE_FORMAT_2040:
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_1571:
        case VDRIVE_IMAGE_FORMAT_8050:
        case VDRIVE_IMAGE_FORMAT_8250:
        case VDRIVE_IMAGE_FORMAT_9000:
        case VDRIVE_IMAGE_FORMAT_NP:
            /* write the individual BAM blocks based on the BAM size */
            for (i = 0; i < vdrive->bam_size >> 8; i++) {
                /* only write the blocks that have changed */
                if (vdrive->bam_state[i] > 0) {
                    err = vdrive_write_sector(vdrive, vdrive->bam + (i << 8),
                              vdrive->bam_tracks[i], vdrive->bam_sectors[i]);
                    if (err) {
                        break;
                    }
                    /* set the status to read */
                    vdrive->bam_state[i] = 0;
                }
            }
            break;
        case VDRIVE_IMAGE_FORMAT_SYS:
            break;
        default:
            log_error(LOG_ERR, "Unknown disk type %u.  Cannot write BAM.",
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
    unsigned int t;
    unsigned int s;
    unsigned int a;
    uint8_t *bamp;
    static uint8_t bitcount[256];
    static int bc = 0;

    /* create a LUT that helps calculate the blocks from on a NP faster */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP && !bc) {
        for (t = 0; t < 256; t++) {
            blocks = t;
            for (s = 0; s < 8; s++) {
                bitcount[t] += (blocks & 1);
                blocks >>= 1;
            }
        }
        bc++;
    }

    /* load in the whole bam */
    t = vdrive_bam_read_bam(vdrive);
    if (t) {
        return 0;
    }

    blocks = 0;
    /* note reserved DIR space is skipped on all drives except D9090/60 */
    for (t = 1; t <= vdrive->num_tracks; t++) {
        switch (vdrive->image_format) {
            case VDRIVE_IMAGE_FORMAT_2040:
            case VDRIVE_IMAGE_FORMAT_1541:
            case VDRIVE_IMAGE_FORMAT_1581:
            case VDRIVE_IMAGE_FORMAT_8050:
            case VDRIVE_IMAGE_FORMAT_8250:
                if (t != vdrive->Dir_Track) {
                    bamp = vdrive_bam_get_track_entry(vdrive, t, 0);
                    if (bamp) {
                       blocks += *bamp;
                    }
                }
                break;
            case VDRIVE_IMAGE_FORMAT_1571:
                if (t != vdrive->Dir_Track && t != vdrive->Dir_Track + 35) {
                    if (t <= NUM_TRACKS_1571 / 2) {
                        bamp = vdrive_bam_get_track_entry(vdrive, t, 0);
                    } else {
                        bamp = &(vdrive->bam[BAM_EXT_BIT_MAP_1571 + t - NUM_TRACKS_1571 / 2 - 1]);
                    }
                    if (bamp) {
                       blocks += *bamp;
                    }
                }
                break;
            case VDRIVE_IMAGE_FORMAT_NP:
                /* NPs don't have a free count; just count the bits */
                a = BAM_BIT_MAP_NP + 256 + 32 * (t - 1);
                for (s = ((t == vdrive->Bam_Track) ? 8 : 0); s < 32; s++) {
                    blocks += bitcount[ vdrive->bam[ a + s ] ];
                }
                break;
            case VDRIVE_IMAGE_FORMAT_9000:
                /* More than 1 BAM entry per track */
                for (s = 0; s < vdrive->image->sectors ; s += 32 ) {
                    bamp = vdrive_bam_get_track_entry(vdrive, t, s);
                    if (bamp) {
                       blocks += *bamp;
                    }
                }
                break;
            default:
                log_error(LOG_ERR,
                          "Unknown disk type %u.  Cannot calculate free sectors.",
                          vdrive->image_format);
        }
    }

    return blocks;
}

/* ------------------------------------------------------------------------- */

/*
 * Return if disk has a GEOS format tag.
 */

int vdrive_bam_isgeos(vdrive_t *vdrive)
{
    uint8_t geosmagic[15] = {0x47, 0x45, 0x4f, 0x53, 0x20, 0x66, 0x6f, 0x72,
                             0x6d, 0x61, 0x74, 0x20, 0x56, 0x31, 0x2e};
    int i;

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_1571:
        case VDRIVE_IMAGE_FORMAT_1581:
        case VDRIVE_IMAGE_FORMAT_NP:
            /* check for geos signature on bam on supported drives */
            /* should be "GEOS format V1.x"; we skip the x */
            for (i = 0; i < 15; i++) {
                if (geosmagic[i] != vdrive->bam[0xad + i]) {
                    break;
                }
            }
            if (i == 15) {
                return 1;
            }
            break;
        default:
            break;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

/*
 * Allocate space for bam, and invalidate the state
 */

void vdrive_bam_setup_bam(vdrive_t *vdrive)
{
    int i;

    if (vdrive->bam) {
        lib_free(vdrive->bam);
        vdrive->bam = NULL;
    }
    if (vdrive->bam_size) {
        vdrive->bam = lib_malloc(vdrive->bam_size);
    } else {
        vdrive->bam = NULL;
    }

    /* set all state bits as invalid */
    for (i = 0; i < VDRIVE_BAM_MAX_STATES; i++) {
        vdrive->bam_state[i] = -1;
        vdrive->bam_tracks[i] = -1;
        vdrive->bam_sectors[i] = -1;
    }

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1571:
            vdrive->bam_tracks[1] = BAM_TRACK_1571 + (vdrive->num_tracks / 2);
            vdrive->bam_sectors[1] = BAM_SECTOR_1571;
            /* fall through */
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_2040:
            vdrive->bam_tracks[0] = BAM_TRACK_1541;
            vdrive->bam_sectors[0] = BAM_SECTOR_1541;
            break;
        case VDRIVE_IMAGE_FORMAT_1581:
            /* 1581's have different BAMs based on the partition */
            /* generated dynamically on read and write */
            break;
        case VDRIVE_IMAGE_FORMAT_8250:
            vdrive->bam_tracks[3] = BAM_TRACK_8050;
            vdrive->bam_sectors[3] = BAM_SECTOR_8050 + 6;
            vdrive->bam_tracks[4] = BAM_TRACK_8050;
            vdrive->bam_sectors[4] = BAM_SECTOR_8050 + 9;
            /* fall through */
        case VDRIVE_IMAGE_FORMAT_8050:
            vdrive->bam_tracks[0] = HDR_TRACK_8050;
            vdrive->bam_sectors[0] = HDR_SECTOR_8050;
            vdrive->bam_tracks[1] = BAM_TRACK_8050;
            vdrive->bam_sectors[1] = BAM_SECTOR_8050;
            vdrive->bam_tracks[2] = BAM_TRACK_8050;
            vdrive->bam_sectors[2] = BAM_SECTOR_8050 + 3;
            break;
        case VDRIVE_IMAGE_FORMAT_NP:
            /* sector 1 is the header, sector 2 is the bam beginning, up to sector 33 which is the last one */
            for (i = 0; i < 33; i++) {
                vdrive->bam_tracks[i] = BAM_TRACK_NP;
                vdrive->bam_sectors[i] = BAM_SECTOR_NP + i;
            }
            break;
        case VDRIVE_IMAGE_FORMAT_SYS:
            /* doesn't have a BAM */
            break;
        case VDRIVE_IMAGE_FORMAT_9000:
            vdrive->bam_tracks[0] = vdrive->Header_Track;
            vdrive->bam_sectors[0] = vdrive->Header_Sector;
            vdrive->bam_tracks[1] = vdrive->Bam_Track;
            vdrive->bam_sectors[1] = vdrive->Bam_Sector;
            break;
        default:
            log_error(LOG_ERR, "Unknown disk type %u.  Cannot locate BAM.",
                    vdrive->image_format);
    }
}
