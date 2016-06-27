/*
 * grc.h - GCR handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 * Additional changes by
 *  Robert McIntyre <rjmcinty@hotmail.com>
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

#ifndef VICE_GCR_H
#define VICE_GCR_H

#include "types.h"
#include "cbmdos.h"

/* Number of bytes in one raw track. For usage with D64/D71 */
#define NUM_MAX_BYTES_TRACK 7928

/* Number of bytes in one raw track in memory. 64k big to avoid buffer overrun, because
 * the G64 track size field is a 16-bit word */
#define NUM_MAX_MEM_BYTES_TRACK 65536

/* Number of tracks we emulate. 84 for 1541, 140 for 1571 */
#define MAX_GCR_TRACKS 140

#define SECTOR_GCR_SIZE_WITH_HEADER 340

typedef struct disk_track_s {
    BYTE *data;
    int size;
} disk_track_t;

typedef struct gcr_s {
    /* Raw GCR image of the disk.  */
    disk_track_t tracks[MAX_GCR_TRACKS];
} gcr_t;

typedef struct gcr_header_s {
    BYTE sector, track, id2, id1;
} gcr_header_t;

extern void gcr_convert_sector_to_GCR(const BYTE *buffer, BYTE *ptr, const gcr_header_t *header,
                                      int gap, int sync, enum fdc_err_e error_code);
extern enum fdc_err_e gcr_read_sector(const disk_track_t *raw, BYTE *data, BYTE sector);
extern enum fdc_err_e gcr_write_sector(disk_track_t *raw, const BYTE *data, BYTE sector);

extern gcr_t *gcr_create_image(void);
extern void gcr_destroy_image(gcr_t *gcr);
#endif
