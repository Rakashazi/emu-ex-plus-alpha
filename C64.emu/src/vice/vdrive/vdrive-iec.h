/*
 * vdrive-iec.h - Virtual disk-drive IEC implementation.
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

#ifndef VICE_VDRIVEIEC_H
#define VICE_VDRIVEIEC_H

#include "types.h"

struct bufferinfo_s;
struct cbmdos_cmd_parse_s;
struct vdrive_s;

extern void vdrive_iec_init(void);

/* Generic IEC interface.  */
extern int vdrive_iec_open(struct vdrive_s *vdrive, const BYTE *name, unsigned int length, unsigned int secondary, struct cbmdos_cmd_parse_s *cmd_parse_ext);
extern int vdrive_iec_close(struct vdrive_s *vdrive, unsigned int secondary);
extern int vdrive_iec_read(struct vdrive_s *vdrive, BYTE *data, unsigned int secondary);
extern int vdrive_iec_write(struct vdrive_s *vdrive, BYTE data, unsigned int secondary);
extern void vdrive_iec_flush(struct vdrive_s *vdrive, unsigned int secondary);

extern int vdrive_iec_attach(unsigned int unit, const char *name);

extern void vdrive_iec_listen(struct vdrive_s *vdrive, unsigned int secondary);

extern int vdrive_iec_update_dirent(struct vdrive_s *vdrive, unsigned int channel);

#endif
