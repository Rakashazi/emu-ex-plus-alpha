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
#include "video.h"
#include "viewport.h"

struct video_render_config_s;
struct video_canvas_s;

typedef void (*render_pal_ntsc_func_t)(video_render_config_t *, uint8_t *, uint8_t *,
                                  int, int, int, int,
                                  int, int, int, int,
                                  int,
                                  unsigned int, unsigned int);

typedef void (*render_rgbi_func_t)(video_render_config_t *, uint8_t *, uint8_t *,
                                  int, int, int, int,
                                  int, int, int, int,
                                  unsigned int, unsigned int);

typedef void (*render_crt_mono_func_t)(video_render_config_t *, uint8_t *, uint8_t *,
                                  int, int, int, int,
                                  int, int, int, int,
                                  unsigned int, unsigned int);

extern void video_render_main(struct video_render_config_s *config, uint8_t *src,
                              uint8_t *trg, int width, int height,
                              int xs, int ys, int xt, int yt,
                              int pitchs, int pitcht,
                              viewport_t *viewport);
extern void video_render_update_palette(struct video_canvas_s *canvas);

extern void video_render_palntscfunc_set(render_pal_ntsc_func_t func);
extern void video_render_crtmonofunc_set(render_crt_mono_func_t func);
extern void video_render_rgbifunc_set(render_rgbi_func_t func);

/* Default render functions */

extern void video_render_pal_ntsc_main(video_render_config_t *config,
                                  uint8_t *src, uint8_t *trg,
                                  int width, int height, int xs, int ys, int xt,
                                  int yt, int pitchs, int pitcht,
                                  int crt_type,
                                  unsigned int viewport_first_line, unsigned int viewport_last_line);

extern void video_render_rgbi_main(video_render_config_t *config,
                                  uint8_t *src, uint8_t *trg,
                                  int width, int height, int xs, int ys, int xt,
                                  int yt, int pitchs, int pitcht,
                                  unsigned int viewport_first_line, unsigned int viewport_last_line);

extern void video_render_crt_mono_main(video_render_config_t *config,
                                  uint8_t *src, uint8_t *trg,
                                  int width, int height, int xs, int ys, int xt,
                                  int yt, int pitchs, int pitcht,
                                  unsigned int viewport_first_line, unsigned int viewport_last_line);

#endif
