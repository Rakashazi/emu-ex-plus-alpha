/*
 * vdctypes.h - MOS8563 (VDC) emulation.
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

#define VDC_REVISION_0  0 /* 8563 R7A */
#define VDC_REVISION_1  1 /* 8563 R8/R9 */
#define VDC_REVISION_2  2 /* 8568 */

#define VDC_NUM_REVISIONS 3

struct vdc_s {
    /* Flag: Are we initialized?  */
    int initialized;            /* = 0; */

    /* VDC registers.  */
    BYTE regs[64];

    /* VDC geometry constants that differ in doulbe size mode.  */
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

    /* Value to add to `mem_counter' after the graphics has been painted.  */
    unsigned int mem_counter_inc;

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
    BYTE ram[0x10000];

    /* used to record the value of the cpu clock at the start of a raster line */
    CLOCK vdc_line_start;
    /* based on blacky_stardust calculations, calculating current_x_pixel should be like:
    current_x_pixel = pixels_per_line / (vdc.xsync_increment >> 16) * (current_cycle - vdc_line_start) */

    /* record register 27 in case of a change between raster updates */
    int old_reg27;

    /* Row counter (required for comparison with reg[6] - number of visible screen rows - to know if we are at the end of the visible data) */
    unsigned int row_counter;

    /* Row counter_y counts individual raster lines of the current row to know if we are at the end of the current row. */
    int row_counter_y;

    /* offset into the attribute memory - used for emulating the 8x1 attribute VDC quirk */
    unsigned int attribute_offset;

    /* Light pen. */
    vdc_light_pen_t light_pen;
};
typedef struct vdc_s vdc_t;

extern vdc_t vdc;

/* Private function calls, used by the other VDC modules.  */
extern int vdc_load_palette(const char *name);
extern void vdc_fetch_matrix(int offs, int num);
extern void vdc_update_memory_ptrs(unsigned int cycle);
extern void vdc_update_video_mode(unsigned int cycle);
extern void vdc_set_set_canvas_refresh(int enable);
extern void vdc_calculate_xsync(void);

#endif
