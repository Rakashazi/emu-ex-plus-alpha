/*
 * viciitypes.h - A cycle-exact event-driven MOS6569 (VIC-II) emulation.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *  Daniel Kahlin <daniel@kahlin.net>
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

#ifndef VICE_VICIITYPES_H
#define VICE_VICIITYPES_H

#include "raster.h"
#include "types.h"

/* Screen constants.  */
#define VICII_SCREEN_XPIX                  320
#define VICII_SCREEN_YPIX                  200
#define VICII_SCREEN_TEXTCOLS              40
#define VICII_SCREEN_TEXTLINES             25
#define VICII_SCREEN_CHARHEIGHT            8

#define VICII_NUM_SPRITES      8
#define VICII_NUM_COLORS       16

/* This macro translated PAL cycles 1 to 63 into our internal
   representation, i.e 0-63. */
#define VICII_PAL_CYCLE(c) ((c) - 1)

/* Common parameters for all video standards */
#define VICII_25ROW_START_LINE    0x33
#define VICII_25ROW_STOP_LINE     0xfb
#define VICII_24ROW_START_LINE    0x37
#define VICII_24ROW_STOP_LINE     0xf7

/* Bad line range.  */
#define VICII_FIRST_DMA_LINE      0x30
#define VICII_LAST_DMA_LINE       0xf7

/* drawing constants. */
#define VICII_DRAW_BUFFER_SIZE (65 * 8)

/* just a dummy for the vicii-draw.c wrapper */
#define VICII_DUMMY_MODE (0)


/* VIC-II structures.  This is meant to be used by VIC-II modules
   *exclusively*!  */

struct vicii_light_pen_s {
    int state;
    int triggered;
    int x, y, x_extra_bits;
    CLOCK trigger_cycle;
};
typedef struct vicii_light_pen_s vicii_light_pen_t;

struct vicii_sprite_s {
    /* Sprite data to display */
    DWORD data;
    /* 6 bit counters */
    BYTE mc;
    BYTE mcbase;
    /* 8 bit pointer */
    BYTE pointer;
    /* Expansion flop */
    int exp_flop;
    /* X coordinate */
    int x;
};
typedef struct vicii_sprite_s vicii_sprite_t;

struct video_chip_cap_s;

struct vicii_s {
    /* Flag: Are we initialized?  */
    int initialized;            /* = 0; */

    /* VIC-II raster.  */
    raster_t raster;

    /* VIC-II registers.  */
    BYTE regs[0x40];

    /* Cycle # within the current line.  */
    unsigned int raster_cycle;

    /* Cycle flags for the cycle table */
    unsigned int cycle_flags;

    /* Current line.  */
    unsigned int raster_line;

    /* Start of frame flag.  */
    int start_of_frame;

    /* Interrupt register.  */
    int irq_status;             /* = 0; */

    /* Line for raster compare IRQ.  */
    unsigned int raster_irq_line;

    /* Flag for raster compare edge detect.  */
    int raster_irq_triggered;

    /* Pointer to the base of RAM seen by the VIC-II.  */
    /* address is base of 64k bank. vbank adds 0/16k/32k/48k to get actual
       video address */
    BYTE *ram_base_phi1;                /* = VIC-II address during Phi1; */
    BYTE *ram_base_phi2;                /* = VIC-II address during Phi2; */

    /* valid VIC-II address bits for Phi1 and Phi2. After masking
       the address, it is or'd with the offset value to set always-1 bits */
    WORD vaddr_mask_phi1;            /* mask of valid address bits */
    WORD vaddr_mask_phi2;            /* mask of valid address bits */
    WORD vaddr_offset_phi1;          /* mask of address bits always set */
    WORD vaddr_offset_phi2;          /* mask of address bits always set */

    /* Those two values determine where in the address space the chargen
       ROM is mapped. Use mask=0x7000, value=0x1000 for the C64. */
    WORD vaddr_chargen_mask_phi1;    /* address bits to comp. for chargen */
    WORD vaddr_chargen_mask_phi2;    /* address bits to comp. for chargen */
    WORD vaddr_chargen_value_phi1;   /* compare value for chargen */
    WORD vaddr_chargen_value_phi2;   /* compare value for chargen */

    /* Screen memory buffers (chars and color).  */
    BYTE vbuf[VICII_SCREEN_TEXTCOLS];
    BYTE cbuf[VICII_SCREEN_TEXTCOLS];

    /* Graphics buffer (bitmap/LinearB) */
    BYTE gbuf;

    /* Current rendering position into the draw buffer */
    int dbuf_offset;

    /* Draw buffer for a full line (one byte per pixel) */
    BYTE dbuf[VICII_DRAW_BUFFER_SIZE];

    /* parsed vicii register fields */
    unsigned int ysmooth;

    /* If this flag is set, bad lines (DMA's) can happen.  */
    int allow_bad_lines;

    /* Sprite-sprite and sprite-background collision registers.  */
    BYTE sprite_sprite_collisions;
    BYTE sprite_background_collisions;

    /* flag to signal collision clearing */
    BYTE clear_collisions;

    /* Flag: are we in idle state? */
    int idle_state;

    /* Internal memory pointer (VCBASE).  */
    int vcbase;

    /* Internal memory counter (VC).  */
    int vc;

    /* Internal row counter (RC).  */
    int rc;

    /* Offset to the vbuf/cbuf buffer (VMLI) */
    int vmli;

    /* Flag: is the current line a `bad' line? */
    int bad_line;

    /* Light pen.  */
    vicii_light_pen_t light_pen;

    /* Start of the memory bank seen by the VIC-II.  */
    int vbank_phi1;                     /* = 0; */
    int vbank_phi2;                     /* = 0; */

    /* All the VIC-II logging goes here.  */
    signed int log;

    /* Delayed mode selection */
    BYTE reg11_delay;

    /* Fetch state */
    int prefetch_cycles;

    /* Mask for sprites being displayed.  */
    unsigned int sprite_display_bits;

    /* Flag: is sprite DMA active? */
    BYTE sprite_dma;

    /* State of sprites. */
    vicii_sprite_t sprite[VICII_NUM_SPRITES];

    /* Geometry and timing parameters of the selected VIC-II emulation.  */
    unsigned int screen_height;
    int first_displayed_line;
    int last_displayed_line;

    int screen_leftborderwidth;
    int screen_rightborderwidth;

    /* parameters (set by vicii-chip-model). */
    int cycles_per_line;
    int color_latency;
    int lightpen_old_irq_mode;

    /* cycle table (set by vicii-chip-model). */
    unsigned int cycle_table[65];

    /* last color register update (set by vicii-mem.c,
       cleared by vicii-draw-cycle.c */
    BYTE last_color_reg;
    BYTE last_color_value;

    /* Last value read by VICII during phi1.  */
    BYTE last_read_phi1;

    /* Last value on the internal VICII bus during phi2.  */
    BYTE last_bus_phi2;

    /* Vertical border flag */
    int vborder;

    /* latched set of Vertical border flag */
    int set_vborder;

    /* Main border flag (this is what controls rendering) */
    int main_border;

    /* Counter used for DRAM refresh accesses.  */
    BYTE refresh_counter;

    /* Video chip capabilities.  */
    struct video_chip_cap_s *video_chip_cap;

    unsigned int int_num;
};
typedef struct vicii_s vicii_t;

extern vicii_t vicii;

/* Private function calls, used by the other VIC-II modules.  */
extern void vicii_raster_draw_handler(void);

/* Debugging options.  */

/* #define VICII_VMODE_DEBUG */
/* #define VICII_RASTER_DEBUG */
/* #define VICII_REGISTERS_DEBUG */
/* #define VICII_CYCLE_DEBUG */

#ifdef VICII_VMODE_DEBUG
#define VICII_DEBUG_VMODE(x) log_debug x
#else
#define VICII_DEBUG_VMODE(x)
#endif

#ifdef VICII_RASTER_DEBUG
#define VICII_DEBUG_RASTER(x) log_debug x
#else
#define VICII_DEBUG_RASTER(x)
#endif

#ifdef VICII_REGISTERS_DEBUG
#define VICII_DEBUG_REGISTER(x) log_debug x
#else
#define VICII_DEBUG_REGISTER(x)
#endif

#ifdef VICII_CYCLE_DEBUG
#define VICII_DEBUG_CYCLE(x) log_debug x
#else
#define VICII_DEBUG_CYCLE(x)
#endif

#endif
