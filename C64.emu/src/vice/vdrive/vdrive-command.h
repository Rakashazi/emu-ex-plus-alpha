/*
 * vdrive-command.h - Virtual disk-drive implementation. Command interpreter.
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

#ifndef VICE_VDRIVE_COMMAND_H
#define VICE_VDRIVE_COMMAND_H

#include "types.h"

struct vdrive_s;

extern void vdrive_command_init(void);
extern int vdrive_command_execute(struct vdrive_s *vdrive, const BYTE *buf, unsigned int length);
extern int vdrive_command_format(struct vdrive_s *vdrive, const char *disk_name);
extern int vdrive_command_validate(struct vdrive_s *vdrive);
extern void vdrive_command_set_error(struct vdrive_s *vdrive, int code, unsigned int track, unsigned int sector);
extern int vdrive_command_memory_read(struct vdrive_s *vdrive, const BYTE *buf, WORD addr, unsigned int length);
extern int vdrive_command_memory_write(struct vdrive_s *vdrive, const BYTE *buf, WORD addr, unsigned int length);
extern int vdrive_command_memory_exec(struct vdrive_s *vdrive, const BYTE *buf, WORD addr, unsigned int length);

#endif
