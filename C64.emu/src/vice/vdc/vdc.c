/*
 * vdc.c - MOS 8563 (VDC) emulation.
 *
 * Written by
 *  Markus Brenner <markus@brenner.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *  Errol Smith <strobey@users.sourceforge.net>
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
#include "raster-canvas.h"
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
    /* unsigned int first_displayed_line, last_displayed_line; */
    unsigned int screen_xpix; /*, screen_ypix; */
    unsigned int border_width;  /* border_height, */
    /* unsigned int vdc_25row_start_line, vdc_25row_stop_line; */
    /* unsigned int displayed_width, displayed_height; */
    unsigned int vdc_80col_start_pixel, vdc_80col_stop_pixel;

    raster = &vdc.raster;
    /* total visible pixels including border(s) - constants defined in vdctypes.h */
    screen_width = VDC_SCREEN_WIDTH;    /* 856 */
    screen_height = 288;    /* maximum number of raster lines visible on a PAL system. also the same as old winvice    //VDC_SCREEN_HEIGHT;  //312 */

    screen_xpix = vdc.screen_xpix;

    border_width = vdc.border_width;

    vdc_80col_start_pixel = border_width;
    vdc_80col_stop_pixel = vdc_80col_start_pixel + vdc.charwidth * vdc.screen_text_cols;


/*
printf("SH: %03i SW: %03i\n", screen_height, screen_width);
printf("YP: %03i XP: %03i\n", screen_ypix, screen_xpix);
printf("DH: %03i DW: %03i\n", displayed_height, displayed_width);
printf("BH: %03i BW: %03i\n", border_height, border_width);
printf("SA: %03i SO: %03i\n", vdc_25row_start_line, vdc_25row_stop_line);
printf("LD: %03i FD: %03i\n", last_displayed_line, first_displayed_line);
*/

    raster->display_ystart = 1;
    raster->display_ystop = 1 + VDC_SCREEN_HEIGHT;
    raster->display_xstart = vdc_80col_start_pixel;
    raster->display_xstop = vdc_80col_stop_pixel;

    raster_set_geometry(raster,
                        screen_width, screen_height,  /* canvas - physically displayed width/height, ie size of visible window */
                        screen_width, 1024,        /* width/height of virtual screen - FIXME we should make the width 1024 as well but the xsync code interacts with this the wrong way */
                        screen_xpix, screen_height,   /* size of the foreground area (pixels) */
                        VDC_SCREEN_MAX_TEXTCOLS, 256,  /* size of the foreground area (characters) */
                        border_width, 0, /* gfx_pos_x/y - position of visible screen in virtual coords */
                        0,  /* gfx_area_moves */
                        0,   /* 1st line of virtual screen physically visible */
                        1024 -1,    /* last line physically visible */
                        0, 0); /* extra off screen border left / right */

    raster->geometry->pixel_aspect_ratio = vdc_get_pixel_aspect();
    raster->geometry->char_pixel_width = vdc.charwidth;
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

    int hsync;

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

    /* FIXME: this seems to have semi-randomly selected constants... */
    if(vdc.regs[25] & 0x10) { /* double pixel a.k.a 40column mode */
        hsync = 62 * 16            /* 992 */
            - vdc.regs[2] * vdc.charwidth;       /* default (55) - 880 = 112 */
    } else { /* 80 column mode */
        hsync = 116 * 8            /* 928 */
            - vdc.regs[2] * vdc.charwidth;       /* default (102) - 816 = 112 */
    }
    /* FIXME sync / left border calculation should probably be something like:
        Total Width: (127+1) * 8 = 1024     - Hsync Pos: 102 * 8 = 816    - Hsync width: 9 * 8 = 72    = 136 (17 chars)
        e.g. hsync = (vdc.regs[0] + 1) * vdc.charwidth - vdc.regs[2] * vdc.charwidth - (vdc.regs[3] & 0x0F) * vdc.charwidth; */
    /* FIXME: It's potentially valid for the active display area to start off the left (or right) of the display,
        because the VDC can put the horizontal sync anywhere. But we can't really emulate that (yet) without things crashing. */
    if (hsync < 0) {
            hsync = 0;
    }
    vdc.hsync_shift = hsync;

    /* clamp the display within the right edge of the screen */
    if ((vdc.screen_text_cols * vdc.charwidth) > VDC_SCREEN_WIDTH ) {
        /* bounds check so we don't wind up with a "negative unsigned" hsync_shift (i.e. a massive value) which then causes a segfault in vdc-draw.. */
        vdc.hsync_shift = 0;
    } else if (vdc.hsync_shift + (vdc.screen_text_cols * vdc.charwidth) > VDC_SCREEN_WIDTH ) {
        vdc.hsync_shift = VDC_SCREEN_WIDTH - (vdc.screen_text_cols * vdc.charwidth);
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
    vdc.regs[8] = 0;
	vdc.interlaced = 0;
    vdc.regs[9] = vdc.raster_ycounter_max = 7;
    vdc.regs[22] = 0x78;
    vdc.charwidth = 8;
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
    uint8_t v = 0xff;
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
    /* FIXME: use it or lose it */
}


/*
 * Temporarily disabled due to annoying compiler warnings
 */
#if 0
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

static void vdc_increment_memory_pointer_interlace_bitmap(void)
{   /* This is identical to above (and should remain so), we just don't increment the bitmap pointer */
    vdc.mem_counter_inc = vdc.screen_text_cols;
    if (vdc.raster.ycounter >= vdc.raster_ycounter_max) {
        vdc.mem_counter += vdc.mem_counter_inc + vdc.regs[27];
    }
    vdc.raster.ycounter = (vdc.raster.ycounter + 1)
                          % (vdc.raster_ycounter_max + 1);
}
#endif

static void vdc_set_video_mode(void)
{
    vdc.raster.video_mode = (vdc.regs[25] & 0x80)
                            ? VDC_BITMAP_MODE : VDC_TEXT_MODE;

    if (vdc.draw_counter_y > (unsigned int)(vdc.regs[9] & 0x1f)) {
        vdc.raster.video_mode = VDC_IDLE_MODE;
    }
}


/* Redraw the current raster line. */
/* This was mostly re-written from scratch in May-June 2019 by Strobe to develop,
 and then correctly emulate the VDC101 demo which uses a new raster split technique
 that forces the VDC to display multiple frames as if they are one frame.
 The old code assumed a mostly static screen and couldn't cope.
 The new code is intended to function a bit more like the VDC does internally
 (or at least how we think it does).. */
static void vdc_raster_draw_alarm_handler(CLOCK offset, void *data)
{
    static unsigned int vdc_row_counter_latch = 0;
    static unsigned int vdc_draw_counter_latch = 0;
    static unsigned int vdc_vert_fine_adj = 0;

    /*  Video signal handling section ----------------------------------------------------------------------------------------------------------*/
    if (vdc_row_counter_latch) {    /* latch is set if the previous raster line was the last of its character row */
        /* We've just drawn the last raster line of the current character row,
            so handle a new row from the video signal side:
            - restart internal counters if we've displayed a full frame
            - drawing starting e.g. pass top border
            - drawing stopping e.g. pass into bottom border
            - start or stop vsync pulse */
        vdc_row_counter_latch = 0;
        vdc.row_counter_y = vdc.interlaced & vdc.frame_counter;
        
        /* Update the row counter because we are starting a new line */
        vdc.row_counter++;
        /* FIXME: not clamping to 8bits here so the "> vdc.regs[4]" case below catches reg#4=$FF and still shows an image. */
        
        /* FIXME this seems a bit messy and/or is in the wrong spot */
        if (vdc.row_counter == 1) { /* we think anything visible starts on internal row 1 */
            vdc.display_enable = 1;
        }
        
        /* check if we are at the end of the active area, e.g. after last visible row, normally #25 */
        if (vdc.row_counter > vdc.regs[6])  {
            vdc.display_enable = 0;
            /* FIXME handle prebuffering next line here and/or in draw section */
        }
        
        /* Check if we've hit past the last char row and we should restart
            FIXME comparison looks wrong, probably not a > ?
            FIXME Also this draws the actual number of rows in reg 4, which is 1 less than what MTC128 says it should be drawing
            (even though the timing is then correct for e.g. RFO..) */
        if (vdc.row_counter > (vdc.regs[4])) {
            /* FIXME handle vertical fine adjust (reg#5>0) a bit cleaner and handle edge cases */
            if (vdc_vert_fine_adj || vdc.regs[5] == 0 || vdc.regs[5] == vdc.regs[9]) {
                vdc_vert_fine_adj = 0;
                vdc.row_counter = 0;
                /* we reset the raster line counter to 0, except if we are interlaced (==1) and on an odd frame */
                vdc.row_counter_y = vdc.interlaced & vdc.frame_counter; 
                vdc.prime_draw = 1;
            
                /* FIXME fall through catch in case the attribute pointers didn't latch. Probably not exactly what the chip does... */
                if (!vdc.draw_finished) {
                    /* Reset address pointers */
                    vdc.screen_adr = ((vdc.regs[12] << 8) | vdc.regs[13])
                        & vdc.vdc_address_mask;
                    vdc.attribute_adr = ((vdc.regs[20] << 8) | vdc.regs[21])
                        & vdc.vdc_address_mask;
                    if (vdc.old_screen_adr != vdc.screen_adr || vdc.old_attribute_adr != vdc.attribute_adr) {
                        /* the cache can't cleanly handle these changing */
                        vdc.force_repaint = 1;
                        vdc.old_screen_adr = vdc.screen_adr;
                        vdc.old_attribute_adr = vdc.attribute_adr;
                    }
                    if (vdc.interlaced                  /* interlace and.. */
                        && (vdc.regs[25] & 0x80)        /* bitmap mode and.. */
                        && !(vdc.frame_counter & 1)) { /* even frame */
                        /* Do nothing because I can't be bothered reversing the above logic */
                    } else {    /* Reset all the internal VDC memory pointers and counters to 0 */
                        vdc.mem_counter = 0;
                        vdc.bitmap_counter = 0;
                        vdc.draw_active = 0;
                    }
                }
                vdc.draw_finished = 1;
            } else {    /* now in vertical adjust area where things work different */
                vdc_vert_fine_adj = 1;
            }
        }
        
        /* check if we've hit the vsync position and should start vsync */
        if (vdc.row_counter == vdc.regs[7]) {
            vdc.vsync = 1;
            vdc.vsync_counter = 0;
        }
    } else {
        vdc.row_counter_y += 1 + vdc.interlaced;
        vdc.row_counter_y &= 0x1F;
    }

    /* Check if this is the last raster line of the current row, and latch so */
    if (vdc_vert_fine_adj) {    /* vertical fine adjust area has a different comparison */
        if ((vdc.row_counter_y | vdc.interlaced) == (vdc.regs[5] & 0x1F)) {
            vdc_row_counter_latch = 1;
        }
    } else {    /* normal case, compare with reg #9 */
        if ((vdc.row_counter_y | vdc.interlaced) == (vdc.regs[9] & 0x1F)) {
            vdc_row_counter_latch = 1;
        }
    }
    
    /* Handle if we are in vertical sync pulse */
    if (vdc.vsync) {
        vdc.vsync_counter++;
        /* Check if we are now out of the pulse == at first visible raster line, and reset the raster to the top of the screen if so */
        if (vdc.vsync_counter > 25) {   /* 25 seems to be about right # of raster lines the vsync consumes on a C= monitor, and is official PAL spec */
            vdc.vsync = 0;
            
            /* This SEEMS to work to reset the raster to 0, based on ted.c, but maybe there is something else needed?
                FIXME handle cleanup of remainder of visible raster lines below the reset point somewhere somehow */
            vdc.raster.current_line = 0;
            raster_canvas_handle_end_of_frame(&vdc.raster);
        
            vdc.frame_counter++;    /* As far as the frame counter is concerned, we are now on a new frame */
            if (vdc.regs[24] & 0x20) {
                vdc.attribute_blink = vdc.frame_counter & 16;
            } else {
                vdc.attribute_blink = vdc.frame_counter & 8;
            }
            
            /* FIXME I doubt all this is required any more: */
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
            /* The 'are we drawing or not' section further down handles restarting
                drawing if we're past the sync but we should still be drawing */
        }
    }
    /* END Video signal handling section ----------------------------------------------------------------------------------------------------------*/


    /* This is where previous builds of vice latched in the screen & attribute addresses,
        i.e. not the first raster line after the last active row, but the 2nd
        FIXME - it doesn't seem quite 'right' but it works.
        e.g. changing it to the first raster line with row_counter_y == 0 breaks RFO logo swinger and techtech */
    if ((vdc.row_counter == vdc.regs[6] + 1) && ((vdc.row_counter_y == 1 ) || (vdc.interlaced && vdc.row_counter_y == 2))) {
        vdc.draw_active = 0;
        vdc.draw_finished = 1;
        
        /* Reset address pointers */
        vdc.screen_adr = ((vdc.regs[12] << 8) | vdc.regs[13])
                         & vdc.vdc_address_mask;
        vdc.attribute_adr = ((vdc.regs[20] << 8) | vdc.regs[21])
                            & vdc.vdc_address_mask;
        if (vdc.old_screen_adr != vdc.screen_adr || vdc.old_attribute_adr != vdc.attribute_adr) {
            /* the cache can't cleanly handle these changing */
            vdc.force_repaint = 1;
            vdc.old_screen_adr = vdc.screen_adr;
            vdc.old_attribute_adr = vdc.attribute_adr;
        }
        
        if (vdc.interlaced                  /* interlace and.. */
            && (vdc.regs[25] & 0x80)        /* bitmap mode and.. */
            && !(vdc.frame_counter & 1)) { /* even frame */
            /* Do nothing because I can't be bothered reversing the above logic */
        } else {    /* Reset all the internal VDC memory pointers and counters to 0 */
            vdc.mem_counter = 0;
            vdc.bitmap_counter = 0;
            vdc.draw_active = 0;
        }
    }


    /*  START drawing section ----------------------------------------------------------------------------------------------------------------*/
    /* Handle the beginning of the drawing process/set up, on/after internal row # 0 */
    if (vdc.prime_draw) {
        if ((vdc.row_counter == 0) && (vdc.row_counter_y == (vdc.interlaced & vdc.frame_counter))) {
            /* set up for smooth scroll comparison */
            vdc.draw_counter_y = (vdc.regs[24] & 0x1F);     /* this could be moved up into the screen handling section? */
            vdc.draw_counter = 0;
            vdc_draw_counter_latch = 0;
            if (vdc.interlaced & vdc.frame_counter) {
                vdc.draw_counter_y = (vdc.draw_counter_y + 1)
                          % ((vdc.regs[9] & 0x1F) + 1); /* FIXME this is overly complex and definitely not what the chip does internally.. */
                vdc.bitmap_counter += vdc.mem_counter_inc + vdc.regs[27];
                vdc.bitmap_counter &= vdc.vdc_address_mask;
            }
        } else {
            if ((vdc.draw_counter_y | vdc.interlaced) == ((vdc.regs[9] & 0x1F) | vdc.interlaced)) {
                /* we latch on now and start drawing on the next call, even if that's invisible because it's above the top border */
                
                /* we are trying to keep the bottom bit the same for the next character row, but only if interlaced */
                vdc.draw_counter_y &= vdc.interlaced;
                vdc.draw_counter++;
                vdc.prime_draw = 0;
                vdc.draw_active = 1;
                vdc.draw_finished = 0;
                if (vdc.interlaced                  /* interlace and.. */
                        && (vdc.regs[25] & 0x80)    /* bitmap mode and.. */
                        && !(vdc.frame_counter & 1)) { /* even frame */
                    vdc.mem_counter_inc = vdc.screen_text_cols;
                    vdc.mem_counter += vdc.mem_counter_inc + vdc.regs[27];
                    vdc.mem_counter &= vdc.vdc_address_mask;
                }
            } else {
                vdc.draw_counter_y += 1 + vdc.interlaced;
                vdc.draw_counter_y &= 0x1F;
                if (vdc.interlaced) {
                    vdc.bitmap_counter += vdc.mem_counter_inc + vdc.regs[27];
                    vdc.bitmap_counter &= vdc.vdc_address_mask;
                }
            }
        }
    
    /* Handle the normal drawing case */
    } else if (vdc.draw_active) {
        if (vdc_draw_counter_latch) {    /* latch is set if the previous raster line was the last of its character row */
            vdc.draw_counter_y &= vdc.interlaced;
            vdc.draw_counter++;
            vdc_draw_counter_latch = 0;
            
            /* FIXME We've just drawn the last raster line of the current row, should probably load those buffers or so.. */
            
            /* increment memory pointers etc. for a new character row */
            vdc.mem_counter_inc = vdc.screen_text_cols;
            vdc.mem_counter += vdc.mem_counter_inc + vdc.regs[27];
            vdc.mem_counter &= vdc.vdc_address_mask;
            vdc.bitmap_counter += vdc.mem_counter_inc + vdc.regs[27];
            vdc.bitmap_counter &= vdc.vdc_address_mask;
        } else {
            /* increment memory pointers etc. for a new raster line */
            vdc.draw_counter_y += 1 + vdc.interlaced;
            vdc.draw_counter_y &= 0x1F;
            vdc.bitmap_counter += vdc.mem_counter_inc + vdc.regs[27];
            vdc.bitmap_counter &= vdc.vdc_address_mask;
        }
        if ((vdc.draw_counter_y | vdc.interlaced) == (vdc.regs[9] & 0x1F)) {
            vdc_draw_counter_latch = 1;
        }
    } else {
        /* FIXME handle the last visible(ish) row situation better
           Implied: vdc.prime_draw == 0 && vdc.draw_active == 0 */
        vdc_draw_counter_latch = 0;
    }
    vdc.raster.ycounter = vdc.draw_counter_y;   /* FIXME quick hack. maybe just use ycounter in the first place? Check side effects with memory inc functions */
    /*  END drawing section ----------------------------------------------------------------------------------------------------------------*/


    /* Decide if the current raster line is visible or not and set draw mode appropriately */
    if (vdc.vsync) {    /* no, we're in vsync */
        /*FIXME change this to always black or some other mode so it works more obviously that you're in vsync vs in the border and make sure it restarts drawing past the pulse */
        vdc.raster.video_mode = VDC_IDLE_MODE;
    } else if ( ((vdc.regs[25] & 0x0F) > ((vdc.regs[22] >> 4) & 0x0F)) ||    /* smooth scroll is > char with */
                ((vdc.regs[25] & 0x10) && ((vdc.regs[25] & 0x0F) == ((vdc.regs[22] >> 4) & 0x0F))) ) { /* or double-pixel/40-column mode and smooth scroll = char width */
        /*FIXME technically this should be black in the foreground area (border remains normal), not just border colour all the way across */
        vdc.raster.video_mode = VDC_IDLE_MODE;
    } else if ( ((vdc.regs[34] > vdc.regs[0]) && (vdc.regs[35] <= vdc.regs[0])) ||      /* if #34> #0  and #35 <= #0, blank line, always */
                ((vdc.regs[34] == vdc.regs[35]) && (vdc.regs[35] <= vdc.regs[0])) ) {   /* if #34==#35 and both <= #0, blank line always */
                /* || ((vdc.regs[34] <= vdc.regs[35]) && (vdc.regs[35] <= vdc.regs[0])) ) {*/ /* FIXME if #34<=#35 and both <= #0, blank line sometimes, depends on values relative to display area */
        /*FIXME technically this should be black across the entire screen, not just border colour */
        vdc.raster.video_mode = VDC_IDLE_MODE;
    } else if (vdc.draw_active && vdc.display_enable) {
        vdc_set_video_mode();   /* show stuff */
    } else {
        vdc.raster.video_mode = VDC_IDLE_MODE;  /* fall through for remaining combinations */
    }


    /* FIXME this is a hack to get the "background" between the "left and right" top & bottom border areas actually drawn when the cache is on.
        Strongly suspect raster code is broken at this point, or at least confused, as it always draws black even though the colours are set.. */
    if (vdc.raster.cache_enabled && !vdc.display_enable) {
        vdc.raster.blank_this_line = 1;
    }


    /* actually draw the current raster line */
    raster_line_emulate(&vdc.raster);


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
