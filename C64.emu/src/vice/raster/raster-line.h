/*
 * raster-line.h - Raster-based video chip emulation helper.
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

#ifndef VICE_RASTER_LINE_H
#define VICE_RASTER_LINE_H

struct raster_s;

extern unsigned int raster_line_get_real_mode(struct raster_s *raster);
extern void raster_line_draw_borders(struct raster_s *raster);
extern void raster_line_fill_xsmooth_region(struct raster_s *raster);
extern void raster_line_draw_blank(struct raster_s *raster, unsigned int start,
                                   unsigned int end);
extern void raster_line_emulate(struct raster_s *raster);

#endif
