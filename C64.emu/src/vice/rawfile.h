/*
 * rawfile.h - Raw file handling.
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

#ifndef VICE_RAWFILE_H
#define VICE_RAWFILE_H

#include "types.h"

struct rawfile_info_s;

extern struct rawfile_info_s *rawfile_open(const char *file_name, const char *path, unsigned int command);
extern void rawfile_destroy(struct rawfile_info_s *info);
extern unsigned int rawfile_write(struct rawfile_info_s *info, BYTE *buf, unsigned int len);
extern unsigned int rawfile_read(struct rawfile_info_s *info, BYTE *buf, unsigned int len);
extern int rawfile_seek_set(struct rawfile_info_s *info, int offset);
extern unsigned int rawfile_ferror(struct rawfile_info_s *info);
extern unsigned int rawfile_rename(const char *src_name, const char *dst_name, const char *path);
extern unsigned int rawfile_remove(const char *src_name, const char *path);
extern unsigned int rawfile_get_bytes_left(struct rawfile_info_s *info);

#endif
