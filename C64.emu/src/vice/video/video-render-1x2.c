/*
 * video-render-1x2.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include "vice.h"

#include "render1x2.h"
#include "types.h"
#include "video-render.h"
#include "video.h"


static void video_render_1x2_main(video_render_config_t *config,
                                  const BYTE *src, BYTE *trg,
                                  unsigned int width, const unsigned int height,
                                  const unsigned int xs, const unsigned int ys,
                                  const unsigned int xt, const unsigned int yt,
                                  const unsigned int pitchs,
                                  const unsigned int pitcht,
                                  int depth)
{
    const video_render_color_tables_t *colortab;
    int doublescan;

    doublescan = config->doublescan;
    colortab = &config->color_tables;

    switch (depth) {
        case 8:
            render_08_1x2_04(colortab, src, trg, width, height,
                             xs, ys, xt, yt, pitchs, pitcht, doublescan, config);
            return;
        case 16:
            render_16_1x2_04(colortab, src, trg, width, height,
                             xs, ys, xt, yt, pitchs, pitcht, doublescan, config);
            return;
        case 24:
            render_24_1x2_04(colortab, src, trg, width, height,
                             xs, ys, xt, yt, pitchs, pitcht, doublescan, config);
            return;
        case 32:
            render_32_1x2_04(colortab, src, trg, width, height,
                             xs, ys, xt, yt, pitchs, pitcht, doublescan, config);
            return;
    }
}

void video_render_1x2_init(void)
{
    video_render_1x2func_set(video_render_1x2_main);
}
