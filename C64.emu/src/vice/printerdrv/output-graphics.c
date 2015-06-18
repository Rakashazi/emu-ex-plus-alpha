/*
 * output-graphics.c - Output a graphics file.
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "log.h"
#include "cmdline.h"
#include "gfxoutput.h"
#include "output-select.h"
#include "output-graphics.h"
#include "output.h"
#include "palette.h"
#include "resources.h"
#include "screenshot.h"
#include "types.h"


struct output_gfx_s {
    gfxoutputdrv_t *gfxoutputdrv;
    screenshot_t screenshot;
    BYTE *line;
    char *filename;
    unsigned int isopen;
    unsigned int line_pos;
    unsigned int line_no;
};
typedef struct output_gfx_s output_gfx_t;

static output_gfx_t output_gfx[NUM_OUTPUT_SELECT];

static unsigned int current_prnr;

/* ------------------------------------------------------------------------- */

/*
 * The palette colour order is black, white, blue, green, red.
 * The black and white printers only have the former two.
 */
static BYTE output_pixel_to_palette_index(BYTE pix)
{
    switch (pix) {
        case OUTPUT_PIXEL_BLACK:
            return 0;
        case OUTPUT_PIXEL_WHITE:
        default:
            return 1;
        case OUTPUT_PIXEL_BLUE:
            return 2;
        case OUTPUT_PIXEL_GREEN:
            return 3;
        case OUTPUT_PIXEL_RED:
            return 4;
    }
}

static void output_graphics_line_data(screenshot_t *screenshot, BYTE *data,
                                      unsigned int line, unsigned int mode)
{
    unsigned int i;
    BYTE *line_base;
    unsigned int color;

    line_base = output_gfx[current_prnr].line;

    switch (mode) {
        case SCREENSHOT_MODE_PALETTE:
            for (i = 0; i < screenshot->width; i++) {
                data[i] = output_pixel_to_palette_index(line_base[i]);
            }
            break;
        case SCREENSHOT_MODE_RGB32:
            for (i = 0; i < screenshot->width; i++) {
                color = output_pixel_to_palette_index(line_base[i]);
                data[i * 4] = screenshot->palette->entries[color].red;
                data[i * 4 + 1] = screenshot->palette->entries[color].green;
                data[i * 4 + 2] = screenshot->palette->entries[color].blue;
                data[i * 4 + 3] = 0;
            }
            break;
        default:
            log_error(LOG_ERR, "Invalid mode %i.", mode);
    }
}

/* ------------------------------------------------------------------------- */

static int output_graphics_open(unsigned int prnr,
                                output_parameter_t *output_parameter)
{
    const char *filename;
    int device = 0;
    output_gfx[prnr].gfxoutputdrv = gfxoutput_get_driver("BMP");

    if (output_gfx[prnr].gfxoutputdrv == NULL) {
        return -1;
    }

    switch (prnr) {
        case 0:
            resources_get_int("Printer4TextDevice", &device);
            break;
        case 1:
            resources_get_int("Printer5TextDevice", &device);
            break;
        case 2:
            resources_get_int("PrinterUserportTextDevice", &device);
            break;
    }

    resources_get_string_sprintf("PrinterTextDevice%d", &filename, device + 1);

    if (filename == NULL) {
        filename = "prngfx";
    }

    output_gfx[prnr].filename = lib_malloc(strlen(filename) + 3);
    sprintf(output_gfx[prnr].filename, "%s00", filename);

    output_gfx[prnr].screenshot.width = output_parameter->maxcol;
    output_gfx[prnr].screenshot.height = output_parameter->maxrow;
    output_gfx[prnr].screenshot.dpi_x = output_parameter->dpi_x;
    output_gfx[prnr].screenshot.dpi_y = output_parameter->dpi_y;
    output_gfx[prnr].screenshot.y_offset = 0;
    output_gfx[prnr].screenshot.palette = output_parameter->palette;

    lib_free(output_gfx[prnr].line);
    output_gfx[prnr].line = lib_malloc(output_parameter->maxcol);
    memset(output_gfx[prnr].line, OUTPUT_PIXEL_WHITE, output_parameter->maxcol);

    output_gfx[prnr].line_pos = 0;
    output_gfx[prnr].line_no = 0;

    output_gfx[prnr].screenshot.convert_line = output_graphics_line_data;
    output_gfx[prnr].isopen = 0;

    return 0;
}

static void output_graphics_close(unsigned int prnr)
{
    output_gfx_t *o = &(output_gfx[prnr]);

    /* only do this if something has actually been printed on this page */
    if (o->isopen) {
        unsigned int i;

        /* output current line */
        current_prnr = prnr;
        (o->gfxoutputdrv->write)(&o->screenshot);
        o->line_no++;

        /* fill rest of page with blank lines */
        memset(o->line, OUTPUT_PIXEL_WHITE, o->screenshot.width);
        for (i = o->line_no; i < o->screenshot.height; i++) {
            (o->gfxoutputdrv->write)(&o->screenshot);
        }

        /* close output */
        o->gfxoutputdrv->close(&o->screenshot);
        o->isopen = 0;
    }

    /* free filename */
    lib_free(o->filename);
    o->filename = NULL;
}

static int output_graphics_putc(unsigned int prnr, BYTE b)
{
    output_gfx_t *o = &(output_gfx[prnr]);

    if (b == OUTPUT_NEWLINE) {
        /* if output is not open yet, open it now */
        if (!o->isopen) {
            int i;

            /* increase page count in filename */
            i = (int)strlen(o->filename);
            o->filename[i - 1]++;
            if (o->filename[i - 1] > '9') {
                o->filename[i - 1] = '0';
                o->filename[i - 2]++;
            }

            /* open output file */
            o->gfxoutputdrv->open(&o->screenshot, o->filename);
            o->isopen = 1;
            o->line_pos = 0;
            o->line_no = 0;
        }

        /* write buffered line to output and clear buffer */
        current_prnr = prnr;
        (o->gfxoutputdrv->write)(&o->screenshot);
        memset(o->line, OUTPUT_PIXEL_WHITE, o->screenshot.width);
        o->line_pos = 0;

        /* check for bottom of page.  If so, close output file */
        o->line_no++;
        if (o->line_no == o->screenshot.height) {
            o->gfxoutputdrv->close(&o->screenshot);
            o->isopen = 0;
        }
    } else {
        /* store pixel in buffer */
        if (o->line_pos < o->screenshot.width) {
            o->line[o->line_pos] = b;
        }
        if (o->line_pos < o->screenshot.width - 1) {
            o->line_pos++;
        }
    }

    return 0;
}

static int output_graphics_getc(unsigned int prnr, BYTE *b)
{
    return 0;
}

static int output_graphics_flush(unsigned int prnr)
{
    return 0;
}

/* ------------------------------------------------------------------------- */

void output_graphics_init(void)
{
    unsigned int i;

    for (i = 0; i < 3; i++) {
        output_gfx[i].filename = NULL;
        output_gfx[i].line = NULL;
        output_gfx[i].line_pos = 0;
    }
}

int output_graphics_init_resources(void)
{
    output_select_t output_select;

    output_select.output_name = "graphics";
    output_select.output_open = output_graphics_open;
    output_select.output_close = output_graphics_close;
    output_select.output_putc = output_graphics_putc;
    output_select.output_getc = output_graphics_getc;
    output_select.output_flush = output_graphics_flush;

    output_select_register(&output_select);

    return 1;
}
