/*
 * video-viewport.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 * Screen centering code rewrite
 *  Kajtar Zsolt <soci@c64.rulez.org>
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

#include "lib.h"
#include "machine.h"
#include "video.h"
#include "videoarch.h"
#include "viewport.h"

void video_viewport_get(video_canvas_t *canvas, viewport_t **viewport,
                        geometry_t **geometry)
{
    *viewport = canvas->viewport;
    *geometry = canvas->geometry;
    return;
}

void video_viewport_resize(video_canvas_t *canvas, char resize_canvas)
{
    geometry_t *geometry;
    viewport_t *viewport;     /* this will be displayed at the end */
    rectangle_t *screen_size; /* emulated screen with border */
    rectangle_t *gfx_size;
    position_t *gfx_position; /* top left corner of non border screen area */
    unsigned width, height;
    int first_x;
    int x_offset;
    int real_gfx_height;
    int first_line;
    int displayed_height;
    int y_offset;
    int small_x_border, small_y_border;

    if (canvas->initialized == 0) {
        return;
    }

    geometry = canvas->geometry;
    viewport = canvas->viewport;

    screen_size = &geometry->screen_size;
    gfx_size = &geometry->gfx_size;
    gfx_position = &geometry->gfx_position;

    if (resize_canvas && video_canvas_can_resize(canvas)) {
        /* The emulated screen has changed:
           the size of the emulator's screen changes in order to adapt to it */
        canvas->draw_buffer->canvas_width = canvas->draw_buffer->visible_width;
        canvas->draw_buffer->canvas_height = canvas->draw_buffer->visible_height;
        canvas->draw_buffer->canvas_physical_width = canvas->draw_buffer->canvas_width * (canvas->videoconfig->doublesizex + 1);
        canvas->draw_buffer->canvas_physical_height = canvas->draw_buffer->canvas_height * (canvas->videoconfig->doublesizey + 1);
    } else {
        /* The emulator's screen has been resized,
           or he emulated screen has changed but the emulator's screen is unable to adapt:
           in any case, the size of the emulator screen won't change now */
        canvas->draw_buffer->canvas_width = canvas->draw_buffer->canvas_physical_width / (canvas->videoconfig->doublesizex + 1);
        canvas->draw_buffer->canvas_height = canvas->draw_buffer->canvas_physical_height / (canvas->videoconfig->doublesizey + 1);
    }
    width = canvas->draw_buffer->canvas_width;
    height = canvas->draw_buffer->canvas_height;

    /* Horizontal alignment strategy (from small to big):
     * 1. cut off left border, cut as much as needed from the right side of gfx
     *    except for moving area chips, there cut of equally from left/right side
     * 2. gfx fits, try to make the border area symmetric
     * 3. reveal the asymmetric border part, without black bands
     * 4. the complete screen fits, try to do equal black banding for the remaining space */

    /* smallest of left/right border */
    small_x_border = (int)screen_size->width - (int)gfx_position->x - (int)gfx_size->width;
    if (small_x_border > (int)gfx_position->x) {
        small_x_border = (int)gfx_position->x;
    }
    /* Easy part, just put it in center with symmetric borders */
    if ((int)gfx_size->width + small_x_border * 2 > (int)width) {
        first_x = (int)gfx_position->x - ((int)width - (int)gfx_size->width) / 2;
    } else { /* With a lot of space it's possible reveal the extra non-symmetric border part */
        if ((int)gfx_position->x > small_x_border) { /* left border bigger */
            first_x = (int)screen_size->width - (int)width;
        } else { /* right border bigger */
            first_x = 0;
        }
    }
    x_offset = ((int)width - (int)screen_size->width) / 2;

    if (x_offset < 0) {
        x_offset = 0;
    }
    if (first_x < 0) {
        first_x = 0;
    }
    /* Stop at the border, unless gfx_area_moves, then cut off even more */
    if (!geometry->gfx_area_moves && first_x > (int)gfx_position->x) {
        first_x = (int)gfx_position->x;
    }
    viewport->first_x = first_x;
    viewport->x_offset = x_offset;

    /* Vertical alignment. Also easy, just put the visible area in center. */
    real_gfx_height = geometry->last_displayed_line - geometry->first_displayed_line + 1;

    small_y_border = (int)geometry->last_displayed_line - (int)gfx_position->y - (int)gfx_size->height + 1;
    if (small_y_border > (int)gfx_position->y - (int)geometry->first_displayed_line) {
        small_y_border = (int)gfx_position->y - (int)geometry->first_displayed_line;
    }

    /* Easy part, just put it in center with symmetric borders */
    if ((int)gfx_size->height + small_y_border * 2 > (int)height) {
        first_line = (int)gfx_position->y - ((int)height - (int)gfx_size->height) / 2;
    } else { /* With a lot of space it's possible reveal the extra non-symmetric border part */
        if ((int)gfx_position->y - (int)geometry->first_displayed_line > small_y_border) { /* top border bigger */
            first_line = real_gfx_height - (int)height + (int)geometry->first_displayed_line;
        } else { /* bottom border bigger */
            first_line = (int)geometry->first_displayed_line;
        }
    }
    y_offset = ((int)height - real_gfx_height) / 2;

    if (y_offset < 0) {
        y_offset = 0;
    }
    if (first_line < (int)geometry->first_displayed_line) {
        first_line = (int)geometry->first_displayed_line;
    }
    /* Stop at the border, unless gfx_area_moves, then cut off even more */
    if (!geometry->gfx_area_moves && first_line > (int)gfx_position->y) {
        first_line = (int)gfx_position->y;
    }
    displayed_height = (real_gfx_height > (int)height) ? height : real_gfx_height;
    viewport->first_line = (unsigned int)first_line;
    viewport->y_offset = (unsigned int)y_offset;
    viewport->last_line = viewport->first_line + (unsigned int)displayed_height - 1;

    if (!video_disabled_mode) {
        video_canvas_resize(canvas, (char)(resize_canvas && video_canvas_can_resize(canvas)));
    }

    video_canvas_refresh_all(canvas);
}

void video_viewport_title_set(video_canvas_t *canvas, const char *title)
{
    viewport_t *viewport;

    viewport = canvas->viewport;

    lib_free(viewport->title);
    viewport->title = lib_stralloc(title);
}

void video_viewport_title_free(viewport_t *viewport)
{
    lib_free(viewport->title);
}
