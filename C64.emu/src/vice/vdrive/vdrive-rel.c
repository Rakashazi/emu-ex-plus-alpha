/*
 * vdrive-rel.c - Virtual disk-drive implementation.
 *                Relative files specific functions.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Roberto Muscedere <cococommie@cogeco.ca>
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

/* #define DEBUG_DRIVE */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cbmdos.h"
#include "diskimage.h"
#include "lib.h"
#include "log.h"
#include "vdrive-bam.h"
#include "vdrive-command.h"
#include "vdrive-dir.h"
#include "vdrive-iec.h"
#include "vdrive-rel.h"
#include "vdrive.h"

#define SIDE_SECTORS_MAX 6
#define SIDE_INDEX_MAX   120

#define OFFSET_NEXT_TRACK  0
#define OFFSET_NEXT_SECTOR 1
#define OFFSET_SECTOR_NUM  2
#define OFFSET_RECORD_LEN  3
#define OFFSET_SIDE_SECTOR 4
#define OFFSET_POINTER     16

#define OFFSET_SUPER_254     2
#define OFFSET_SUPER_POINTER 3
#define SIDE_SUPER_MAX       126

#define DIRTY_SECTOR   1
#define DIRTY_RECORD   2
#define WRITTEN_RECORD 4

static log_t vdrive_rel_log = LOG_ERR;

static unsigned int vdrive_rel_record_max(vdrive_t *vdrive, unsigned int secondary);

void vdrive_rel_init(void)
{
    vdrive_rel_log = log_open("VDriveREL");
}

static unsigned int vdrive_rel_has_super(vdrive_t *vdrive)
{
    unsigned int super = 0;

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_2040:
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_1571:
        case VDRIVE_IMAGE_FORMAT_8050:
            /* no super side sector */
            super = 0;
            break;
        case VDRIVE_IMAGE_FORMAT_1581:
        case VDRIVE_IMAGE_FORMAT_8250:
            /* has super side sector */
            super = 1;
            break;
        default:
            log_error(vdrive_rel_log,
                      "Unknown disk type %i.  Cannot determine if it supports super side sectors.",
                      vdrive->image_format);
            break;
    }

    return super;
}

static unsigned int vdrive_rel_blocks_max(vdrive_t *vdrive)
{
    unsigned int maximum = 0;

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_2040:
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_1571:
            /* 700 blocks + 6 side sectors */
            maximum = 700 + 6;
            break;
        case VDRIVE_IMAGE_FORMAT_1581:
            /* 3000 blocks + 4 side sector groups + 1 side sector + super side
               sector */
            maximum = 3000 + 4 * 6 + 1 + 1;
            break;
        case VDRIVE_IMAGE_FORMAT_8050:
            /* 720 blocks + 6 side sectors */
            maximum = 720 + 6;
            break;
        case VDRIVE_IMAGE_FORMAT_8250:
            /* SFD1001: 4089 blocks + 5 side sector groups + 5 side sector +
               super side sector */
            /* 8250: 4090 blocks + 5 side sector groups + 5 side sector +
               super side sector */
            /* The SFD cannot create a file with REL 4090 blocks, but it can
               read it.  We will therefore use 4090 as our limit. */
            maximum = 4090 + 5 * 6 + 5 + 1;
            break;
        default:
            log_error(vdrive_rel_log,
                      "Unknown disk type %i.  Cannot determine max REL size.",
                      vdrive->image_format);
            break;
    }

    return maximum;
}

static void vdrive_rel_commit(vdrive_t *vdrive, bufferinfo_t *p)
{
    /* Check for writes here to commit the buffers. */
    if (p->needsupdate & DIRTY_SECTOR) {
        /* Write the sector */
        vdrive_write_sector(vdrive, p->buffer, p->track, p->sector);
        /* Clear flag for next sector */
        p->needsupdate &= ~(DIRTY_SECTOR);
    }

    return;
}

static int vdrive_rel_add_sector(vdrive_t *vdrive, unsigned int secondary, unsigned int *track, unsigned int *sector)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);
    unsigned int i, j, k, l, m, side, o;
    unsigned int t_new, s_new, t_super, s_super, current;
    int retval;
    BYTE *slot = p->slot;

    /* Find the total blocks this REL file uses. */
    i = slot[SLOT_NR_BLOCKS] + (slot[SLOT_NR_BLOCKS + 1] << 8);

    /* Compare it with the maximum for the particular disk image. */
    if (i >= vdrive_rel_blocks_max(vdrive)) {
        /* If we are equal or over, report error 52 */
        vdrive_command_set_error(vdrive, CBMDOS_IPE_TOOLARGE, 0, 0);
        return 1;
    }

    /* find the number of side sector groups */
    o = OFFSET_SUPER_POINTER;
    for (side = 0; side < SIDE_SUPER_MAX && p->super_side_sector[o] != 0; side++, o += 2) {}

    /* If the file isn't new, find the last record */
    if (side) {
        side--;

        /* Find last side sector; guaranteed find. */
        o = 256 * ( side * SIDE_SECTORS_MAX ) + OFFSET_NEXT_TRACK;
        for (i = 0; i < SIDE_SECTORS_MAX; i++, o += 256) {
            if (p->side_sector[o] == 0) {
                break;
            }
        }

        /* obtain the last byte of the sector according to the index */
        j = ( p->side_sector[256 * ( i + side * SIDE_SECTORS_MAX) + OFFSET_NEXT_SECTOR]
              + 1 - OFFSET_POINTER ) / 2;

    #if 0
        /* Scan last side sector for a null track and sector pointer;
            not guaranteed to be found. */
        o = 256 * ( i + side * SIDE_SECTORS_MAX) + OFFSET_POINTER;
        for (j = 0; j < SIDE_INDEX_MAX; j++, o += 2) {
            if (p->side_sector[o] == 0) {
                break;
            }
        }

        /* obtain the last byte of the sector according to the index */
        o = 256 * ( i + side * SIDE_SECTORS_MAX) + OFFSET_NEXT_SECTOR;
        k = p->side_sector[o];

        /* generate a last byte index based on the actual sectors used */
        l = OFFSET_POINTER + 2 * j - 1;

        /* compare them */
        if (k != l) {
            /* something is wrong with this rel file, try to fix it */
            log_error(vdrive_rel_log,
                      "Relative file ending sector and side sectors don't match up.");
            /* base the new value on the sector references, not the index. */
            p->side_sector[o] = l;
            p->side_sector_needsupdate[i + side * SIDE_SECTORS_MAX] = 1;
        }
    #endif

        /* Get the track and sector of the last REL block */
        o = 256 * ( i + side * SIDE_SECTORS_MAX) + OFFSET_POINTER + 2 * (j - 1);
        *track = p->side_sector[o];
        *sector = p->side_sector[o + 1];

        /* Find a new sector */
        retval = vdrive_bam_alloc_next_free_sector(vdrive, track, sector);
    } else {
        /* New REL file, get ready to create... */
        i = 0;
        j = 0;

        *track = 0;
        *sector = 0;

        /* Find the first new sector */
        retval = vdrive_bam_alloc_first_free_sector(vdrive, track, sector);
    }

    /* Leave if no space left */
    if (retval < 0) {
        vdrive_command_set_error(vdrive, CBMDOS_IPE_DISK_FULL, 0, 0);
        return 1;
    }
    /* Check if this side sector is full or if we need on in general. */
    if (j == SIDE_INDEX_MAX || j == 0) {
        /* Allocate a new sector for the new side sector. */
        t_new = *track;
        s_new = *sector;
        retval = vdrive_bam_alloc_next_free_sector(vdrive, &t_new, &s_new);
        /* If no space, leave with no changes */
        if (retval < 0) {
            vdrive_command_set_error(vdrive, CBMDOS_IPE_DISK_FULL, 0, 0);
            return 1;
        }
        /* We will adjust the side sector later, but atleast we know
            we have a place to make adjustments. */
    }

    /* setup for later */
    k = 0;
    m = p->slot[SLOT_RECORD_LENGTH];

    /* remember old current record, we will need it for later. */
    current = p->record + 1;

    /* If this is a unallocated file... */
    if (j == 0) {
        /* Update slot information if this is our first side sector. */
        p->slot[SLOT_FIRST_TRACK] = *track;
        p->slot[SLOT_FIRST_SECTOR] = *sector;

        /* Update the super side sector */
        p->super_side_sector[OFFSET_NEXT_TRACK] = t_new;
        p->super_side_sector[OFFSET_NEXT_SECTOR] = s_new;
        p->super_side_sector[OFFSET_SUPER_254] = 254;
        p->super_side_sector[OFFSET_SUPER_POINTER] = t_new;
        p->super_side_sector[OFFSET_SUPER_POINTER + 1] = s_new;
        p->super_side_sector_needsupdate = 1;

        t_super = t_new;
        s_super = s_new;

        /* Does this image require a real super side sector? */
        if (vdrive_rel_has_super(vdrive)) {
            retval = vdrive_bam_alloc_next_free_sector(vdrive, &t_super, &s_super);
            /* If no space, leave with no changes */
            if (retval < 0) {
                vdrive_command_set_error(vdrive, CBMDOS_IPE_DISK_FULL, 0, 0);
                return 1;
            }
            p->super_side_sector_track = t_super;
            p->super_side_sector_sector = s_super;
        } else {
            /* No super side sector required for this image */
            p->super_side_sector_track = 0;
            p->super_side_sector_sector = 0;
        }

        p->slot[SLOT_SIDE_TRACK] = t_super;
        p->slot[SLOT_SIDE_SECTOR] = s_super;

        /* set up first data block */
        p->track_next = *track;
        p->sector_next = *sector;

        /* Update the directory entry */
        vdrive_iec_update_dirent(vdrive, secondary);
    } else {
        /* Existing... */
        /* Move to last sector, use position command - this will flush
            any dirty buffers too. */
        vdrive_rel_position(vdrive, secondary, p->record_max & 255,
                            p->record_max >> 8, 1);

        /* Check whether record is split over two sectors to determine
           the last sector of the relative file. */
        if (p->bufptr + p->slot[SLOT_RECORD_LENGTH] > 256) {
            BYTE *tmp;

            /* Commit the buffers. */
            vdrive_rel_commit(vdrive, p);

            /* Swap the two buffers */
            tmp = p->buffer;
            p->buffer = p->buffer_next;
            p->buffer_next = tmp;
            p->track = p->track_next;
            p->sector = p->sector_next;

            o = p->bufptr + m - 254;
        } else {
            o = p->bufptr + m;
        }

        /* Modify this sector to connect to the next one. */
        /* We won't use the pointer in this sector for the last used
            byte as it could very well be wrong.  The 1541/71/81 had a
            tendency to mess the last sector when making REL files
            by expanding in small chunks.  This generally would happen
            when a new side sector was created.  I can generate a case
            where this bug happens, but I can't seem to figure out why
            - yet. */
        p->buffer[OFFSET_NEXT_TRACK] = p->track_next = *track;
        p->buffer[OFFSET_NEXT_SECTOR] = p->sector_next = *sector;

        /* Fill the new records up with the default 0xff 0x00 ... */
        while (o < 256)
        {
            if (k == 0) {
                p->buffer[o] = 0xff;
            } else {
                p->buffer[o] = 0x00;
            }
            k = ( k + 1 ) % m;
            /* increment the maximum records each time we complete a full
                record. */
            if (k == 0) {
                p->record_max++;
            }
            o++;
        }

        /* flag the current sector as dirty - the other functions will
            update it. */
        p->needsupdate = DIRTY_SECTOR;
    }

    /* Fill new sector with maximum records */
    o = 2;
    while (o < 256)
    {
        if (k == 0) {
            p->buffer_next[o] = 0xff;
        } else {
            p->buffer_next[o] = 0x00;
        }
        k = ( k + 1 ) % m;
        /* increment the maximum records each time we complete a full
            record. */
        if (k == 0) {
            p->record_max++;
        }
        o++;
    }

    /* set as last sector in REL file */
    p->buffer_next[OFFSET_NEXT_TRACK] = 0;

    /* Update the last byte based on how much of the last record we
        filled. */
    p->buffer_next[OFFSET_NEXT_SECTOR] = 255 - k;

    /* update the "next" buffer to disk, we don't have a dirty flag for
        it. */
    vdrive_write_sector(vdrive, p->buffer_next, p->track_next, p->sector_next);

    /* If this is the first side sector being made */
    if (j == 0) {
        /* Build some of the structure */
        p->side_sector[OFFSET_NEXT_TRACK] = 0;
        p->side_sector[OFFSET_RECORD_LEN] = m;
        p->side_sector[OFFSET_SIDE_SECTOR] = t_new;
        p->side_sector[OFFSET_SIDE_SECTOR + 1] = s_new;
        p->side_sector_track[0] = t_new;
        p->side_sector_sector[0] = s_new;
        /* Follow through with the "else" of the next if */
    }

    /* If this side sector is full... */
    if (j == SIDE_INDEX_MAX) {
        /* Update previous side sector */
        o = ( i + side * SIDE_SECTORS_MAX);
        /* dirty the side sector. */
        p->side_sector_needsupdate[o] = 1;
        o *= 256;
        /* Update the link. */
        p->side_sector[o + OFFSET_NEXT_TRACK] = t_new;
        p->side_sector[o + OFFSET_NEXT_SECTOR] = s_new;

        /* Is this the last side sector of a group in a super side sector? */
        if (i == SIDE_SECTORS_MAX - 1) {
            /* Yes, create a new group. */
            /* correct side reference. */
            side++;

            /* reallocate memory for another side sector group */
            o = SIDE_SECTORS_MAX * 256;
            p->side_sector = lib_realloc(p->side_sector, ( side + 1 ) * o );
            /* clear out new portion, the function may not do this */
            memset(&(p->side_sector[side * o]), 0, o);

            /* Also reallocate and clear out new sections of track and
                sectors locations and dirty flag of    each side sector */
            o = ( side + 1 ) * SIDE_SECTORS_MAX;
            p->side_sector_track = lib_realloc(p->side_sector_track, o);
            p->side_sector_sector = lib_realloc(p->side_sector_sector, o);
            p->side_sector_needsupdate = lib_realloc(p->side_sector_needsupdate, o);
            o = side * SIDE_SECTORS_MAX;
            memset(&(p->side_sector_track[o]), 0, SIDE_SECTORS_MAX);
            memset(&(p->side_sector_sector[o]), 0, SIDE_SECTORS_MAX);
            memset(&(p->side_sector_needsupdate[o]), 0, SIDE_SECTORS_MAX);

            /* Create first side sector of new group */
            o *= 256;
            p->side_sector[o + OFFSET_SIDE_SECTOR] = t_new;
            p->side_sector[o + OFFSET_SIDE_SECTOR + 1] = s_new;
            p->side_sector[o + OFFSET_SECTOR_NUM] = 0;

            /* Adjust the super side sector */
            o = OFFSET_SUPER_POINTER + side * 2;
            p->super_side_sector[o] = t_new;
            p->super_side_sector[o + 1] = s_new;
            p->super_side_sector_needsupdate = 1;

            /* Set up reference to the first side sector. */
            o = ( side * SIDE_SECTORS_MAX);
        } else {
            /* Nope, update old group. */
            /* Update side sector indices. */
            l = o = 256 * ( side * SIDE_SECTORS_MAX );
            for (k = 0; k <= i; k++, o += 256) {
                p->side_sector[o + OFFSET_SIDE_SECTOR + ( i + 1 ) * 2] = t_new;
                p->side_sector[o + OFFSET_SIDE_SECTOR + ( i + 1 ) * 2 + 1] = s_new;
            }
            /* Update the new sector */
            p->side_sector[o + OFFSET_SECTOR_NUM] = i + 1;
            /* Copy side sector track and sectors from first side sector. */
            for (k = 0; k < SIDE_SECTORS_MAX * 2; k++) {
                p->side_sector[o + OFFSET_SIDE_SECTOR + k] = p->side_sector[l + OFFSET_SIDE_SECTOR + k];
            }
            o = ( side * SIDE_SECTORS_MAX );
            /* Mark all of other side sectors as dirty. */
            for (k = 0; k <= i; k++, o++) {
                p->side_sector_needsupdate[o] = 1;
            }
            /* Set up the reference to the new side sector. */
            o = ((i + 1) + side * SIDE_SECTORS_MAX);
        }

        /* dirty the side sector. */
        p->side_sector_needsupdate[o] = 1;
        /* update the internal references. */
        p->side_sector_track[o] = t_new;
        p->side_sector_sector[o] = s_new;
        /* Update the side sector contents. */
        o *= 256;
        p->side_sector[o + OFFSET_NEXT_TRACK] = 0;
        p->side_sector[o + OFFSET_NEXT_SECTOR] = OFFSET_POINTER + 1;
        p->side_sector[o + OFFSET_RECORD_LEN] = m;
        p->side_sector[o + OFFSET_POINTER] = *track;
        p->side_sector[o + OFFSET_POINTER + 1] = *sector;
    } else {
        /* Update last side sector with new data. */
        o = ( i + side * SIDE_SECTORS_MAX );
        /* set sector dirty. */
        p->side_sector_needsupdate[o] = 1;
        /* update last byte used. */
        o *= 256;
        p->side_sector[o + OFFSET_NEXT_SECTOR] = OFFSET_POINTER + 2 * j + 1;
        o += OFFSET_POINTER + 2 * j;
        /* update track and sector of data. */
        p->side_sector[o] = *track;
        p->side_sector[o + 1] = *sector;
    }

    /* Move back to original record - it may not even exist. */
    vdrive_rel_position(vdrive, secondary, current & 255,
                        current >> 8, 1);

    /* everything is okay. */
    return 0;
}

static void vdrive_rel_fillrecord(vdrive_t *vdrive, unsigned int secondary)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);

    /* Write the rest of the current record */
    if (p->needsupdate & DIRTY_RECORD) {
        /* Fill remaining portion of record with zeros. */
        while (p->bufptr < p->record_next) {
            if (p->bufptr < 256) {
                /* speed this up if we are in the same sector. */
                p->buffer[p->bufptr] = 0;
                p->bufptr++;
                /* keep setting the data as dirty - the write function
                    may set it as clean when it commits the record. */
                p->needsupdate |= DIRTY_SECTOR;
            } else {
                /* if we have to move to the next sector, just use the
                    write routine. */
                vdrive_rel_write(vdrive, 0, secondary);
            }
        }
        /* Clear dirty record flag */
        p->needsupdate &= (~DIRTY_RECORD );
    }
    /* Clear written flag */
    p->needsupdate &= (~WRITTEN_RECORD );

    return;
}

static void vdrive_rel_flush_sidesectors(vdrive_t *vdrive, bufferinfo_t *p)
{
    unsigned int i, j, o, side;

    /* Write super side sector if it is dirty and if it is not imaginary */
    if (p->super_side_sector_needsupdate && p->super_side_sector_track) {
        /* Write the super side sector */
        vdrive_write_sector(vdrive, p->super_side_sector, p->super_side_sector_track, p->super_side_sector_sector);
        /* Clear flag for super side sector */
        p->super_side_sector_needsupdate = 0;
    }

    /* find the number of side sector groups */
    for (side = 0; p->super_side_sector[OFFSET_SUPER_POINTER + side * 2] != 0; side++) {}

    o = 0;
    for (j = 0; j < side; j++) {
        for (i = 0; i < SIDE_SECTORS_MAX; i++) {
            if (p->side_sector_needsupdate[o] && p->side_sector_track[o]) {
                /* Write the super side sector */
                vdrive_write_sector(vdrive, &(p->side_sector[o * 256]), p->side_sector_track[o], p->side_sector_sector[o]);

                /* Clear flag for super side sector */
                p->side_sector_needsupdate[o] = 0;
            }
            o++;
        }
    }

    return;
}

static int vdrive_rel_grow(vdrive_t *vdrive, unsigned int secondary,
                           unsigned int records)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);
    unsigned int track, sector;
    unsigned int i, j, k, l = 0;

    /* Add a sector to the rel file until we meet the required
        records. */
    while (records >= p->record_max) {
        l = vdrive_rel_add_sector(vdrive, secondary, &track, &sector);
        if (l) {
            break;
        }
    }

    /* Flush the side sectors, since they have changed */
    vdrive_rel_flush_sidesectors(vdrive, p);

    /* Update the BAM. */
    vdrive_bam_write_bam(vdrive);

    /* Update block count on file expansions. */

    /* Find the total blocks this REL file uses. */
    k = p->slot[SLOT_NR_BLOCKS] + (p->slot[SLOT_NR_BLOCKS + 1] << 8);

    /* determine the block count of the data */
    i = p->record_max * p->slot[SLOT_RECORD_LENGTH];
    j = i / 254;
    if (i % 254) {
        j++;
    }
    /* determine the block count of the side sectors */
    i = j / SIDE_INDEX_MAX;
    if (j % SIDE_INDEX_MAX) {
        i++;
    }
    /* Add another block for a super side sector */
    if (p->super_side_sector_track) {
        i++;
    }
    /* Sum them up */
    i = i + j;

    /* Compare with the current */
    if (i != k) {
        /* Convert to high/low */
        p->slot[SLOT_NR_BLOCKS] = i % 256;
        p->slot[SLOT_NR_BLOCKS + 1] = i / 256;
        /* Update the directory entry (block count) */
        vdrive_iec_update_dirent(vdrive, secondary);
    }

    return l;
}

static int vdrive_rel_open_existing(vdrive_t *vdrive, unsigned int secondary)
{
    unsigned int track, sector, side, i, j, o;
    bufferinfo_t *p = &(vdrive->buffers[secondary]);
    BYTE *slot;

    slot = p->slot;

    /* Create our own slot, since the one passed is static */
    p->slot = lib_calloc(1, 32);

    /* Copy the static on to the new one. */
    memcpy(p->slot, slot, 32);

    /* read super side sector, if it exists */

    track = p->slot[SLOT_SIDE_TRACK];
    sector = p->slot[SLOT_SIDE_SECTOR];

    /* Allocate memory for super side sector */
    p->super_side_sector = lib_malloc(256);

    if (vdrive_read_sector(vdrive, p->super_side_sector, track, sector) != 0) {
        log_error(vdrive_rel_log, "Cannot read side sector.");
        lib_free((char *)p->super_side_sector);
        return -1;
    }

    /* check to see if this is a super side sector.  If not, create an
        imaginary one so we don't have to change all our code. */
    if (p->super_side_sector[OFFSET_SUPER_254] != 254) {
        /* clear out block */
        memset(p->super_side_sector, 0, 256);
        /* build valid super side sector block */
        p->super_side_sector[OFFSET_NEXT_TRACK] = track;
        p->super_side_sector[OFFSET_NEXT_SECTOR] = sector;
        p->super_side_sector[OFFSET_SUPER_254] = 254;
        p->super_side_sector[OFFSET_SUPER_POINTER] = track;
        p->super_side_sector[OFFSET_SUPER_POINTER + 1] = sector;
        /* set track and sector to 0, i.e. no update */
        p->super_side_sector_track = 0;
        p->super_side_sector_sector = 0;
    } else {
        /* set track and sector */
        p->super_side_sector_track = track;
        p->super_side_sector_sector = sector;
    }
    /* Clear dirty flag */
    p->super_side_sector_needsupdate = 0;

    /* find the number of side sector groups */
    for (side = 0; p->super_side_sector[OFFSET_SUPER_POINTER + side * 2] != 0; side++) {}

    /* allocate memory for side sectors */
    p->side_sector = lib_malloc(side * SIDE_SECTORS_MAX * 256);
    memset(p->side_sector, 0, side * SIDE_SECTORS_MAX * 256);

    /* Also clear out track and sectors locations and dirty flag of
        each side sector */
    p->side_sector_track = lib_malloc(side * SIDE_SECTORS_MAX);
    p->side_sector_sector = lib_malloc(side * SIDE_SECTORS_MAX);
    p->side_sector_needsupdate = lib_malloc(side * SIDE_SECTORS_MAX);
    memset(p->side_sector_track, 0, side * SIDE_SECTORS_MAX);
    memset(p->side_sector_sector, 0, side * SIDE_SECTORS_MAX);
    memset(p->side_sector_needsupdate, 0, side * SIDE_SECTORS_MAX);

    for (j = 0; j < side; j++) {
        track = p->super_side_sector[OFFSET_SUPER_POINTER + j * 2];
        sector = p->super_side_sector[OFFSET_SUPER_POINTER + j * 2 + 1];

        for (i = 0; i < SIDE_SECTORS_MAX; i++) {
            o = i + j * SIDE_SECTORS_MAX;
            /* Save the track and sector of each side sector so we can also write
               and update REL files. */
            p->side_sector_track[o] = track;
            p->side_sector_sector[o] = sector;

            o *= 256;

            if (vdrive_read_sector(vdrive, &(p->side_sector[o]), track, sector) != 0) {
                log_error(vdrive_rel_log, "Cannot read side sector.");
                return -1;
            }
            if (p->side_sector[o + OFFSET_SECTOR_NUM] != i) {
                log_error(vdrive_rel_log, "Side sector number do not match.");
                return -1;
            }

            track = p->side_sector[o + OFFSET_NEXT_TRACK];
            sector = p->side_sector[o + OFFSET_NEXT_SECTOR];

            if (track == 0) {
                break;
            }
        }
    }

    return 0;
}

static int vdrive_rel_open_new(vdrive_t *vdrive, unsigned int secondary,
                               cbmdos_cmd_parse_t *cmd_parse, const BYTE *name)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);
    BYTE *slot;

#ifdef DEBUG_DRIVE
    log_debug("vdrive_rel_open_new: Name (%d) '%s'",
              cmd_parse->parselength, cmd_parse->parsecmd);
#endif

    /* Allocate a directory slot */
    vdrive_dir_find_first_slot(vdrive, NULL, -1, 0, &p->dir);
    slot = vdrive_dir_find_next_slot(&p->dir);

    /* If we can't get one the directory is full - disk full */
    if (!slot) {
        vdrive_command_set_error(vdrive, CBMDOS_IPE_DISK_FULL, 0, 0);
        return 1;
    }

    /* Create our own slot */
    p->slot = lib_calloc(1, 32);

    /* Populate it */
    memset(p->slot + SLOT_NAME_OFFSET, 0xa0, 16);
    memcpy(p->slot + SLOT_NAME_OFFSET, cmd_parse->parsecmd, cmd_parse->parselength);
#ifdef DEBUG_DRIVE
    log_debug("DIR: Created dir slot. Name (%d) '%s'",
              cmd_parse->parselength, cmd_parse->parsecmd);
#endif
    p->slot[SLOT_TYPE_OFFSET] = cmd_parse->filetype | 0x80;       /* closed */

    p->slot[SLOT_RECORD_LENGTH] = cmd_parse->recordlength;

    /* Store in buffer */
    memcpy(&(p->dir.buffer[p->dir.slot * 32 + 2]), p->slot + 2, 30);

#ifdef DEBUG_DRIVE
    log_debug("DEBUG: write DIR slot (%d %d).",
              vdrive->Curr_track, vdrive->Curr_sector);
#endif
    /* Write the sector */
    vdrive_write_sector(vdrive, p->dir.buffer, p->dir.track, p->dir.sector);

    /* Allocate memory for super side sector */
    p->super_side_sector = lib_malloc(256);

    /* clear out block */
    memset(p->super_side_sector, 0, 256);
    /* build valid super side sector block */
    p->super_side_sector[OFFSET_SUPER_254] = 254;
    p->super_side_sector_track = 0;
    p->super_side_sector_sector = 0;
    /* Clear dirty flag */
    p->super_side_sector_needsupdate = 0;

    /* allocate memory for side sectors */
    p->side_sector = lib_malloc(SIDE_SECTORS_MAX * 256);
    memset(p->side_sector, 0, SIDE_SECTORS_MAX * 256);

    /* Also clear out track and sectors locations and dirty flag of
        each side sector */
    p->side_sector_track = lib_malloc(SIDE_SECTORS_MAX);
    p->side_sector_sector = lib_malloc(SIDE_SECTORS_MAX);
    p->side_sector_needsupdate = lib_malloc(SIDE_SECTORS_MAX);
    memset(p->side_sector_track, 0, SIDE_SECTORS_MAX);
    memset(p->side_sector_sector, 0, SIDE_SECTORS_MAX);
    memset(p->side_sector_needsupdate, 0, SIDE_SECTORS_MAX);

    /* Everything okay */
    return 0;
}

int vdrive_rel_open(vdrive_t *vdrive, unsigned int secondary,
                    cbmdos_cmd_parse_t *cmd_parse, const BYTE *name)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);
    int newrelfile = 0;

    if (p->slot) {
        log_debug(
            "Open existing REL file '%s' with record length %i on channel %d.",
            name, cmd_parse->recordlength, secondary);
        /* Follow through to function. */
        if (vdrive_rel_open_existing(vdrive, secondary)) {
            return SERIAL_ERROR;
        }
    } else {
        log_debug(
            "Open new REL file '%s' with record length %i on channel %d.",
            name, cmd_parse->recordlength, secondary);

        /* abort if we are in read only mode */
        if (vdrive->image->read_only || VDRIVE_IMAGE_FORMAT_4000_TEST) {
            vdrive_command_set_error(vdrive, CBMDOS_IPE_WRITE_PROTECT_ON, 0, 0);
            return SERIAL_ERROR;
        }

        /* Call function to open a new REL file, leave if error exists. */
        if (vdrive_rel_open_new(vdrive, secondary, cmd_parse, name)) {
            return SERIAL_ERROR;
        }
        /* set a flag so we can expand the rel file to 1 record later. */
        newrelfile++;
    }

    /* Allocate dual buffers to improve performance */
    p->mode = BUFFER_RELATIVE;
    p->bufptr = 0;
    p->buffer = lib_malloc(256);
    p->record = 0;
    p->track = 0;
    p->sector = 0;
    p->buffer_next = lib_malloc(256);
    p->track_next = 0;
    p->sector_next = 0;

    /* Determine maximum record */
    p->record_max = vdrive_rel_record_max(vdrive, secondary);

    /* If this is a new REL file, have atleast 1 record. */
    if (newrelfile) {
        vdrive_rel_grow(vdrive, secondary, 0);
    }

    /* Move to first record, no offset */
    vdrive_rel_position(vdrive, secondary, 1, 0, 1);

    return SERIAL_OK;
}

void vdrive_rel_track_sector(vdrive_t *vdrive, unsigned int secondary,
                             unsigned int record, unsigned int *track,
                             unsigned int *sector, unsigned int *rec_start)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);
    unsigned int rec_long, side, offset, rec_len, super;

    /* find the record length from the first side sector */
    rec_len = p->slot[SLOT_RECORD_LENGTH];

    /* compute the total byte offset */
    rec_long = (record * rec_len);

    /* find the sector offset */
    *rec_start = rec_long % 254;

    /* find the super side sector (0 - 125) */
    offset = (254 * SIDE_INDEX_MAX * SIDE_SECTORS_MAX);
    super = rec_long / offset;
    rec_long = rec_long % offset;

    /* find the side sector (0-5) */
    offset = (254 * SIDE_INDEX_MAX);
    side = rec_long / offset;
    rec_long = rec_long % offset;

    /* find the offset into the side sector (0-120) */
    offset = ( rec_long / 254 ) * 2;

    /* add super, side sector and index offsets */
    offset += ( super * SIDE_SECTORS_MAX + side ) * 256 + OFFSET_POINTER;

    *track = p->side_sector[offset];
    *sector = p->side_sector[offset + 1];

    return;
}

static unsigned int vdrive_rel_record_max(vdrive_t *vdrive, unsigned int secondary)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);

    unsigned int i, j, k, l, side, o;
    unsigned int track, sector;

    /* The original 1541 documentation states that there can only be 720
        (6*120) records in a REL file - this is incorrect.  It is actually
        blocks.  The side sectors point to each block in the REL file.  It is
        up to the drive to determine the where the buffer pointer should be
        when the record changes. */

    /* To find the maximum records we have to check the last block of the REL
        file.  The last block is referenced in the last side sector group by
        the OFFSET_NEXT_SECTOR byte.  The next sector pointer after this one
        should be zero - we will check anyways to make sure.  Once the last
        block is found, we can move to that sector and check the used bytes
        there. */

    /* The maximum REL file size for a 1541/1571 is 700 blocks, although it
        is documented to be 720.  The 1581 is limited to 3000 blocks,
        although it is documented to be 90720 (126*6*120).  You think they
        would have allowed it to be atleast the maximum amount of disk
        space. */

    /* find the number of side sector groups */
    o = OFFSET_SUPER_POINTER;
    for (side = 0; side < SIDE_SUPER_MAX && p->super_side_sector[o] != 0; side++, o += 2) {}

    /* check if this is a NEW rel file - no data in super side sector. */
    if (!side) {
        /* return a maximum of 0. */
        return 0;
    }

    side--;

    /* Find last side sector; guaranteed find. */
    o = 256 * ( side * SIDE_SECTORS_MAX ) + OFFSET_NEXT_TRACK;
    for (i = 0; i < SIDE_SECTORS_MAX; i++, o += 256) {
        if (p->side_sector[o] == 0) {
            break;
        }
    }

    /* obtain the last byte of the sector according to the index */
    j = ( p->side_sector[256 * ( i + side * SIDE_SECTORS_MAX) + OFFSET_NEXT_SECTOR]
          + 1 - OFFSET_POINTER ) / 2;

#if 0
    /* Scan last side sector for a null track and sector pointer;
        not guaranteed to be found. */
    o = 256 * ( i + side * SIDE_SECTORS_MAX) + OFFSET_POINTER;
    for (j = 0; j < SIDE_INDEX_MAX; j++, o += 2) {
        if (p->side_sector[o] == 0) {
            break;
        }
    }

    /* obtain the last byte of the sector according to the index */
    k = p->side_sector[256 * ( i + side * SIDE_SECTORS_MAX) + OFFSET_NEXT_SECTOR];

    /* generate a last byte index based on the actual sectors used */
    l = OFFSET_POINTER + 2 * j - 1;

    /* compare them */
    if (k != l) {
        /* something is wrong with this rel file, it should be repaired */
        log_error(vdrive_rel_log, "Relative file ending sector and side sectors don't match up.");
    }
#endif

    /* Get the track and sector of the last block */
    j--;
    o = 256 * ( i + side * SIDE_SECTORS_MAX ) + OFFSET_POINTER + 2 * j;
    track = p->side_sector[o];
    sector = p->side_sector[o + 1];

    /* read it in */
    if (vdrive_read_sector(vdrive, p->buffer, track, sector) != 0) {
        log_error(vdrive_rel_log, "Cannot read relative file data sector.");
        vdrive_command_set_error(vdrive, CBMDOS_IPE_ILLEGAL_TRACK_OR_SECTOR, track, sector);
        return 0;
    }

    /* calculate the total bytes based on the number of super side, side
        sectors, and last byte index */
    k = (( side * SIDE_SECTORS_MAX + i ) * SIDE_INDEX_MAX + j ) * 254 + p->buffer[OFFSET_NEXT_SECTOR] - 1;

    /* divide by the record length, and get the maximum records */
    l = k / p->slot[SLOT_RECORD_LENGTH];

    return l;
}

int vdrive_rel_position(vdrive_t *vdrive, unsigned int secondary,
                        unsigned int rec_lo, unsigned int rec_hi,
                        unsigned int position)
{
    unsigned int record, rec_start, rec_len;
    unsigned int track, sector;
    bufferinfo_t *p = &(vdrive->buffers[secondary]);

    /* find the record length from the first side sector */
    rec_len = p->slot[SLOT_RECORD_LENGTH];

    /* position 0 and 1 are the same - the first */
    position = (position == 0) ? 0 : position - 1;

    /* generate error 51 if we point to a position greater than the record
       length */
    if (position >= rec_len) {
        log_error(vdrive_rel_log, "Position larger than record!?");
        return 51;
    }

    /* generate a 16 bit value for the record from the two 8-bit values */
    record = rec_lo + (rec_hi << 8);
    /* record 0 and 1 are the same - the first */
    record = (record == 0) ? 0 : record - 1;

    p->record = record;
    /* if the record is greater then the maximum, return error 50, but
       remember the record for growth */
    if (record >= p->record_max) {
        if (record > 0) {
            return CBMDOS_IPE_NO_RECORD;
        } else {
            /* If the REL has just been created, report OK on record #1. */
            return CBMDOS_IPE_OK;
        }
    }

    /* Fill the rest of the currently written record. */
    vdrive_rel_fillrecord(vdrive, secondary);

    log_debug("Requested position %d, %d on channel %d.", record, position, secondary);

    /* locate the track, sector and sector offset of record */
    vdrive_rel_track_sector(vdrive, secondary, record, &track, &sector, &rec_start);

    /* Check to see if the next record is in the buffered sector */
    if (p->track_next == track && p->sector_next == sector) {
        BYTE *tmp;

        /* Commit the buffers. */
        vdrive_rel_commit(vdrive, p);

        /* Swap the two buffers */
        tmp = p->buffer;
        p->buffer = p->buffer_next;
        p->buffer_next = tmp;
        p->track_next = p->track;
        p->sector_next = p->sector;
        p->track = track;
        p->sector = sector;
    } else if (p->track != track || p->sector != sector) {
        /* Commit the buffers. */
        vdrive_rel_commit(vdrive, p);

        /* load in the sector to memory */
        if (vdrive_read_sector(vdrive, p->buffer, track, sector) != 0) {
            log_error(vdrive_rel_log, "Cannot read track %i sector %i.", track, sector);
            return 66;
        }
        p->track = track;
        p->sector = sector;
        /* keep buffered sector where ever it is */
        /* we won't read in the next sector unless we have to */
    }

    /* set the buffer pointer */
    p->bufptr = rec_start + 2 + position;
    /* compute the next pointer record */
    p->record_next = p->bufptr - position + rec_len;

    /* set the maximum record length */
    p->length = p->record_next - 1;

    /* It appears that all Commodore drives have the same problems handling
        REL files when it comes to using the position option of the record
        command.  Normally when a record is selected, the drive firmware
        scan from the end of the record to the beginning for when a non-NULL
        byte is found.  When it finds it, this is considered the end of a
        record (for a read).  The problem is: what happens when you position
        the read pointer past this end point?  An overflow occurs in the
        read of course!  Since the calculated record length is much larger
        than it should be (but less than 256) the drive can read up to a
        whole block of data    from the subsequent records.  Although this is
        a bug (from my point of view), we have to try to emulate it.  Apon
        analyzing the 1541 code (the 1571 and 1581 have the same behavior),
        at $e170 (the subroutine for searching for the non-NULL is at $e1b5)
        the    overflow may not occur under one specific instance.  This is
        when the start of the record is in one block and the selected
        position places it in another block.  In this case, only a length of
        1 byte is set as the record length.  In all other cases, the length
        is set to 255 - position. */

    /* Find the length of the record starting from the end until no zeros
       are found */
    if (p->length < 256) {
        /* If the maximum length of the record is in this sector, just
           check for where the zeros end */
        for (; p->length >= p->bufptr && p->buffer[p->length] == 0; p->length--) {}

        /* Compensate for overflow length bug, set maximum length based on
            the position requested */
        if (p->bufptr > p->length && position > 0) {
            p->length = p->bufptr + 254 - position;
        }
    } else {
        int status = 1;

        /* Get the next sector */
        /* If it doesn't exist, we will use the max length calculated above */
        if (p->buffer[0] != 0) {
            /* Read in the sector if it has not been buffered */
            if (p->buffer[0] != p->track_next || p->buffer[1] != p->sector_next) {
                status = vdrive_read_sector(vdrive, p->buffer_next, p->buffer[0], p->buffer[1]);
            } else {
                status = 0;
            }
        }
        /* If all is good, calculate the length now */
        if (!status) {
            /* update references, if the sector read in */
            p->track_next = p->buffer[0];
            p->sector_next = p->buffer[1];
            /* Start looking in the buffered sector */
            for (; p->length >= 256 && p->length >= p->bufptr && p->buffer_next[p->length - 256 + 2] == 0; p->length--) {}

            /* If we crossed into the current sector, look there too, but
                only if the position places us in the current sector.
                Otherwise, the length will be 1 at this point. */
            if (p->length < 256 && p->bufptr < 256) {
                for (; p->length >= p->bufptr && p->buffer[p->length] == 0; p->length--) {}

                /* Compensate for overflow length bug, set maximum length
                    based on the position requested */
                if (p->bufptr > p->length && position > 0) {
                    p->length = p->bufptr + 254 - position;
                }
            }
        }
    }

    return CBMDOS_IPE_OK;
}

int vdrive_rel_read(vdrive_t *vdrive, BYTE *data, unsigned int secondary)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);

    /* Check if we are past the end of the REL file. */
    if (p->record >= p->record_max) {
        *data = 0x0d;
        vdrive_command_set_error(vdrive, CBMDOS_IPE_NO_RECORD, 0, 0);
        return SERIAL_EOF;
    }

    /*
     * Read next block if needed
     */
    if (p->buffer[0]) {
        if (p->bufptr >= 256) {
            int status = 0;
            unsigned int track, sector;

            track = (unsigned int)p->buffer[0];
            sector = (unsigned int)p->buffer[1];

            /* Commit the buffers. */
            vdrive_rel_commit(vdrive, p);

            /* Check to see if the next record is in the buffered sector */
            if (p->track_next == track && p->sector_next == sector) {
                /* Swap the two buffers */
                BYTE *tmp;
                tmp = p->buffer;
                p->buffer = p->buffer_next;
                p->buffer_next = tmp;
                p->track_next = p->track;
                p->sector_next = p->sector;
                p->track = track;
                p->sector = sector;
                status = 0;
            } else if (p->track != track || p->sector != sector) {
                /* load in the sector to memory */
                status = vdrive_read_sector(vdrive, p->buffer, track, sector);
            }

            if (status == 0) {
                p->track = track;
                p->sector = sector;
                p->bufptr = p->bufptr - 254;
                p->length = p->length - 254;
                p->record_next = p->record_next - 254;
                /* keep buffered sector where ever it is */
                /* we won't read in the next sector unless we have to */
            } else {
                log_error(vdrive_rel_log, "Cannot read track %i sector %i.", track, sector);
                *data = 0xc7;
                return SERIAL_EOF;
            }
        }
    } else {
        if (p->bufptr >= (unsigned int)( p->buffer[1] + 2 )) {
            /* check to see if we have the pointer overflow problem here. */
            if (p->record_next > p->length) {
                *data = 0x0d;
    #ifdef DEBUG_DRIVE
                if (p->mode == BUFFER_COMMAND_CHANNEL) {
                    log_error(vdrive_rel_log,
                              "Disk read  %d [%02d %02d] data %02x (%c).",
                              p->mode, 0, 0, *data, (isprint(*data)
                                                     ? *data : '.'));
                }
    #endif
                vdrive_command_set_error(vdrive, CBMDOS_IPE_NO_RECORD, 0, 0);
                return SERIAL_EOF;
            } else {
                /* Yes, just rotate though to this buffer. */
                if (p->bufptr >= 256) {
                    p->bufptr = p->bufptr - 254;
                    p->length = p->length - 254;
                    p->record_next = p->record_next - 254;
                }
            }
        }
    }

    *data = p->buffer[p->bufptr];
    p->bufptr++;

    if (p->bufptr > p->length) {
        /* set up the buffer pointer to the next record */
        p->bufptr = p->record_next;
        /* add the record length to the next record pointer */
        p->record_next += p->side_sector[OFFSET_RECORD_LEN];
        /* calculate the maximum length (for read, for write we should use
            record_next - 1 */
        p->length = p->record_next - 1;
        /* increment the record reference */
        p->record++;
        /* Exit if we have hit the end of the REL data. */
        if (p->record >= p->record_max) {
            return SERIAL_EOF;
        }
        /* Find the length of the record starting from the end until no zeros
            are found */
        if (p->length < 256) {
            /* If the maximum length of the record is in this sector, just
            check for where the zeros end */
            for (; p->length >= p->bufptr && p->buffer[p->length] == 0; p->length--) {
            }
        } else {
            int status = 1;

            /* Get the next sector */
            /* If it doesn't exist, we will use the max length calculated above */
            if (p->buffer[0] != 0) {
                /* Read in the sector if it has not been buffered */
                if (p->buffer[0] != p->track_next || p->buffer[1] != p->sector_next) {
                    status = vdrive_read_sector(vdrive, p->buffer_next, p->buffer[0], p->buffer[1]);
                } else {
                    status = 0;
                }
            }
            /* If all is good, calculate the length now */
            if (!status) {
                /* update references, if the sector read in */
                p->track_next = p->buffer[0];
                p->sector_next = p->buffer[1];
                /* Start looking in the buffered sector */
                for (; p->length >= 256 && p->buffer_next[p->length - 256 + 2] == 0; p->length--) {
                }
                /* If we crossed into the current sector, look there too */
                if (p->length < 256) {
                    for (; p->length >= p->bufptr && p->buffer[p->length] == 0; p->length--) {
                    }
                }
            }
        }
        log_debug("Forced from read to position %d, 0 on channel %d.", p->record, secondary);

        return SERIAL_EOF;
    }

    return SERIAL_OK;
}

int vdrive_rel_write(vdrive_t *vdrive, BYTE data, unsigned int secondary)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);

    if (vdrive->image->read_only || VDRIVE_IMAGE_FORMAT_4000_TEST) {
        vdrive_command_set_error(vdrive, CBMDOS_IPE_WRITE_PROTECT_ON, 0, 0);
        return SERIAL_ERROR;
    }

    /* Check if we need to grow the REL file. */
    if (p->record >= p->record_max) {
        if (vdrive_rel_grow(vdrive, secondary, p->record) < 0) {
            /* Couldn't grow it, error generated in function. */
            return SERIAL_OK;
        }
    }

    /*
     * Read next block if needed
     */
    if (p->buffer[0]) {
        if (p->bufptr >= 256) {
            int status = 0;
            unsigned int track, sector;

            track = (unsigned int)p->buffer[0];
            sector = (unsigned int)p->buffer[1];

            /* Commit the buffers. */
            vdrive_rel_commit(vdrive, p);

            /* Check to see if the next record is in the buffered sector */
            if (p->track_next == track && p->sector_next == sector) {
                /* Swap the two buffers */
                BYTE *tmp;
                tmp = p->buffer;
                p->buffer = p->buffer_next;
                p->buffer_next = tmp;
                p->track_next = p->track;
                p->sector_next = p->sector;
                p->track = track;
                p->sector = sector;
                status = 0;
            } else if (p->track != track || p->sector != sector) {
                /* load in the sector to memory */
                status = vdrive_read_sector(vdrive, p->buffer, track, sector);
            }

            if (status == 0) {
                p->track = track;
                p->sector = sector;
                p->bufptr = p->bufptr - 254;
                p->length = p->length - 254;
                p->record_next = p->record_next - 254;
                /* keep buffered sector where ever it is */
                /* we won't read in the next sector unless we have to */
            } else {
                log_error(vdrive_rel_log, "Cannot read track %i sector %i.", track, sector);
                return SERIAL_EOF;
            }
        }
    } else {
        if (p->bufptr >= (unsigned int)( p->buffer[1] + 2 )) {
#ifdef DEBUG_DRIVE
            if (p->mode == BUFFER_COMMAND_CHANNEL) {
                log_error(vdrive_rel_log,
                          "Disk read  %d [%02d %02d] data %02x (%c).",
                          p->mode, 0, 0, data, (isprint(data)
                                                ? data : '.'));
            }
#endif
            if (vdrive_rel_grow(vdrive, secondary, p->record) < 0) {
                /* Couldn't grow it, error generated in function. */
                return SERIAL_OK;
            }
        }
    }

    if (p->bufptr >= p->record_next) {
        /* We CANNOT write into the next record under any
            circumstances, unlike the reads. */
        vdrive_command_set_error(vdrive, CBMDOS_IPE_OVERFLOW, 0, 0);

        /* But if we try, we still say it is okay, but with a DOS
            error. */
        return SERIAL_OK;
    }

    /* write the data to the buffer */
    p->buffer[p->bufptr] = data;
    /* increment the pointer */
    p->bufptr++;
    /* flag this sector as dirty and written */
    p->needsupdate |= ( DIRTY_SECTOR | WRITTEN_RECORD );

    /* Mark this record as dirty only if we are still in it */
    if (p->bufptr != p->record_next) {
        p->needsupdate |= DIRTY_RECORD;
    } else {
        p->needsupdate &= (~DIRTY_RECORD);
    }

    /* Any pointer overflow gets handled at the beginning of this
        function. */

    return SERIAL_OK;
}

int vdrive_rel_close(vdrive_t *vdrive, unsigned int secondary)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);

    log_debug("VDrive REL close channel %d.", secondary);

    /* Fill the rest of the currently written record. */
    vdrive_rel_fillrecord(vdrive, secondary);

    /* Commit the buffers. */
    vdrive_rel_commit(vdrive, p);

    p->mode = BUFFER_NOT_IN_USE;

    lib_free((char *)p->buffer);
    p->buffer = NULL;

    lib_free((char *)p->buffer_next);
    p->buffer_next = NULL;

    /* remove side sectors */
    lib_free(p->side_sector);
    p->side_sector = NULL;

    /* remove side sector tracks */
    lib_free(p->side_sector_track);
    p->side_sector_track = NULL;

    /* remove side sector sectors */
    lib_free(p->side_sector_sector);
    p->side_sector_sector = NULL;

    /* remove super side sector too */
    lib_free(p->super_side_sector);
    p->super_side_sector = NULL;

    /* remove dirty flags */
    lib_free(p->side_sector_needsupdate);
    p->side_sector_needsupdate = NULL;

    /* Free the slot */
    lib_free(p->slot);

    return SERIAL_OK;
}

void vdrive_rel_listen(vdrive_t *vdrive, unsigned int secondary)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);

    if (p->needsupdate & WRITTEN_RECORD) {
        /* Fill the rest of the currently written record. */
        vdrive_rel_fillrecord(vdrive, secondary);

        /* set up the buffer pointer to the next record */
        p->bufptr = p->record_next;
        /* add the record length to the next record pointer */
        p->record_next += p->side_sector[OFFSET_RECORD_LEN];
        /* calculate the maximum length (for read, for write we should use
            record_next - 1 */
        p->length = p->record_next - 1;
        /* increment the record reference */
        p->record++;
        /* Find the length of the record starting from the end until no zeros
            are found */
        if (p->length < 256) {
            /* If the maximum length of the record is in this sector, just
            check for where the zeros end */
            for (; p->length >= p->bufptr && p->buffer[p->length] == 0; p->length--) {
            }
        } else {
            int status = 1;

            /* Get the next sector */
            /* If it doesn't exist, we will use the max length calculated above */
            if (p->buffer[0] != 0) {
                /* Read in the sector if it has not been buffered */
                if (p->buffer[0] != p->track_next || p->buffer[1] != p->sector_next) {
                    status = vdrive_read_sector(vdrive, p->buffer_next, p->buffer[0], p->buffer[1]);
                } else {
                    status = 0;
                }
            }
            /* If all is good, calculate the length now */
            if (!status) {
                /* update references, if the sector read in */
                p->track_next = p->buffer[0];
                p->sector_next = p->buffer[1];
                /* Start looking in the buffered sector */
                for (; p->length >= 256 && p->buffer_next[p->length - 256 + 2] == 0; p->length--) {
                }
                /* If we crossed into the current sector, look there too */
                if (p->length < 256) {
                    for (; p->length >= p->bufptr && p->buffer[p->length] == 0; p->length--) {
                    }
                }
            }
        }
        log_debug("Forced from write to position %d, 0 on channel %d.", p->record, secondary);
    }
}
