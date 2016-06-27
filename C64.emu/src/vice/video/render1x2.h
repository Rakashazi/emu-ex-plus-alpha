/*
 * render1x2.h - Implementation of framebuffer to physical screen copy
 *
 * Written by
 *  John Selck <graham@cruise.de>
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

#ifndef VICE_RENDER1X2_H
#define VICE_RENDER1X2_H

#include "types.h"

#include "video.h"

extern void render_08_1x2_04(const video_render_color_tables_t *color_tab, const BYTE *src, BYTE *trg,
                             unsigned int width, const unsigned int height,
                             const unsigned int xs, const unsigned int ys,
                             const unsigned int xt, const unsigned int yt,
                             const unsigned int pitchs,
                             const unsigned int pitcht,
                             const unsigned int doublescan,
                             video_render_config_t *config);
extern void render_16_1x2_04(const video_render_color_tables_t *color_tab, const BYTE *src, BYTE *trg,
                             unsigned int width, const unsigned int height,
                             const unsigned int xs, const unsigned int ys,
                             const unsigned int xt, const unsigned int yt,
                             const unsigned int pitchs,
                             const unsigned int pitcht,
                             const unsigned int doublescan,
                             video_render_config_t *config);
extern void render_24_1x2_04(const video_render_color_tables_t *color_tab, const BYTE *src, BYTE *trg,
                             unsigned int width, const unsigned int height,
                             const unsigned int xs, const unsigned int ys,
                             const unsigned int xt, const unsigned int yt,
                             const unsigned int pitchs,
                             const unsigned int pitcht,
                             const unsigned int doublescan,
                             video_render_config_t *config);
extern void render_32_1x2_04(const video_render_color_tables_t *color_tab, const BYTE *src, BYTE *trg,
                             unsigned int width, const unsigned int height,
                             const unsigned int xs, const unsigned int ys,
                             const unsigned int xt, const unsigned int yt,
                             const unsigned int pitchs,
                             const unsigned int pitcht,
                             const unsigned int doublescan,
                             video_render_config_t *config);
#endif
