/*
 * render2x2crt.h - Implementation of framebuffer to physical screen copy
 *
 * Written by
 *  groepaz <groepaz@gmx.net> based on the pal renderers
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

#ifndef VICE_RENDER2X2CRT_H
#define VICE_RENDER2X2CRT_H

#include "types.h"
#include "viewport.h"

extern void render_UYVY_2x2_crt(video_render_color_tables_t *colortab,
                                const BYTE *src, BYTE *trg,
                                unsigned int width, const unsigned int height,
                                const unsigned int xs, const unsigned int ys,
                                const unsigned int xt, const unsigned int yt,
                                const unsigned int pitchs, const unsigned int pitcht,
                                viewport_t *viewport, video_render_config_t *config);

extern void render_YUY2_2x2_crt(video_render_color_tables_t *colortab,
                                const BYTE *src, BYTE *trg,
                                unsigned int width, const unsigned int height,
                                const unsigned int xs, const unsigned int ys,
                                const unsigned int xt, const unsigned int yt,
                                const unsigned int pitchs, const unsigned int pitcht,
                                viewport_t *viewport, video_render_config_t *config);

extern void render_YVYU_2x2_crt(video_render_color_tables_t *colortab,
                                const BYTE *src, BYTE *trg,
                                unsigned int width, const unsigned int height,
                                const unsigned int xs, const unsigned int ys,
                                const unsigned int xt, const unsigned int yt,
                                const unsigned int pitchs, const unsigned int pitcht,
                                viewport_t *viewport, video_render_config_t *config);


extern void render_16_2x2_crt(video_render_color_tables_t *colortab,
                              const BYTE *src, BYTE *trg,
                              unsigned int width, const unsigned int height,
                              const unsigned int xs, const unsigned int ys,
                              const unsigned int xt, const unsigned int yt,
                              const unsigned int pitchs, const unsigned int pitcht,
                              viewport_t *viewport, video_render_config_t *config);

extern void render_24_2x2_crt(video_render_color_tables_t *colortab,
                              const BYTE *src, BYTE *trg,
                              unsigned int width, const unsigned int height,
                              const unsigned int xs, const unsigned int ys,
                              const unsigned int xt, const unsigned int yt,
                              const unsigned int pitchs,
                              const unsigned int pitcht,
                              viewport_t *viewport, video_render_config_t *config);

extern void render_32_2x2_crt(video_render_color_tables_t *colortab,
                              const BYTE *src, BYTE *trg,
                              unsigned int width, const unsigned int height,
                              const unsigned int xs, const unsigned int ys,
                              const unsigned int xt, const unsigned int yt,
                              const unsigned int pitchs,
                              const unsigned int pitcht,
                              viewport_t *viewport, video_render_config_t *config);
#endif
