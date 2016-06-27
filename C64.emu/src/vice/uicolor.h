/*
 * uicolor.h - X11 color routines.
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

#ifndef VICE_UICOLOR_H
#define VICE_UICOLOR_H

#include "types.h"

struct video_canvas_s;
struct palette_s;

extern int uicolor_alloc_color(unsigned int red, unsigned int green,
                               unsigned int blue, unsigned long *color_pixel,
                               BYTE *pixel_return);
extern void uicolor_free_color(unsigned int red, unsigned int green,
                               unsigned int blue, unsigned long color_pixel);
extern void uicolor_convert_color_table(unsigned int colnr, BYTE *data,
                                        long color_pixel, void *c);
extern int uicolor_set_palette(struct video_canvas_s *c,
                               const struct palette_s *palette);

/* Temporary! */
extern int uicolor_alloc_colors(struct video_canvas_s *c);

#endif
