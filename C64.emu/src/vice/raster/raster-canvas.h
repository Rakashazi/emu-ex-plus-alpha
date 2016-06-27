/*
 * raster-canvas.h - Raster-based video chip emulation helper.
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

#ifndef VICE_RASTER_CANVAS_H
#define VICE_RASTER_CANVAS_H

struct raster_s;

/* A simple convenience type for defining a rectangular area on the screen.  */
struct raster_canvas_area_s {
    unsigned int xs;
    unsigned int ys;
    unsigned int xe;
    unsigned int ye;
    int is_null;
};
typedef struct raster_canvas_area_s raster_canvas_area_t;

extern void raster_canvas_init(struct raster_s *raster);
extern void raster_canvas_shutdown(struct raster_s *raster);

extern void raster_canvas_handle_end_of_frame(struct raster_s *raster);
extern void raster_canvas_update_all(struct raster_s *raster);

#endif
