/*
 * vdrive-dir.h - Virtual disk-drive implementation.
 *                Directory specific functions.
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

#ifndef VICE_VDRIVE_DIR_H
#define VICE_VDRIVE_DIR_H

#include "types.h"
#include "cbmdos.h"

#define SLOT_TYPE_OFFSET      2
#define SLOT_FIRST_TRACK      3
#define SLOT_FIRST_SECTOR     4
#define SLOT_NAME_OFFSET      5
#define SLOT_SIDE_TRACK       21
#define SLOT_SIDE_SECTOR      22
#define SLOT_RECORD_LENGTH    23 /* for relative files */
#define SLOT_REPLACE_TRACK    28
#define SLOT_REPLACE_SECTOR   29
#define SLOT_NR_BLOCKS        30

#define SLOT_GEOS_ITRACK      21
#define SLOT_GEOS_ISECTOR     22
#define SLOT_GEOS_STRUCT      23
#define SLOT_GEOS_TYPE        24
#define SLOT_GEOS_YEAR        25 /* same on CMD (year to minute) */
#define SLOT_GEOS_MONTH       26
#define SLOT_GEOS_DATE        27
#define SLOT_GEOS_HOUR        28
#define SLOT_GEOS_MINUTE      29

#define SLOT_SIZE             32

#define PSLOT_TYPE            2
#define PSLOT_NAME            5

struct vdrive_s;
struct cbmdos_cmd_parse_plus_s;
struct bufferinfo_s;

typedef struct vdrive_dir_context_s {
    uint8_t buffer[256];      /* Current directory sector. */
    int find_length;       /* -1 allowed.  */
    uint8_t find_nslot[CBMDOS_SLOT_NAME_LENGTH];
    unsigned int find_type;
    unsigned int slot;
    unsigned int track;
    unsigned int sector;
    unsigned int time_low;
    unsigned int time_high;
    struct vdrive_s *vdrive;
} vdrive_dir_context_t;

void vdrive_dir_init(void);
int vdrive_dir_first_directory(struct vdrive_s *vdrive, struct cbmdos_cmd_parse_plus_s *cmd_parse, struct bufferinfo_s *p);
int vdrive_dir_next_directory(struct vdrive_s *vdrive, struct bufferinfo_s *b);
void vdrive_dir_find_first_slot(struct vdrive_s *vdrive, const uint8_t *name, int length, unsigned int type, vdrive_dir_context_t *dir);
uint8_t *vdrive_dir_find_next_slot(vdrive_dir_context_t *dir);
void vdrive_dir_no_a0_pads(uint8_t *ptr, int l);
int vdrive_dir_filetype(const uint8_t *name, int length);
void vdrive_dir_remove_slot(vdrive_dir_context_t *dir);
void vdrive_dir_create_slot(struct bufferinfo_s *p, uint8_t *realname, int reallength, int filetype);
void vdrive_dir_free_chain(struct vdrive_s *vdrive, int t, int s);
void vdrive_dir_updatetime(struct vdrive_s *vdrive, uint8_t *slot);
uint8_t *vdrive_dir_part_find_next_slot(vdrive_dir_context_t *dir);
int vdrive_dir_part_next_directory(struct vdrive_s *vdrive, struct bufferinfo_s *b);
int vdrive_dir_part_first_directory(struct vdrive_s *vdrive, const uint8_t *name, int length, struct bufferinfo_s *p);
void vdrive_dir_part_find_first_slot(struct vdrive_s *vdrive, const uint8_t *name, int length, unsigned int type, vdrive_dir_context_t *dir);

#endif
