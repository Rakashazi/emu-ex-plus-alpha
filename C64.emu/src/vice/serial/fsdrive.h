/*
 * fsdrive.h - Filesystem based serial emulation.
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

#ifndef VICE_FSDRIVE_H
#define VICE_FSDRIVE_H

#include "types.h"

extern void fsdrive_init(void);
extern void fsdrive_reset(void);
extern void fsdrive_open(unsigned int device, BYTE secondary, void (*st_func)(BYTE));
extern void fsdrive_close(unsigned int device, BYTE secondary, void (*st_func)(BYTE));
extern void fsdrive_listentalk(unsigned int device, BYTE secondary, void (*st_func)(BYTE));
extern void fsdrive_unlisten(unsigned int device, BYTE secondary, void (*st_func)(BYTE));
extern void fsdrive_untalk(unsigned int device, BYTE secondary, void (*st_func)(BYTE));
extern void fsdrive_write(unsigned int device, BYTE secondary, BYTE data, void (*st_func)(BYTE));
extern BYTE fsdrive_read(unsigned int device, BYTE secondary, void (*st_func)(BYTE));

#endif
