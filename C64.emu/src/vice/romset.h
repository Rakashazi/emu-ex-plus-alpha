/*
 * romset.h - romset file handling
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_ROMSET_H
#define VICE_ROMSET_H

extern void romset_init(void);
extern int romset_resources_init(void);
extern void romset_resources_shutdown(void);
extern int romset_cmdline_options_init(void);

extern int romset_file_load(const char *filename);
extern int romset_file_save(const char *filename, const char **resource_list);
extern char *romset_file_list(const char **resource_list);

extern int romset_archive_load(const char *filename, int autostart);
extern int romset_archive_save(const char *filename);
extern char *romset_archive_list(void);
extern int romset_archive_item_save(const char *filename, const char *romset_name);
extern int romset_archive_item_select(const char *romset_name);
extern int romset_archive_item_create(const char *romset_name, const char **resource_list);
extern int romset_archive_item_delete(const char *romset_name);
extern void romset_archive_clear(void);
extern int romset_archive_get_number(void);
extern char *romset_archive_get_item(int number);

#endif
