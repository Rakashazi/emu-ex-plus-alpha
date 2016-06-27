/*
 * vdc.c - MOS 8563 (VDC) emulation.
 *
 * Written by
 *  Markus Brenner <markus@brenner.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "alarm.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "raster.h"
#include "raster-line.h"
#include "raster-modes.h"
#include "resources.h"
#include "screenshot.h"
#include "snapshot.h"
#include "types.h"
#include "vdc-cmdline-options.h"
#include "vdc-color.h"
#include "vdc-draw.h"
#include "vdc-resources.h"
#include "vdc-snapshot.h"
#include "vdc.h"
#include "vdctypes.h"
#include "video.h"
#include "viewport.h"

#if defined(__MSDOS__)
#include "videoarch.h"
#endif

vdc_t vdc;

static void vdc_raster_draw_alarm_handler(CLOCK offset, void *data);

/* return pixel aspect ratio for current video mode */
/* FIXME: calculate proper values.
   look at http://www.codebase64.org/doku.php?id=base:pixel_aspect_ratio&s[]=aspect
   for an example calculation
*/
static float vdc_get_pixel_aspect(void)
{
/*
    int video;
    resources_get_int("MachineVideoStandard", &video);
    switch (video) {
        case MACHINE_SYNC_PAL:
        case MACHINE_SYNC_PALN:
            return 0.936f;
        default:
            return 0.75f;
    }
*/
    return 1.0f; /* assume 1:1 for CGA */
}

/* return type of monitor used for current video mode */
static int vdc_get_crt_type(void)
{
    return 2; /* RGB */
}

static void vdc_set_geometry(void)
{
    raster_t *raster;
    unsigned int screen_width, screen_height;
    unsigned int first_displayed_line, last_displayed_line;
    unsigned int screen_xpix, screen_ypix;
    unsigned int border_height, border_width;
    unsigned int vdc_25row_start_line, vdc_25row_stop_line;
    unsigned int displayed_width, displayed_height;
    unsigned int vdc_80col_start_pixel, vdc_80col_stop_pixel;
    unsigned int charwidth;

    raster = &vdc.raster;

    screen_width = VDC_SCREEN_WIDTH;
    screen_height = VDC_SCREEN_HEIGHT;

    first_displayed_line = vdc.first_displayed_line;
    last_displayed_line = vdc.last_displayed_line;

    screen_xpix = vdc.screen_xpix;
    screen_ypix = vdc.screen_ypix;

    border_width = vdc.border_width;
    border_height = vdc.border_height;

    vdc_25row_start_line = border_height;
    vdc_25row_stop_line = vdc_25row_start_line + screen_ypix;

    if(vdc.regs[25] & 0x10) { /* double pixel a.k.a 40column mode */
        charwidth = 2 * (vdc.regs[22] >> 4);
    } else { /* 80 column mode */
        charwidth = 1 + (vdc.regs[22] >> 4);
    }
    vdc_80col_start_pixel = border_width;
    vdc_80col_stop_pixel = vdc_80col_start_pixel + charwidth * vdc.screen_text_cols;

    displayed_width = VDC_SCREEN_WIDTH;
    displayed_height = last_displayed_line - first_displayed_line + 1;

/*
printf("SH: %03i SW: %03i\n", screen_height, screen_width);
printf("YP: %03i XP: %03i\n", screen_ypix, screen_xpix);
printf("DH: %03i DW: %03i\n", displayed_height, displayed_width);
printf("BH: %03i BW: %03i\n", border_height, border_width);
printf("SA: %03i SO: %03i\n", vdc_25row_start_line, vdc_25row_stop_line);
printf("LD: %03i FD: %03i\n", last_displayed_line, first_displayed_line);
*/

    raster->display_ystart = vdc_25row_start_line;
    raster->display_ystop = vdc_25row_stop_line;
    raster->display_xstart = vdc_80col_start_pixel;
    raster->display_xstop = vdc_80col_stop_pixel;

    raster_set_geometry(raster,
                        displayed_width, displayed_height,  /* canvas - physically displayed width/height, ie size of visible window */
                        screen_width, screen_height,        /* width/height of virtual screen */
                        screen_xpix, screen_ypix,   /* size of the foreground area (pixels) */
                        VDC_SCREEN_MAX_TEXTCOLS, vdc.screen_textlines,  /* size of the foreground area (characters) */
                        border_width, vdc_25row_start_line, /* gfx_pos_x/y - position of visible screen in virtual coords */
                        0,  /* gfx_area_moves */
                        first_displayed_line,   /* 1st line of virtual screen physically visible */
                        last_displayed_line,    /* last line physically visible */
                        0, 0); /* extra off screen border left / right */

    raster->geometry->pixel_aspect_ratio = vdc_get_pixel_aspect();
    raster->geometry->char_pixel_width = charwidth;
    raster->viewport->crt_type = vdc_get_crt_type();
}

static void vdc_invalidate_cache(raster_t *raster, unsigned int screen_height)
{
    raster_new_cache(raster, screen_height);
}

static int init_raster(void)
{
    raster_t *raster;

    raster = &vdc.raster;

    raster->sprite_status = NULL;
    raster_line_changes_init(raster);

    if (raster_init(raster, VDC_NUM_VMODES) < 0) {
        return -1;
    }

    raster_modes_set_idle_mode(raster->modes, VDC_IDLE_MODE);
    resources_touch("VDCVideoCache");

    vdc_set_geometry();

    if (vdc_color_update_palette(vdc.raster.canvas) < 0) {
        log_error(vdc.log, "Cannot load palette.");
        return -1;
    }

    raster_set_title(raster, machine_name);

    if (raster_realize(raster) < 0) {
        return -1;
    }

    raster->border_color = 0;

    /* FIXME: this seems to be the only way to disable cache on VDC.
       The GUI (at least on win32) doesn't let you do it */
    /* raster->cache_enabled = 0; */  /* Force disable cache for testing non-cache mode */

    return 0;
}

/* Initialize the VDC emulation. */
raster_t *vdc_init(void)
{
    vdc.initialized = 0;

    vdc.log = log_open("VDC");

    vdc.raster_draw_alarm = alarm_new(maincpu_alarm_context, "VdcRasterDraw",
                                      vdc_raster_draw_alarm_handler, NULL);

    vdc_powerup();

    if (init_raster() < 0) {
        return NULL;
    }

    vdc.force_resize = 0;
    vdc.force_repaint = 0;

    vdc_draw_init();

    vdc.initialized = 1;

    /*vdc_set_geometry();*/
    resources_touch("VDCDoubleSize");

    return &vdc.raster;
}

struct video_canvas_s *vdc_get_canvas(void)
{
    return vdc.raster.canvas;
}


static void vdc_set_next_alarm(CLOCK offset)
{
    unsigned int next_alarm;
    static unsigned int next_line_accu = 0;

    next_line_accu += vdc.xsync_increment;
    next_alarm = next_line_accu >> 16;
    next_line_accu -= (next_alarm << 16);

    /* Set the next draw event. */
    alarm_set(vdc.raster_draw_alarm, maincpu_clk + (CLOCK)next_alarm - offset);
}

static void vdc_update_geometry(void)
{   /* This sets things based on registers:  2, 6, 9
    it sets vdc.screenheight
                last_displayed_line
                screen_textlines
                screen_ypix
                screen_text_cols
                hsync_shift
                border_width    */
    
    int charwidth, hsync;

    /* Leave this fixed so the window isn't getting constantly resized */
    vdc.screen_height = VDC_SCREEN_HEIGHT;

    vdc.last_displayed_line = MIN(VDC_LAST_DISPLAYED_LINE, vdc.screen_height - 1);

    /* TODO get rid of this if/when it we don't need it anymore..  */
/*     printf("BH:%1i 0:%02X 1:%02X 2:%02X 3:%02X 4:%02X 5:%02X 6:%02X 7:%02X 9:%02X 22:%02X 24:%02X 25:%02X 26:%02X\n",
        vdc.border_height, vdc.regs[0], vdc.regs[1], vdc.regs[2], vdc.regs[3], vdc.regs[4], (vdc.regs[5] & 0x1f), vdc.regs[6], vdc.regs[7], vdc.regs[9] & 0x1f, vdc.regs[22], vdc.regs[24], vdc.regs[25], vdc.regs[26] );
*/

    vdc.screen_textlines = vdc.regs[6];

    vdc.screen_ypix = vdc.regs[6] * ((vdc.regs[9] & 0x1f) + 1);

    if (vdc.regs[1] >= 6 && vdc.regs[1] <= VDC_SCREEN_MAX_TEXTCOLS) {
        vdc.screen_text_cols = vdc.regs[1];
    } else if (vdc.regs[1] < 6) {
        vdc.screen_text_cols = 6;
    } else {
        vdc.screen_text_cols = VDC_SCREEN_MAX_TEXTCOLS;
    }

    if(vdc.regs[25] & 0x10) { /* double pixel a.k.a 40column mode */
        charwidth = 2 * (vdc.regs[22] >> 4);
        hsync = 62 * 16            /* 992 */
            - vdc.regs[2] * charwidth;       /* default (55) - 880 = 112 */
    } else { /* 80 column mode */
        charwidth = 1 + (vdc.regs[22] >> 4);
        hsync = 116 * 8            /* 928 */
            - vdc.regs[2] * charwidth;       /* default (102) - 816 = 112 */
    }
    if (hsync < 0) {
            hsync = 0;
    }
    vdc.hsync_shift = hsync;

    /* clamp the display within the right edge of the screen */
    if (vdc.hsync_shift + (vdc.screen_text_cols * charwidth) > VDC_SCREEN_WIDTH ) {
        vdc.hsync_shift = VDC_SCREEN_WIDTH - (vdc.screen_text_cols * charwidth);
    }
    vdc.border_width = vdc.hsync_shift;

    vdc.update_geometry = 0;
}


/* Reset the VDC chip */
void vdc_reset(void)
{
    if (vdc.initialized) {
        raster_reset(&vdc.raster);
    }

    vdc.frame_counter = 0;
    vdc.screen_text_cols = VDC_SCREEN_MAX_TEXTCOLS;
    vdc.xsmooth = 7;
    vdc.regs[0] = 126;
    vdc.regs[1] = 102;
    vdc.xchars_total = vdc.regs[0] + 1;
    vdc_calculate_xsync();
    vdc.regs[4] = 39;
    vdc.regs[5] = 0;
    vdc.regs[6] = 25;
    vdc.regs[9] = vdc.raster_ycounter_max = 7;
    vdc.attribute_offset = 0;
    vdc.border_height = 59;
    vdc.bytes_per_char = 16;
    vdc_update_geometry();
    vdc_set_next_alarm((CLOCK)0);
    vdc.light_pen.x = vdc.light_pen.y = vdc.light_pen.triggered = 0;
}

/* This _should_ put the VDC in the same state as powerup */
void vdc_powerup(void)
{
    /* Setup the VDC's ram with a 0xff00ff00.. pattern */
    unsigned int i;
    BYTE v = 0xff;
    for (i = 0; i < sizeof(vdc.ram); i++) {
        vdc.ram[i] = v;
        v ^= 0xff;
    }
    memset(vdc.regs, 0, sizeof(vdc.regs));
    vdc.mem_counter = 0;
    vdc.mem_counter_inc = 0;

    vdc.screen_xpix = VDC_SCREEN_XPIX;
    vdc.first_displayed_line = VDC_FIRST_DISPLAYED_LINE;
    vdc.last_displayed_line = VDC_LAST_DISPLAYED_LINE;

    vdc_reset();
}



/* ---------------------------------------------------------------------*/

/* Trigger the light pen.  */
void vdc_trigger_light_pen(CLOCK mclk)
{
    vdc.light_pen.triggered = 1;
    vdc.regs[16] = vdc.light_pen.y;
    vdc.regs[17] = vdc.light_pen.x;
}

/* Calculate lightpen pulse time based on x/y */
CLOCK vdc_lightpen_timing(int x, int y)
{
    CLOCK pulse_time;

    double vdc_cycles_per_line, host_cycles_per_second;
    host_cycles_per_second = (double)machine_get_cycles_per_second();
    vdc_cycles_per_line = (double)(vdc.xchars_total) * 8.0
                          * host_cycles_per_second / VDC_DOT_CLOCK;

    /* FIXME - this doesn't work properly.. */
    pulse_time = maincpu_clk;
    pulse_time += (CLOCK)((x / 8) + (y * vdc_cycles_per_line));

    /* Figure out what values should go into the registers when triggered */
    vdc.light_pen.y = (y - (int)vdc.first_displayed_line - 1) / ((int)(vdc.regs[9] & 0x1f) + 1);
    if (vdc.light_pen.y < 0) {
        vdc.light_pen.y += vdc.regs[4] + 1;
    }
    vdc.light_pen.x = (x - vdc.border_width) / ((vdc.regs[22] >> 4) + 1) + 22;
    return pulse_time;
}

/* ---------------------------------------------------------------------*/

/* Set the memory pointers according to the values in the registers. */
void vdc_update_memory_ptrs(unsigned int cycle)
{
}

static void vdc_increment_memory_pointer(void)
{
    vdc.mem_counter_inc = vdc.screen_text_cols;
    if (vdc.raster.ycounter >= vdc.raster_ycounter_max) {
        vdc.mem_counter += vdc.mem_counter_inc + vdc.regs[27];
    }

    vdc.raster.ycounter = (vdc.raster.ycounter + 1)
                          % (vdc.raster_ycounter_max + 1);

    vdc.bitmap_counter += vdc.mem_counter_inc + vdc.regs[27];
}

static void vdc_set_video_mode(void)
{
    vdc.raster.video_mode = (vdc.regs[25] & 0x80)
                            ? VDC_BITMAP_MODE : VDC_TEXT_MODE;

    if (vdc.raster.ycounter > (unsigned int)(vdc.regs[9] & 0x1f)) {
        vdc.raster.video_mode = VDC_IDLE_MODE;
    }
}


/* Redraw the current raster line. */
static void vdc_raster_draw_alarm_handler(CLOCK offset, void *data)
{
    int in_idle_state, calculated_border_height;
    static unsigned int old_screen_adr, old_attribute_adr, screen_ystart, need_increment_memory_pointer;

    /* Update the memory pointers just before we draw the next line (vs after last line),
       in case relevent registers changed since last call. */
    if (need_increment_memory_pointer) {
        vdc_increment_memory_pointer();
        need_increment_memory_pointer = 0;
    }

    /* VDC locks in the screen/attr start addresses after the last raster line of foreground */
    if (vdc.raster.current_line == vdc.border_height + vdc.screen_ypix + 1) {
        vdc.screen_adr = ((vdc.regs[12] << 8) | vdc.regs[13])
                         & vdc.vdc_address_mask;
        vdc.attribute_adr = ((vdc.regs[20] << 8) | vdc.regs[21])
                            & vdc.vdc_address_mask;
        if (old_screen_adr != vdc.screen_adr || old_attribute_adr != vdc.attribute_adr) {
            /* the cache can't cleanly handle these changing */
            vdc.force_repaint = 1;
            old_screen_adr = vdc.screen_adr;
            old_attribute_adr = vdc.attribute_adr;
        }
    }

    if (vdc.raster.current_line == 0) {
        /* The top border position is based on the position of the vertical
           sync pulse [7] in relation to the total height of the screen [4]
           and the width of the sync pulse [3] */
        calculated_border_height = (vdc.regs[4] + 1 - vdc.regs[7])  /* # of rows from sync pulse */
                                   * ((vdc.regs[9] & 0x1f) + 1)     /* height of each row (R9) */
                                   - (vdc.regs[3] >> 4);            /* vertical sync pulse width */

        if (calculated_border_height >= 0) {
            vdc.border_height = calculated_border_height;
        } else {
            vdc.border_height = 0;
        }
        vdc.screen_ypix = vdc.regs[6] * ((vdc.regs[9] & 0x1f) + 1);
        /* screen_ystart is the raster line the foreground data actually starts on, which may be above or below the border */
        screen_ystart = vdc.border_height + (((vdc.regs[9] & 0x1f) - (vdc.regs[24] & 0x1f)) & 0x1f);  /* - R24 is vertical smooth scroll, which interacts with the screen & R9 like this based on experimentation. */
        vdc.border_height = vdc.border_height + (vdc.regs[9] & 0x1f);
        /* fix to catch the end of the display for the bitmap/character memory pointers */
        if ((vdc.border_height + vdc.screen_ypix + 1) > vdc.last_displayed_line) {
            vdc.screen_ypix = vdc.last_displayed_line - vdc.border_height - 1;
        }
        vdc.raster.display_ystart = vdc.border_height;
        vdc.raster.display_ystop = vdc.border_height + vdc.screen_ypix;
        vdc.row_counter = 0;
        vdc.row_counter_y = vdc.raster_ycounter_max;
        vdc.raster.video_mode = VDC_IDLE_MODE;
        vdc.mem_counter = 0;
        need_increment_memory_pointer = 0;
        vdc.bitmap_counter = 0;
        vdc.raster.ycounter = 0;
        vdc.frame_counter++;
        if (vdc.regs[24] & 0x20) {
            vdc.attribute_blink = vdc.frame_counter & 16;
        } else {
            vdc.attribute_blink = vdc.frame_counter & 8;
        }
        if (vdc.update_geometry) {
            vdc_update_geometry();
            vdc.force_resize = 1;
            vdc.force_repaint = 1;
            /* Screen height has changed, so do not invalidate cache with
               the new value.  It will be recreated by resize anyway.  */
            vdc.force_cache_flush = 0;
        } else {
            if (vdc.force_cache_flush) {
                vdc_invalidate_cache(&vdc.raster, vdc.screen_height);
                vdc.force_cache_flush = 0;
            }
        }

        if (vdc.force_resize) {
            if (vdc.initialized) {
                vdc_set_geometry();
                raster_mode_change();
            }
            vdc.force_resize = 0;
        }

        if (vdc.force_repaint) {
            vdc.force_repaint = 0;
            raster_force_repaint(&vdc.raster);
        }
    }

    /* If in_idle_state then we are not drawing anything on the current raster line */
    in_idle_state = (vdc.raster.current_line < vdc.border_height)
                    || vdc.raster.current_line < screen_ystart
                    || (vdc.raster.current_line >= (vdc.border_height + vdc.screen_ypix));

    if (!in_idle_state) {
        vdc_set_video_mode();
    } else {
        vdc.raster.video_mode = VDC_IDLE_MODE;
    }

    /* actually draw the current raster line */
    raster_line_emulate(&vdc.raster);

#ifdef __MSDOS__
    if (vdc.raster.canvas->viewport->update_canvas) {
        canvas_set_border_color(vdc.raster.canvas, vdc.raster.border_color);
    }
#endif

    /* see if we still should be drawing things - if we haven't drawn more than regs[6] rows since the top border */
    if (!in_idle_state) {
        vdc.row_counter_y--;
        if (vdc.row_counter_y < 0) {
            vdc.row_counter_y = vdc.raster_ycounter_max;
            /* update the row counter if we are starting a new line */
            vdc.row_counter++;
            /* check if we are at the end of the display */
            if (vdc.row_counter == vdc.regs[6]) {
                /* vdc.last_displayed_line = vdc.raster.current_line; */
                /* FIXME - this is really a hack to lock in the screen/attr addresses at the next raster alarm handler */
                vdc.screen_ypix = vdc.raster.current_line - vdc.border_height;
            }
        }
    }

    /* update the memory pointers if we are past screen_ystart, which may be above or below the top border */
    need_increment_memory_pointer = (vdc.raster.current_line > screen_ystart);

    vdc_set_next_alarm(offset);
}


void vdc_calculate_xsync(void)
{
    double vdc_cycles_per_line, host_cycles_per_second;

    host_cycles_per_second = (double)machine_get_cycles_per_second();

    vdc_cycles_per_line = (double)(vdc.xchars_total) * 8.0
                          * host_cycles_per_second / VDC_DOT_CLOCK;

    vdc.xsync_increment = (unsigned int)(vdc_cycles_per_line * 65536);
}

void vdc_set_canvas_refresh(int enable)
{
    raster_set_canvas_refresh(&vdc.raster, enable);
}

int vdc_write_snapshot_module(snapshot_t *s)
{
    return vdc_snapshot_write_module(s);
}

int vdc_read_snapshot_module(snapshot_t *s)
{
    return vdc_snapshot_read_module(s);
}

void vdc_screenshot(screenshot_t *screenshot)
{
    raster_screenshot(&vdc.raster, screenshot);
    screenshot->chipid = "VDC";
    screenshot->video_regs = vdc.regs;
    screenshot->screen_ptr = vdc.ram + vdc.screen_adr;
    screenshot->chargen_ptr = vdc.ram + vdc.chargen_adr;
    screenshot->bitmap_ptr = NULL; /* todo */
    screenshot->bitmap_low_ptr = NULL;
    screenshot->bitmap_high_ptr = NULL;
    screenshot->color_ram_ptr = vdc.ram + vdc.attribute_adr;
}

void vdc_async_refresh(struct canvas_refresh_s *refresh)
{
    raster_async_refresh(&vdc.raster, refresh);
}

void vdc_shutdown(void)
{
    raster_shutdown(&vdc.raster);
}
