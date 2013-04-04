/*
 * vdrive-internal.h - Virtual disk-drive implementation.
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

#ifndef VICE_VDRIVE_INTERNAL_H
#define VICE_VDRIVE_INTERNAL_H

struct vdrive_s;

extern void vdrive_internal_init(void);

extern struct vdrive_s *vdrive_internal_open_fsimage(const char *name, unsigned int read_only);
extern int vdrive_internal_close_disk_image(struct vdrive_s *vdrive);
extern int vdrive_internal_create_format_disk_image(const char *filename, const char *diskname, unsigned int type);

#endif
