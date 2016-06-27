/*
 * lamelib.h - Access lame lib.
 *
 * Written by
 *  Christian Vogelgsang <chris@vogelgsang.org>
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

#ifndef VICE_MP3LAME_H
#define VICE_MP3LAME_H

#include "vice.h"

#include "lame/lame.h"

#ifdef HAVE_STATIC_LAME

#define vice_lame_init                          lame_init
#define vice_lame_close                         lame_close
#define vice_lame_set_num_channels              lame_set_num_channels
#define vice_lame_set_in_samplerate             lame_set_in_samplerate
#define vice_lame_set_quality                   lame_set_quality
#define vice_lame_set_brate                     lame_set_brate
#define vice_lame_init_params                   lame_init_params
#define vice_lame_encode_buffer_interleaved     lame_encode_buffer_interleaved
#define vice_lame_encode_flush                  lame_encode_flush

#else

#define vice_lame_init                          (lamelib.p_lame_init)
#define vice_lame_close                         (lamelib.p_lame_close)
#define vice_lame_set_num_channels              (lamelib.p_lame_set_num_channels)
#define vice_lame_set_in_samplerate             (lamelib.p_lame_set_in_samplerate)
#define vice_lame_set_quality                   (lamelib.p_lame_set_quality)
#define vice_lame_set_brate                     (lamelib.p_lame_set_brate)
#define vice_lame_init_params                   (lamelib.p_lame_init_params)
#define vice_lame_encode_buffer_interleaved     (lamelib.p_lame_encode_buffer_interleaved)
#define vice_lame_encode_flush                  (lamelib.p_lame_encode_flush)

/* define function pointers of Lame API */
typedef lame_global_flags * CDECL (*lame_init_t)(void);
typedef int CDECL (*lame_close_t)(lame_global_flags *);
typedef int CDECL (*lame_set_num_channels_t)(lame_global_flags *, int);
typedef int CDECL (*lame_set_in_samplerate_t)(lame_global_flags *, int);
typedef int CDECL (*lame_set_quality_t)(lame_global_flags *, int);
typedef int CDECL (*lame_set_brate_t)(lame_global_flags *, int);
typedef int CDECL (*lame_init_params_t)(lame_global_flags *);
typedef int CDECL (*lame_encode_buffer_interleaved_t)(lame_global_flags*, short int [], int, unsigned char*, int);
typedef int CDECL (*lame_encode_flush_t)(lame_global_flags *  gfp, unsigned char*, int);

struct lamelib_s {
    lame_init_t                             p_lame_init;
    lame_close_t                            p_lame_close;
    lame_set_num_channels_t                 p_lame_set_num_channels;
    lame_set_in_samplerate_t                p_lame_set_in_samplerate;
    lame_set_quality_t                      p_lame_set_quality;
    lame_set_brate_t                        p_lame_set_brate;
    lame_init_params_t                      p_lame_init_params;
    lame_encode_buffer_interleaved_t        p_lame_encode_buffer_interleaved;
    lame_encode_flush_t                     p_lame_encode_flush;
};

typedef struct lamelib_s lamelib_t;

extern lamelib_t lamelib;
extern int lamelib_open(void);
extern void lamelib_close(void);

#endif
#endif
