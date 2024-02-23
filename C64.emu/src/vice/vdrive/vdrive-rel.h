/*
 * vdrive-rel.h - Virtual disk-drive implementation.
 *                Relative files specific functions.
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

#ifndef VICE_VDRIVE_REL_H
#define VICE_VDRIVE_REL_H

#include "types.h"

struct vdrive_s;

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

void vdrive_rel_init(void);
int vdrive_rel_open(struct vdrive_s *vdrive, unsigned int secondary, cbmdos_cmd_parse_plus_t *cmd_parse);
int vdrive_rel_position(struct vdrive_s *vdrive, unsigned int channel, unsigned int rec_lo, unsigned int rec_hi, unsigned int position);
int vdrive_rel_read(struct vdrive_s *vdrive, uint8_t *data, unsigned int secondary);
int vdrive_rel_write(struct vdrive_s *vdrive, uint8_t data, unsigned int secondary);
int vdrive_rel_close(struct vdrive_s *vdrive, unsigned int secondary);
void vdrive_rel_listen(struct vdrive_s *vdrive, unsigned int secondary);
void vdrive_rel_scratch(struct vdrive_s *vdrive, unsigned int t, unsigned int s);
int vdrive_rel_setup_ss_buffers(struct vdrive_s *vdrive, unsigned int secondary);
void vdrive_rel_shutdown_ss_buffers(struct vdrive_s *vdrive, unsigned int secondary);

#endif
