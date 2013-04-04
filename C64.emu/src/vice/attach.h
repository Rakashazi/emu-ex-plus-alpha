
/*
 * attach.h - File system attach management.
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

#ifndef VICE_ATTACH_H
#define VICE_ATTACH_H

#include "types.h"

#define ATTACH_DEVICE_NONE 0
#define ATTACH_DEVICE_FS   1 /* filesystem */
#define ATTACH_DEVICE_REAL 2 /* real IEC device (opencbm) */
#define ATTACH_DEVICE_RAW  3 /* raw device */
#define ATTACH_DEVICE_VIRT 4 /* non-tde drive/image */

struct vdrive_s;

extern void file_system_init(void);
extern void file_system_shutdown(void);
extern int file_system_resources_init(void);
extern int file_system_cmdline_options_init(void);

extern const char *file_system_get_disk_name(unsigned int unit);
extern int file_system_attach_disk(unsigned int unit, const char *filename);
extern void file_system_detach_disk(int unit);
extern void file_system_detach_disk_shutdown(void);
extern struct vdrive_s *file_system_get_vdrive(unsigned int unit);
extern int file_system_bam_get_disk_id(unsigned int unit, BYTE *id);
extern int file_system_bam_set_disk_id(unsigned int unit, BYTE *id);
extern void file_system_event_playback(unsigned int unit, const char *filename);

#endif
