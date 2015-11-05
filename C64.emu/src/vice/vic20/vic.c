/*
 * vic.c - A line-based VIC-I emulation (under construction).
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Matthies <andreas.matthies@gmx.net>
 *
 * 16/24bpp support added by
 *  Steven Tieu <stieu@physics.ubc.ca>
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

#include "archdep.h"
#include "clkguard.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "raster-line.h"
#include "raster-modes.h"
#include "resources.h"
#include "screenshot.h"
#include "snapshot.h"
#include "types.h"
#include "vic-cmdline-options.h"
#include "vic-draw.h"
#include "vic-mem.h"
#include "vic-resources.h"
#include "vic-snapshot.h"
#include "vic-timing.h"
#include "vic-color.h"
#include "vic.h"
#include "victypes.h"
#include "vic20.h"
#include "vic20-resources.h"
#include "vic20mem.h"
#include "vic20memrom.h"
#include "videoarch.h"
#include "viewport.h"
#include "vsync.h"


vic_t vic;

static void vic_set_geometry(void);

static void clk_overflow_callback(CLOCK sub, void *unused_data)
{
    if (vic.light_pen.trigger_cycle < CLOCK_MAX) {
        vic.light_pen.trigger_cycle -= sub;
    }
}

void vic_change_timing(machine_timing_t *machine_timing, int border_mode)
{
    vic_timing_set(machine_timing, border_mode);
    if (vic.initialized) {
        vic_set_geometry();
        raster_mode_change();
    }
}

/* return pixel aspect ratio for current video mode
 * based on http://codebase64.com/doku.php?id=base:pixel_aspect_ratio
 */
static float vic_get_pixel_aspect(void)
{
    int video;
    resources_get_int("MachineVideoStandard", &video);
    switch (video) {
        case MACHINE_SYNC_PAL:
            return 1.66574035f / 2.0f;
        case MACHINE_SYNC_NTSC:
            return 1.50411479f / 2.0f;
        default:
            return 1.0f;
    }
}

/* return type of monitor used for current video mode */
static int vic_get_crt_type(void)
{
    int video;
    resources_get_int("MachineVideoStandard", &video);
    switch (video) {
        case MACHINE_SYNC_PAL:
        case MACHINE_SYNC_PALN:
            return 1; /* PAL */
        default:
            return 0; /* NTSC */
    }
}

static void vic_set_geometry(void)
{
    unsigned int width, height;

    width = vic.display_width * VIC_PIXEL_WIDTH;
    height = vic.last_displayed_line - vic.first_displayed_line + 1;

    raster_set_geometry(&vic.raster,
                        width, height,
                        vic.screen_width * VIC_PIXEL_WIDTH,
                        vic.screen_height,
                        22 * 8 * VIC_PIXEL_WIDTH,
                        23 * 8,          /* handled dynamically  */
                        22, 23,          /* handled dynamically  */
                        /* handled dynamically  */
                        12 * 4 * VIC_PIXEL_WIDTH,
                        38 * 2 - vic.first_displayed_line,
                        1,
                        vic.first_displayed_line,
                        vic.last_displayed_line,
                        vic.screen_width + vic.max_text_cols * 8,
                        vic.screen_width + vic.max_text_cols * 8);
#ifdef __MSDOS__
    video_ack_vga_mode();
#endif

    vic.raster.geometry->pixel_aspect_ratio = vic_get_pixel_aspect();
    vic.raster.viewport->crt_type = vic_get_crt_type();
}


/* Notice: The screen origin X register has a 4-pixel granularity, so our
   write accesses are always aligned. */

void vic_raster_draw_handler(void)
{
    /* emulate the line */
    raster_line_emulate(&vic.raster);

    /* handle start of frame */
    if (vic.raster.current_line == 0) {
        raster_skip_frame(&vic.raster,
                          vsync_do_vsync(vic.raster.canvas,
                                         vic.raster.skip_frame));
    }
}

static void update_pixel_tables(raster_t *raster)
{
    unsigned int i;

    for (i = 0; i < 256; i++) {
        vic.pixel_table.sing[i] = i;
        *((BYTE *)(vic.pixel_table.doub + i))
            = *((BYTE *)(vic.pixel_table.doub + i) + 1)
                  = vic.pixel_table.sing[i];
    }
}

static int init_raster(void)
{
    raster_t *raster;

    raster = &vic.raster;

    raster->sprite_status = NULL;
    raster_line_changes_init(raster);

    if (raster_init(raster, VIC_NUM_VMODES) < 0) {
        return -1;
    }

    update_pixel_tables(raster);

    raster_modes_set_idle_mode(raster->modes, VIC_IDLE_MODE);
    resources_touch("VICVideoCache");

    vic_set_geometry();

    vic_color_update_palette(raster->canvas);

    raster_set_title(raster, machine_name);

    if (raster_realize(raster) < 0) {
        return -1;
    }

    raster->display_ystart = vic.first_displayed_line;
    raster->display_ystop = vic.first_displayed_line + 1;
    raster->display_xstart = 0;
    raster->display_xstop = 1;

    return 0;
}


/* Initialization. */
raster_t *vic_init(void)
{
    vic.log = log_open("VIC");

    /* vic_change_timing(); */

    if (init_raster() < 0) {
        return NULL;
    }

    vic.auxiliary_color = 0;
    vic.mc_border_color = 0;
    vic.old_auxiliary_color = 0;
    vic.old_mc_border_color = 0;
    vic.reverse = 0;
    vic.old_reverse = 0;
    vic.half_char_flag = 0;

    /* FIXME: Where do these values come from? */
    vic.light_pen.state = 0;
    vic.light_pen.triggered = 0;
    vic.light_pen.x = 87;
    vic.light_pen.y = 234;
    vic.light_pen.x_extra_bits = 1;
    vic.light_pen.trigger_cycle = CLOCK_MAX;

    /* FIXME */
    vic.char_height = 8;
    vic.row_increase_line = 8;
    vic.pending_text_cols = 22;
    vic.text_lines = 23;

    vic_reset();

    vic_draw_init();

    vic.initialized = 1;

    clk_guard_add_callback(maincpu_clk_guard, clk_overflow_callback, NULL);

    resources_touch("VICDoubleSize");

    return &vic.raster;
}

struct video_canvas_s *vic_get_canvas(void)
{
    return vic.raster.canvas;
}

/* Reset the VIC-I chip. */
void vic_reset(void)
{
/*    vic_change_timing();*/

    raster_reset(&vic.raster);

/*    vic_set_geometry();*/

    vic.row_counter = 0;
    vic.memptr = 0;
    vic.memptr_inc = 0;
    vic.area = VIC_AREA_IDLE;
    vic.raster_line = 0;
    vic.raster_cycle = 6; /* magic value from cpu_reset() (mainviccpu.c) */
    vic.fetch_state = VIC_FETCH_IDLE;
}

void vic_shutdown(void)
{
    raster_shutdown(&vic.raster);
}

void vic_screenshot(screenshot_t *screenshot)
{
    raster_screenshot(&vic.raster, screenshot);
    screenshot->chipid = "VIC";
    screenshot->video_regs = vic.regs;
    screenshot->screen_ptr = (vic.regs[0x02] & 0x80) ? mem_ram + 0x1e00 : mem_ram + 0x1000;
    switch (vic.regs[0x05] & 0xf) {
        case 0:
            screenshot->chargen_ptr = vic20memrom_chargen_rom;
            break;
        case 1:
            screenshot->chargen_ptr = vic20memrom_chargen_rom + 0x400;
            break;
        case 2:
            screenshot->chargen_ptr = vic20memrom_chargen_rom + 0x800;
            break;
        case 3:
            screenshot->chargen_ptr = vic20memrom_chargen_rom + 0xc00;
            break;
        case 5:
            screenshot->chargen_ptr = mem_ram + 0x9400;
            break;
        case 4:
        case 6:
        case 7:
        default:
            screenshot->chargen_ptr = NULL;
            break;
        case 8:
            screenshot->chargen_ptr = mem_ram;
            break;
        case 9:
            if (ram_block_0_enabled) {
                screenshot->chargen_ptr = mem_ram + 0x400;
            } else {
                screenshot->chargen_ptr = NULL;
            }
            break;
        case 10:
            if (ram_block_0_enabled) {
                screenshot->chargen_ptr = mem_ram + 0x800;
            } else {
                screenshot->chargen_ptr = NULL;
            }
            break;
        case 11:
            if (ram_block_0_enabled) {
                screenshot->chargen_ptr = mem_ram + 0xc00;
            } else {
                screenshot->chargen_ptr = NULL;
            }
            break;
        case 12:
            screenshot->chargen_ptr = mem_ram + 0x1000;
            break;
        case 13:
            screenshot->chargen_ptr = mem_ram + 0x1400;
            break;
        case 14:
            screenshot->chargen_ptr = mem_ram + 0x1800;
            break;
        case 15:
            screenshot->chargen_ptr = mem_ram + 0x1c00;
            break;
    }
    screenshot->bitmap_ptr = NULL;
    screenshot->bitmap_low_ptr = NULL;
    screenshot->bitmap_high_ptr = NULL;
    screenshot->color_ram_ptr = (vic.regs[0x02] & 0x80) ? mem_ram + 0x9600 : mem_ram + 0x9400;
}

void vic_async_refresh(struct canvas_refresh_s *refresh)
{
    raster_async_refresh(&vic.raster, refresh);
}

/* ------------------------------------------------------------------------- */

/* Set light pen input state. Used by c64cia1.c.  */
void vic_set_light_pen(CLOCK mclk, int state)
{
    if (state) {
        /* delay triggering by 1 cycle */
        vic.light_pen.trigger_cycle = mclk + 1;

        /* HACK for the magic 6 in the PAL dump */
        if ((vic.raster_line == (vic.screen_height - 1)) && (vic.raster_cycle == (vic.cycles_per_line - 2))) {
            vic.light_pen.x_extra_bits = 0;
        }
    }
    vic.light_pen.state = state;
}

/* Trigger the light pen. Used internally.  */
void vic_trigger_light_pen_internal(int retrigger)
{
    unsigned int x, y, cycle;

    /* Unset the trigger cycle.
       If this function was call from elsewhere before the cycle,
       then the light pen was triggered by other means than
       an actual light pen and the following "if" would make the
       scheduled "actual" triggering pointless. */
    vic.light_pen.trigger_cycle = CLOCK_MAX;

    if (vic.light_pen.triggered) {
        return;
    }

    vic.light_pen.triggered = 1;

    y = vic.raster_line;

    cycle = vic.raster_cycle;

    /* don't trigger on the last line, except on the first cycle */
    if ((y == (vic.screen_height - 1)) && (cycle > 1)) {
        return;
    }

    x = 2 * ((cycle + 1) % vic.cycles_per_line);

    /* HACK for the magic 6 in the PAL dump */
    x += vic.light_pen.x_extra_bits;

    vic.light_pen.x = x;
    vic.light_pen.y = y / 2;
    vic.light_pen.x_extra_bits = 1;
}

/* Calculate lightpen pulse time based on x/y */
CLOCK vic_lightpen_timing(int x, int y)
{
    CLOCK pulse_time = maincpu_clk;

    x += 0x70 - vic.cycle_offset;
    y += vic.first_displayed_line;

    /* Check if x would wrap to previous line */
    if (x < 0 /*TODO*/) {
        /* lightpen is off screen */
        pulse_time = 0;
    } else {
        pulse_time += (x / 8) + (y * vic.cycles_per_line);
        /* Remove frame alarm jitter */
        pulse_time -= maincpu_clk - VIC_LINE_START_CLK(maincpu_clk);

        /* Store x extra bits for sub CLK precision */
        vic.light_pen.x_extra_bits = (x >> 2) & 0x1;
    }

    return pulse_time;
}

/* Trigger the light pen. Used by lightpen.c only. */
void vic_trigger_light_pen(CLOCK mclk)
{
    /* Record the real trigger time */
    vic.light_pen.trigger_cycle = mclk;
}

/* ------------------------------------------------------------------------- */

static const char *fetch_state_name[] = {
    "idle",
    "start",
    "matrix",
    "chargen",
    "done"
};

/* Make a "real" 16b address from the 14b VIC address */
static inline int vic_dump_addr(int addr)
{
    int msb = ~((addr & 0x2000) << 2) & 0x8000;
    return (addr & 0x1fff) | msb;
}

int vic_dump(void)
{
    int xstart, ystart, xstop, ystop, cols, lines, addr;
    int matrix_base, char_base;

    mon_out("Raster cycle/line: %d/%d\n", vic.raster_cycle, vic.raster_line);

    matrix_base = ((vic.regs[5] & 0xf0) << 6) | ((vic.regs[2] & 0x80) << 2);
    char_base = (vic.regs[5] & 0xf) << 10;

    mon_out("Matrix: $%04x, Char: $%04x, Memptr: $%03x\n",
            vic_dump_addr(matrix_base),
            vic_dump_addr(char_base),
            vic.memptr);

    mon_out("Y counter: %d, char height: %d, offset: %i\n",
            vic.raster.ycounter,
            vic.char_height,
            vic.buf_offset);

    mon_out("Fetch: %s, from ", fetch_state_name[vic.fetch_state]);

    switch (vic.fetch_state) {
        case VIC_FETCH_MATRIX:
            addr = matrix_base + (vic.memptr + vic.buf_offset);
            mon_out("$%04x\n", vic_dump_addr(addr));
            break;

        case VIC_FETCH_CHARGEN:
            addr = char_base + (vic.vbuf * vic.char_height + (vic.raster.ycounter & ((vic.char_height >> 1) | 7)));

            mon_out("$%04x (vbuf $%02x)\n", vic_dump_addr(addr), vic.vbuf);
            break;
        default:
            mon_out("??\n");
            break;
    }

    mon_out("Size: X/Y - X/Y, chars\n");

    cols = vic.regs[2] & 0x7f;
    lines = (vic.regs[3] & 0x7e) >> 1;
    xstart = (vic.regs[0] & 0x7f) * 4;
    ystart = vic.regs[1] << 1;
    xstop = xstart + cols * 8;
    ystop = ystart + lines * vic.char_height;

    mon_out("  Set: %d/%d - %d/%d, %dx%d\n",
            xstart, ystart, xstop, ystop, cols, lines);

    mon_out(" Real: ");

    if (vic.fetch_state != VIC_FETCH_IDLE) {
        mon_out("%d/", vic.raster.display_xstart / VIC_PIXEL_WIDTH);
    } else {
        mon_out("?/");
    }

    if ((vic.area == VIC_AREA_DISPLAY) || (vic.area == VIC_AREA_DONE)) {
        mon_out("%d - ", vic.raster.display_ystart);
    } else {
        mon_out("? - ");
    }

    if (vic.fetch_state != VIC_FETCH_IDLE) {
        mon_out("%d/", vic.raster.display_xstop / VIC_PIXEL_WIDTH);
    } else {
        mon_out("?/");
    }

    if (vic.area == VIC_AREA_DONE) {
        mon_out("%d, ", vic.raster.display_ystop);
    } else {
        mon_out("?, ");
    }

    mon_out("%dx%d\n", vic.text_cols, vic.text_lines);

    return 0;
}
