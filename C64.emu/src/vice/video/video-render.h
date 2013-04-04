/*
 * video-render.h - Implementation of framebuffer to physical screen copy
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

#ifndef VICE_VIDEORENDER_H
#define VICE_VIDEORENDER_H

#include "types.h"
#include "viewport.h"

struct video_render_config_s;
struct video_canvas_s;

extern void video_render_main(struct video_render_config_s *config, BYTE *src,
                              BYTE *trg, int width, int height,
                              int xs, int ys, int xt, int yt,
                              int pitchs, int pitcht, int depth,
                              viewport_t *viewport);
extern void video_render_update_palette(struct video_canvas_s *canvas);

extern void video_render_1x2func_set(void (*func)(struct video_render_config_s *,
                                                  const BYTE *, BYTE *,
                                                  unsigned int, const unsigned int,
                                                  const unsigned int, const unsigned int,
                                                  const unsigned int, const unsigned int,
                                                  const unsigned int, const unsigned int,
                                                  int));

extern void video_render_2x2func_set(void (*func)(struct video_render_config_s *,
                                                  const BYTE *, BYTE *,
                                                  unsigned int, const unsigned int,
                                                  const unsigned int, const unsigned int,
                                                  const unsigned int, const unsigned int,
                                                  const unsigned int, const unsigned int,
                                                  int));

extern void video_render_palfunc_set(void (*func)(struct video_render_config_s *,
                                                  BYTE *, BYTE *, int, int, int, int,
                                                  int, int, int, int, int, viewport_t *));

extern void video_render_crtfunc_set(void (*func)(struct video_render_config_s *,
                                                  BYTE *, BYTE *, int, int, int, int,
                                                  int, int, int, int, int, viewport_t *));

#endif
