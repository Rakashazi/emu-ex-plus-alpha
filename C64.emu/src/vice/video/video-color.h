/*
 * video-color.h - Video implementation of YUV, YCbCr and RGB colors
 *
 * Written by
 *  John Selck <graham@cruise.de>
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

#ifndef VICE_VIDEO_COLOR_H
#define VICE_VIDEO_COLOR_H

#include "palette.h"
#include "types.h"

/* shared gamma table for renderers */
extern DWORD gamma_red[256 * 3];
extern DWORD gamma_grn[256 * 3];
extern DWORD gamma_blu[256 * 3];

/* shared gamma table for renderers */
extern DWORD gamma_red_fac[256 * 3 * 2];
extern DWORD gamma_grn_fac[256 * 3 * 2];
extern DWORD gamma_blu_fac[256 * 3 * 2];

/* optional alpha value for 32bit rendering */
extern DWORD alpha;

extern void video_color_palette_free(struct palette_s *palette);

#endif
