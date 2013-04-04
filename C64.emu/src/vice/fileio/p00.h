/*
 * p00.h - Utility functions for P00 file support.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
 * Based on older code by
 *  Dan Fandrich         <dan@fch.wimsey.bc.ca>
 *  Jouko Valta          <jopi@stekt.oulu.fi>
 *  pottendo <pottendo@gmx.net>
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

#ifndef P00_H
#define P00_H

#include <stdio.h>

#include "types.h"

struct fileio_info_s;

extern struct fileio_info_s *p00_open(const char *file_name, const char *path,
                                      unsigned int command, unsigned int type);
extern void p00_close(struct fileio_info_s *info);
extern unsigned int p00_read(struct fileio_info_s *info, BYTE *buf,
                             unsigned int len);
extern unsigned int p00_write(struct fileio_info_s *info, BYTE *buf,
                              unsigned int len);
extern unsigned int p00_ferror(struct fileio_info_s *info);
extern unsigned int p00_rename(const char *src_name, const char *dest_name,
                               const char *path);
extern unsigned int p00_scratch(const char *file_name, const char *path);
extern unsigned int p00_get_bytes_left(struct fileio_info_s *info);

#endif
