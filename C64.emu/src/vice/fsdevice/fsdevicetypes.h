/*
 * fsdevicetypes.h - File system device.
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

#ifndef VICE_FSDEVICETYPES_H
#define VICE_FSDEVICETYPES_H

#include "types.h"

#define FSDEVICE_BUFFER_MAX 16
#define FSDEVICE_DEVICE_MAX 4

#define FSDEVICE_TRACK_MAX   80
#define FSDEVICE_SECTOR_MAX  32

enum fsmode {
    Write, Read, Append, Directory
};

struct fileio_info_s;
struct ioutil_dir_s;
struct tape_image_s;

struct bufinfo_s {
    struct fileio_info_s *fileio_info;
    struct ioutil_dir_s *ioutil_dir;
    struct tape_image_s *tape;
    enum fsmode mode;
    char *dir;
    BYTE *name;
    int buflen;
    BYTE *bufp;
    int eof;
    int reclen;
    int type;
    BYTE buffered;  /* Buffered Byte: Added to buffer reads to remove buffering from iec code */
    int isbuffered; /* TRUE is a byte exists in the buffer above */
    int iseof;      /* TRUE if an EOF is detected on a buffered read */
    char *dirmask;
};
typedef struct bufinfo_s bufinfo_t;

struct fsdevice_dev_s {
    unsigned int eptr;
    unsigned int elen;
    char *errorl;
    unsigned int cptr;
    BYTE *cmdbuf;
    bufinfo_t bufinfo[FSDEVICE_BUFFER_MAX];
    int track, sector; /* fake track/sector pointer */
    BYTE bam[(FSDEVICE_TRACK_MAX * FSDEVICE_SECTOR_MAX) >> 3]; /* fake bam */
};
typedef struct fsdevice_dev_s fsdevice_dev_t;

extern fsdevice_dev_t fsdevice_dev[FSDEVICE_DEVICE_MAX];

struct vdrive_s;

extern void fsdevice_error(struct vdrive_s *vdrive, int code);
extern char *fsdevice_get_path(unsigned int unit);
extern int fsdevice_error_get_byte(struct vdrive_s *vdrive, BYTE *data);
extern int fsdevice_flush_write_byte(struct vdrive_s *vdrive, BYTE data);

#endif
