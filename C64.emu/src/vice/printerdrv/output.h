/*
 * output.h - Output driver.
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

#ifndef VICE_OUTPUT_H
#define VICE_OUTPUT_H

#define OUTPUT_PIXEL_BLACK '*'
#define OUTPUT_PIXEL_WHITE ' '
#define OUTPUT_PIXEL_RED   'R'
#define OUTPUT_PIXEL_GREEN 'G'
#define OUTPUT_PIXEL_BLUE  'B'
#define OUTPUT_NEWLINE '\n'

struct palette_s;

struct output_parameter_s {
    unsigned int maxcol;
    unsigned int maxrow;
    unsigned int dpi_x;
    unsigned int dpi_y;
    struct palette_s *palette;
};
typedef struct output_parameter_s output_parameter_t;

#endif
