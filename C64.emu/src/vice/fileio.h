/*
 * fileio.h - File IO handling.
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

#ifndef VICE_FILEIO_H
#define VICE_FILEIO_H

#include "types.h"

#define FILEIO_COMMAND_READ        0
#define FILEIO_COMMAND_WRITE       1
#define FILEIO_COMMAND_APPEND      2
#define FILEIO_COMMAND_APPEND_READ 3
#define FILEIO_COMMAND_STAT        4  /* works with directories */
#define FILEIO_COMMAND_MASK        15
#define FILEIO_COMMAND_FSNAME      16

#define FILEIO_FORMAT_RAW (1 << 0)
#define FILEIO_FORMAT_P00 (1 << 1)

#define FILEIO_TYPE_DEL 0
#define FILEIO_TYPE_SEQ 1
#define FILEIO_TYPE_PRG 2
#define FILEIO_TYPE_USR 3
#define FILEIO_TYPE_REL 4
#define FILEIO_TYPE_CBM 5
#define FILEIO_TYPE_DIR 6
#define FILEIO_TYPE_ANY 255

#define FILEIO_FILE_OK         0
#define FILEIO_FILE_NOT_FOUND  1
#define FILEIO_FILE_EXISTS     2
#define FILEIO_FILE_PERMISSION 3
#define FILEIO_FILE_SCRATCHED  4

struct rawfile_info_s;

struct fileio_info_s {
    BYTE *name;
    unsigned int length;
    unsigned int type;
    unsigned int format;
    struct rawfile_info_s *rawfile;
};
typedef struct fileio_info_s fileio_info_t;

extern fileio_info_t *fileio_open(const char *file_name, const char *path,
                                  unsigned int format, unsigned int command,
                                  unsigned int type);
extern void fileio_close(fileio_info_t *info);
extern unsigned int fileio_read(fileio_info_t *info, BYTE *buf, unsigned int len);
extern unsigned int fileio_write(fileio_info_t *info, BYTE *buf, unsigned int len);
extern unsigned int fileio_ferror(fileio_info_t *info);
extern unsigned int fileio_rename(const char *src_name, const char *dest_name,
                                  const char *path, unsigned int format);
extern unsigned int fileio_scratch(const char *file_name, const char *path,
                                   unsigned int format);
extern unsigned int fileio_get_bytes_left(fileio_info_t *info);

#endif
