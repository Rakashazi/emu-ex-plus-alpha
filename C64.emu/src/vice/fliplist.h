/*
 * fliplist.h
 *
 * Written by
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

#ifndef VICE_FLIPLIST_H
#define VICE_FLIPLIST_H

#define FLIP_NEXT 1
#define FLIP_PREV 0

typedef struct fliplist_s * fliplist_t;

extern int fliplist_resources_init(void);
extern void fliplist_resources_shutdown(void);
extern int fliplist_cmdline_options_init(void);

extern void fliplist_shutdown(void);
extern void fliplist_set_current(unsigned int unit, const char *image);
extern void fliplist_add_image(unsigned int unit);
extern void fliplist_remove(unsigned int unit, const char *image);
extern void fliplist_attach_head(unsigned int unit, int direction);
extern fliplist_t fliplist_init_iterate(unsigned int unit);
extern fliplist_t fliplist_next_iterate(unsigned int unit);
/*extern char *fliplist_get_head(unsigned int unit);*/
extern const char *fliplist_get_next(unsigned int unit);
extern const char *fliplist_get_prev(unsigned int unit);
extern const char *fliplist_get_image(fliplist_t fl);
extern unsigned int fliplist_get_unit(fliplist_t fl);

/* FIXME: once all UIs are updated to use FLIPLIST_ALL_UNITS this should be
          changed to a positive value and the cast removed */
#define FLIPLIST_ALL_UNITS       ((unsigned int)-1)

extern void fliplist_clear_list(unsigned int unit);
extern int fliplist_save_list(unsigned int unit, const char *filename);
extern int fliplist_load_list(unsigned int unit, const char *filename, int autoattach);

#endif
