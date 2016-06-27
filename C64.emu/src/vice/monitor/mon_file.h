/*
 * mon_file.h - The VICE built-in monitor file functions.
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

#ifndef VICE_MON_FILE_H
#define VICE_MON_FILE_H

#include "monitor.h"

extern void mon_file_load(const char *filename, int device, MON_ADDR start_addr,
                          bool is_bload);
extern void mon_file_save(const char *filename, int device, MON_ADDR start_addr,
                          MON_ADDR end_addr, bool is_bsave);

extern void mon_file_verify(const char *filename, int device,
                            MON_ADDR start_addr);

extern void mon_attach(const char *filename, int unit);
extern void mon_detach(int unit);

extern void mon_autostart(const char *image_name, int file_index, int run);

#endif
