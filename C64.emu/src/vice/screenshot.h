/*
 * screenshot.h - Create a screenshot.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_SCREENSHOT_H
#define VICE_SCREENSHOT_H

#include <stdio.h>

#include "types.h"

struct palette_s;
struct video_canvas_s;
struct gfxoutputdrv_data_s;

typedef struct screenshot_s {
    struct video_canvas_s *canvas;

    struct palette_s *palette;
    BYTE *color_map;
    BYTE *draw_buffer;
    unsigned int draw_buffer_line_size;

    /* Screen size we actually want to save.  */
    unsigned int width;
    unsigned int height;

    /* Overall screen size.  */
    unsigned int max_width;
    unsigned int max_height;

    /* First and last displayed line.  */
    unsigned int first_displayed_line;
    unsigned int last_displayed_line;

    /* Offset to the overall screen.  */
    unsigned int x_offset;
    unsigned int y_offset;

    /* Pixel size.  */
    unsigned int size_width;
    unsigned int size_height;

    /* DPI */
    unsigned int dpi_x;
    unsigned int dpi_y;

    /* Upper left corner of viewport.  */
    unsigned int first_displayed_col;

    /* Line data convert function.  */
    void (*convert_line)(struct screenshot_s *screenshot, BYTE *data,
                         unsigned int line, unsigned int mode);

    /* Pointer for graphics outout driver internal data.  */
    struct gfxoutputdrv_data_s *gfxoutputdrv_data;

    /* native screenshot chip ID */
    char *chipid;

    /* native screenshot video regs */
    BYTE *video_regs;

    /* native screenshot screen */
    BYTE *screen_ptr;

    /* native screenshot chargen */
    BYTE *chargen_ptr;

    /* native screenshot bitmap */
    BYTE *bitmap_ptr;

    /* native screenshot low part bitmap */
    BYTE *bitmap_low_ptr;

    /* native screenshot high part bitmap */
    BYTE *bitmap_high_ptr;

    /* native screenshot color ram */
    BYTE *color_ram_ptr;
} screenshot_t;

#define SCREENSHOT_MODE_PALETTE 0
#define SCREENSHOT_MODE_RGB32   1
#define SCREENSHOT_MODE_RGB24   2

/* Functions called by external emulator code.  */
extern int screenshot_init(void);
extern void screenshot_shutdown(void);
extern int screenshot_save(const char *drvname, const char *filename,
                           struct video_canvas_s *canvas);
extern int screenshot_record(void);
extern void screenshot_stop_recording(void);
extern int screenshot_is_recording(void);
extern void screenshot_prepare_reopen(void);
extern void screenshot_try_reopen(void);

#ifdef FEATURE_CPUMEMHISTORY
extern int memmap_screenshot_save(const char *drvname, const char *filename, int x_size, int y_size, BYTE *gfx, BYTE *palette);
#endif

#endif
