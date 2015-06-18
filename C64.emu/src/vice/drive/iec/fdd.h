/*
 * fdd.h - (M)FM floppy disk drive emulation
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
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

#ifndef VICE_FDD_H
#define VICE_FDD_H

#include "types.h"

struct disk_image_s;
struct drive_s;
struct snapshot_s;
typedef struct fd_drive_s fd_drive_t;

extern const int fdd_data_rates[4];
extern fd_drive_t *fdd_init(int num, struct drive_s *drive);
extern void fdd_shutdown(fd_drive_t *drv);

extern void fdd_image_attach(fd_drive_t *drv, struct disk_image_s *image);
extern void fdd_image_detach(fd_drive_t *drv);
extern WORD fdd_read(fd_drive_t *drv);
extern int fdd_write(fd_drive_t *drv, WORD data);
extern void fdd_flush(fd_drive_t *drv);
extern void fdd_seek_pulse(fd_drive_t *drv, int dir);
extern void fdd_select_head(fd_drive_t *drv, int head);
extern void fdd_set_motor(fd_drive_t *drv, int motor);
extern void fdd_set_rate(fd_drive_t *drv, int rate);
extern int fdd_rotate(fd_drive_t *drv, int bytes);
extern inline int fdd_index(fd_drive_t *drv);
extern inline void fdd_index_count_reset(fd_drive_t *drv);
extern inline int fdd_index_count(fd_drive_t *drv);
extern inline int fdd_track0(fd_drive_t *drv);
extern inline int fdd_write_protect(fd_drive_t *drv);
extern inline int fdd_disk_change(fd_drive_t *drv);
extern inline WORD fdd_crc(WORD crc, BYTE b);

extern int fdd_snapshot_write_module(fd_drive_t *drv, struct snapshot_s *s);
extern int fdd_snapshot_read_module(fd_drive_t *drv, struct snapshot_s *s);
#endif
