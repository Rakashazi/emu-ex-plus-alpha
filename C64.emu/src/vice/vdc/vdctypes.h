/*
 * vdctypes.h - MOS8563 (VDC) emulation.
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

#ifndef VICE_VDCTYPES_H
#define VICE_VDCTYPES_H

#include "raster.h"
#include "types.h"

/* Screen constants.  */
/* Double pixelsize in y-resolution, to allow for scanlines */
/*  and interlace effects                                   */

/* Not exact, but for now allow 16 pixels of border each    */

#define VDC_DOT_CLOCK 16000000.0

#define VDC_SCREEN_WIDTH              856 /* Approx resolution based on experiment - real vdc shows ~107 columns * 8 = 856 */
#define VDC_SCREEN_HEIGHT             312 /* Vertical resolution (PAL) based on default kernal settings */

#define VDC_SCREEN_XPIX               800
#define VDC_SCREEN_YPIX               200
#define VDC_SCREEN_MAX_TEXTCOLS       100
#define VDC_SCREEN_BORDERWIDTH        8
#define VDC_SCREEN_BORDERHEIGHT       0

#define VDC_FIRST_DISPLAYED_LINE      21
#define VDC_LAST_DISPLAYED_LINE       308
#define VDC_80COL_START_PIXEL         16

#define VDC_NUM_SPRITES               0
#define VDC_NUM_COLORS                16


/* VDC Attribute masks */
#define VDC_FLASH_ATTR              0x10
#define VDC_UNDERLINE_ATTR          0x20
#define VDC_REVERSE_ATTR            0x40
#define VDC_ALTCHARSET_ATTR         0x80

/* Available video modes. */
enum vdc_video_mode_s {
    VDC_TEXT_MODE,
    VDC_BITMAP_MODE,
    VDC_IDLE_MODE,
    VDC_NUM_VMODES
};
typedef enum vdc_video_mode_s vdc_video_mode_t;

#define VDC_IS_ILLEGAL_MODE(x)  ((x) >= VDC_ILLEGAL_TEXT_MODE && (x) != VDC_IDLE_MODE)
#define VDC_IS_BITMAP_MODE(x)   ((x) & 0x02)

/* VDC structures.  This is meant to be used by VDC modules
   *exclusively*!  */

struct vdc_light_pen_s {
    int triggered;
    int x, y;
};
typedef struct vdc_light_pen_s vdc_light_pen_t;

struct alarm_s;
struct video_chip_cap_s;

struct vdc_s {
    /* Flag: Are we initialized?  */
    int initialized;            /* = 0; */

    /* VDC registers.  */
    uint8_t regs[64];

    /* VDC geometry constants that differ in double size mode.  */
    unsigned int screen_height;
    unsigned int screen_xpix;
    unsigned int screen_ypix;
    unsigned int first_displayed_line;
    unsigned int last_displayed_line;
    unsigned int border_height;
    unsigned int border_width;
    unsigned int raster_ycounter_max;
    unsigned int screen_textlines;

    /* Additional left shift.  */
    unsigned int hsync_shift;

    /* Number of chars per line (including blank and sync).  */
    unsigned int xchars_total;

    /* VDC to host processor synchronization.  */
    unsigned int xsync_increment;

    /* Internal VDC register pointer */
    int update_reg;

    /* Number of text characters displayed.  */
    unsigned int screen_text_cols;

    /* Video memory offsets.  */
    unsigned int screen_adr;
    unsigned int cursor_adr;
    unsigned int update_adr;
    unsigned int attribute_adr;
    unsigned int chargen_adr;

    /* Internal memory counter. */
    unsigned int mem_counter;
    unsigned int bitmap_counter;

    /* Bytes per character.  */
    unsigned int bytes_per_char;

    /* Character width - width of each character on screen in physical pixels */
    unsigned int charwidth;

    /* Value to add to `mem_counter' after the graphics has been painted.  */
    unsigned int mem_counter_inc;   /* FIXME - always the same as screen_text_cols! */
    unsigned int skip_after_line;   /* derived from reg27 */

    /* All the VDC logging goes here.  */
    signed int log;

    /* VDC alarms.  */
    /* Alarm to update a raster line. */
    struct alarm_s *raster_draw_alarm;

    /* Memory address mask.  */
    int vdc_address_mask;

    /* Frame counter (required for character blink, cursor blink and interlace) */
    int frame_counter;

    /* Character attribute blink */
    int attribute_blink;

    /* Cursor position.  */
    int crsrpos;

    /* Repaint the whole screen next frame.  */
    int force_repaint;

    /* Resize geometry next frame.  */
    int force_resize;

    /* Flush the cache next frame.  */
    int force_cache_flush;

    /* The screen geometry has changed.  */
    int update_geometry;

    /* 0..7 pixel x shift.  */
    unsigned int xsmooth;

    /* VDC Revision.  */
    unsigned int revision;

    /* VDC raster.  */
    raster_t raster;

    /* Video chip capabilities.  */
    struct video_chip_cap_s *video_chip_cap;

    /* Internal VDC video memory */
    uint8_t ram[0x10000];

    /* used to record the value of the cpu clock at the start of a raster line */
    CLOCK vdc_line_start;
    /* based on blacky_stardust calculations, calculating current_x_pixel should be like:
    current_x_pixel = pixels_per_line / (vdc.xsync_increment >> 16) * (current_cycle - vdc_line_start) */

    /* record register 27 in case of a change between raster updates */
    /* FIXME - never used! */
    int old_reg27;

    /* Row counter (required for comparison with reg[6] - number of visible screen rows - to know if we are at the end of the visible data) */
    unsigned int row_counter;

    /* Row counter_y counts individual raster lines of the current row to know if we are at the end of the current row. */
    unsigned int row_counter_y;

    /* offset into the attribute memory - used for emulating the 8x1 attribute VDC quirk */
    unsigned int attribute_offset;

    /* flag as to whether the VDC is actually rendering active raster lines, or is idle (i.e in the top or bottom border) */
    unsigned int display_enable;

    /* flag as to whether the VDC is preparing to draw but hasn't actually started yet, usually when it's reset to internal row # 0 */
    unsigned int prime_draw;

    /* flag as to whether the VDC is drawing (e.g. reading from screen memory) or if it is stopped. This is different to above, because the VDC can be drawing inside the top or bottom border */
    unsigned int draw_active;

    /* flag as to whether the VDC is finished drawing or not, to make sure end of frame stuff like video pointers is handled properly */
    unsigned int draw_finished;

    /* flag as to whether the VDC is in VSYNC or not */
    unsigned int vsync;

    /* internal VDC counter so the vsync is the correct height (in rows) */
    unsigned int vsync_counter;

    /* height of the vsync pulse in raster lines, this varies by video standard/video mode */
    unsigned int vsync_height;

    /* Row counter used by drawing code. Separate from row_counter above because the drawing can be asynch with the main video signal */
    unsigned int draw_counter;

    /* Raster line counter used by drawing code as above. */
    unsigned int draw_counter_y;

    /* used to monitor for changes in screen and/or attribute addresses, as the cache can't cope with that */
    unsigned int old_screen_adr, old_attribute_adr;

    /* Interlace flag - 0 if normal/non-interlaced, 1 if interlaced */
    unsigned int interlaced;

    /* Light pen. */
    vdc_light_pen_t light_pen;

    /* Size of vdc canvas, i.e. total visible screen area in pixels */
    unsigned int canvas_width;
    unsigned int canvas_height;

    /* To compare if it changes since last frame */
    unsigned int canvas_width_old;
    unsigned int canvas_height_old;

    /* Internal character and attribute buffers */
    uint8_t scrnbuf[0x200];
    unsigned int scrnbufdraw;
    uint8_t attrbuf[0x200];
    unsigned int attrbufdraw;
};
typedef struct vdc_s vdc_t;

extern vdc_t vdc;

/* Private function calls, used by the other VDC modules.  */
int vdc_load_palette(const char *name);
void vdc_fetch_matrix(int offs, int num);
void vdc_update_memory_ptrs(unsigned int cycle);
void vdc_update_video_mode(unsigned int cycle);
void vdc_set_set_canvas_refresh(int enable);
void vdc_calculate_xsync(void);

#endif
