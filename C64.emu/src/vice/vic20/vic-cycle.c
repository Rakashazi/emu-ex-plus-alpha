/*
 * vic-cycle.c - Cycle based VIC-I emulation.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Matthies <andreas.matthies@gmx.net>
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

#include "vice.h"

#include "maincpu.h"
#include "mem.h"
#include "raster.h"
#include "types.h"
#include "vic.h"
#include "victypes.h"
#include "vic20-resources.h"
#include "vic20mem.h"
#include "vic20memrom.h"
#include "vic20.h"
#include "viewport.h"

/* ------------------------------------------------------------------------- */

/* Close vertical flipflop */
static inline void vic_cycle_close_v(void)
{
    vic.area = VIC_AREA_DONE;
    vic.raster.display_ystop = vic.raster_line;

    /* Display one more line if h-flipflop is already open (XPOS == 0) */
    if (vic.fetch_state != VIC_FETCH_IDLE) {
        vic.raster.display_ystop++;
    }
}

/* Open vertical flipflop */
static inline void vic_cycle_open_v(void)
{
    vic.area = VIC_AREA_PENDING;
    vic.raster.display_ystart = vic.raster_line;
    vic.raster.geometry->gfx_position.y = vic.raster_line;

    if (vic.text_lines == 0) {
        vic_cycle_close_v();
    } else {
        if (vic.screen_height == VIC20_NTSC_SCREEN_LINES) {
            vic.raster.display_ystop = vic.screen_height;
        } else {
            vic.raster.display_ystop = vic.screen_height - 1;
        }
    }
}

/* Open horizontal flipflop */
static inline void vic_cycle_open_h(void)
{
    int xstart;

    xstart = MIN((unsigned int)(vic.raster_cycle * 4), vic.screen_width);
    xstart *= VIC_PIXEL_WIDTH;

    vic.raster.display_xstart = xstart;
    vic.raster.geometry->gfx_position.x = xstart;

    vic.fetch_state = VIC_FETCH_START;
    /* buf_offset used as delay counter before first matrix fetch */
    /* TODO why is this needed? */
    vic.buf_offset = 4;

    if (vic.area == VIC_AREA_PENDING) {
        vic.raster.display_ystart = vic.raster_line;
        vic.raster.geometry->gfx_position.y = vic.raster_line;
        vic.area = VIC_AREA_DISPLAY;
    }

    vic.memptr_inc = 0;

    /* Update text cols early. This is needed to handle
       memptr updates properly when XPOS == 0 and COLS == 0
       is used to avoid memptr increases. */
    vic.text_cols = vic.pending_text_cols;
}

/* Close horizontal flipflop */
static inline void vic_cycle_close_h(void)
{
    vic.fetch_state = VIC_FETCH_DONE;
}

/* Start the actual fetch */
static inline void vic_cycle_start_fetch(void)
{
    int xstop;

    vic.text_cols = vic.pending_text_cols;
    vic.raster.geometry->gfx_size.width = vic.text_cols * 8 * VIC_PIXEL_WIDTH;
    vic.raster.geometry->text_size.width = vic.text_cols;

    xstop = vic.raster.display_xstart / VIC_PIXEL_WIDTH + vic.text_cols * 8;

    if (xstop >= (int)vic.screen_width) {
        xstop = vic.screen_width - 1;
        /* FIXME: SCREEN-MIXUP not handled */
    }

    xstop *= VIC_PIXEL_WIDTH;

    vic.raster.display_xstop = xstop;

    if (vic.text_cols > 0) {
        vic.raster.blank_this_line = 0;
        vic.fetch_state = VIC_FETCH_MATRIX;
    } else {
        vic_cycle_close_h();
    }
}

/* Handle end of line */
static inline void vic_cycle_end_of_line(void)
{
    vic.line_was_blank = vic.raster.blank_this_line;

    vic.raster_cycle = 0;
    vic_raster_draw_handler();

    if (vic.area == VIC_AREA_DISPLAY) {
        vic.raster.ycounter++;
    }

    vic.fetch_state = VIC_FETCH_IDLE;
    vic.raster.blank_this_line = 1;
    vic.raster_line++;
}

/* Handle end of frame */
static inline void vic_cycle_end_of_frame(void)
{
    /* Close v-flipflop on end of frame */
    if (vic.area != VIC_AREA_DONE) {
        vic_cycle_close_v();
        vic.raster.display_ystop = vic.raster_line - 1;
    }

    vic.raster.blank_enabled = 1;
    vic.raster.blank = 0;
    vic.row_counter = 0;
    vic.raster_line = 0;
    vic.area = VIC_AREA_IDLE;
    vic.raster.display_ystart = -1;
    vic.raster.display_ystop = -1;
    vic.raster.ycounter = 0;
    vic.memptr = 0;
    vic.memptr_inc = 0;
}

/* Handle memptr increase */
static inline void vic_cycle_handle_memptr(void)
{
    /* check if row step is pending */
    if (vic.row_increase_line == (unsigned int)vic.raster.ycounter
        || 2 * vic.row_increase_line == (unsigned int)vic.raster.ycounter) {
        vic.raster.ycounter = 0;

        vic.memptr_inc = vic.line_was_blank ? 0 : vic.text_cols;

        vic.row_counter++;
        if (vic.row_counter == vic.text_lines) {
            vic_cycle_close_v();
        }
    }

    vic.memptr += vic.memptr_inc;
    vic.memptr_inc = 0;
}

/* Latch number of columns */
static inline void vic_cycle_latch_columns(void)
{
    vic.pending_text_cols = MIN(vic.regs[2] & 0x7f, (int)vic.max_text_cols);
}

/* Latch number of rows */
static inline void vic_cycle_latch_rows(void)
{
    int new_text_lines = (vic.regs[3] & 0x7e) >> 1;
    vic.text_lines = new_text_lines;
    vic.raster.geometry->gfx_size.height = new_text_lines * 8;
    vic.raster.geometry->text_size.height = new_text_lines;
}

/* ------------------------------------------------------------------------- */
/* Fetch hendling */

extern int vic20_vflihack_userport;
extern unsigned char vfli_ram[0x4000];

/* Perform actual fetch */
static inline BYTE vic_cycle_do_fetch(int addr, BYTE *color)
{
    BYTE b, c;
    int color_addr = 0x9400 + (addr & 0x3ff);
    int color_addr2 = (addr & 0x03ff) | (vic20_vflihack_userport << 10);

    if ((addr & 0x9000) == 0x8000) {
        /* chargen */
        b = vic20memrom_chargen_rom[addr & 0xfff];
        c = vflimod_enabled ? vfli_ram[color_addr2] : mem_ram[color_addr];
    } else if ((addr < 0x0400) || ((addr >= 0x1000) && (addr < 0x2000))) {
        /* RAM */
        b = mem_ram[addr];
        c = vflimod_enabled ? vfli_ram[color_addr2] : mem_ram[color_addr];
    } else if (vflimod_enabled && (addr >= 0x0400) && (addr < 0x1000)) {
        /* only for mike's VFLI hack */
        /* RAM */
        b = mem_ram[addr];
        c = vfli_ram[color_addr2];
    } else if (addr >= 0x9400 && addr < 0x9800) {
        /* color RAM */
        b = vflimod_enabled ? vfli_ram[color_addr2] : mem_ram[color_addr];
        c = b; /* FIXME is this correct? */
    } else {
        /* unconnected */
        b = vic20_v_bus_last_data & (0xf0 | vic20_v_bus_last_high);
        c = vflimod_enabled ? vfli_ram[color_addr2] : mem_ram[color_addr]; /* FIXME: is this correct? */
    }
    *color = vic20_v_bus_last_high = c;
    vic20_v_bus_last_data = b;

    return b;
}

/* Perform no fetch */
static inline void vic_cycle_no_fetch(void)
{
    /* TODO: vic20_v_bus_last_high = ? */
}

/* Make a "real" 16b address from the 14b VIC address */
static inline int vic_cycle_fix_addr(int addr)
{
    int msb = ~((addr & 0x2000) << 2) & 0x8000;
    return (addr & 0x1fff) | msb;
}

/* Fetch handler */
static inline void vic_cycle_fetch(void)
{
    int addr;
    BYTE b;

    switch (vic.fetch_state) {
        /* fetch has not started yet */
        case VIC_FETCH_IDLE:
        /* fetch done on current line */
        case VIC_FETCH_DONE:
        default:
            vic_cycle_no_fetch();
            break;

        /* fetch starting */
        case VIC_FETCH_START:
            if ((--vic.buf_offset) == 0) {
                vic_cycle_start_fetch();
            }
            vic_cycle_no_fetch();
            break;

        /* fetch from screen/color memomy */
        case VIC_FETCH_MATRIX:
            addr = (((vic.regs[5] & 0xf0) << 6) | ((vic.regs[2] & 0x80) << 2))+ ((vic.memptr + vic.buf_offset));

            vic.vbuf = vic_cycle_do_fetch(vic_cycle_fix_addr(addr), &b);
            vic.cbuf[vic.buf_offset] = b;

            vic.fetch_state = VIC_FETCH_CHARGEN;
            break;

        /* fetch from chargen */
        case VIC_FETCH_CHARGEN:
            b = vic.vbuf;
            addr = ((vic.regs[5] & 0xf) << 10) + ((b * vic.char_height + (vic.raster.ycounter & ((vic.char_height >> 1) | 7))));

            vic.gbuf[vic.buf_offset] = vic_cycle_do_fetch(vic_cycle_fix_addr(addr), &b);

            vic.buf_offset++;

            if (vic.raster.ycounter == (vic.char_height - 1)) {
                /* TODO should this be vic.memptr_inc++ instead? */
                vic.memptr_inc = vic.buf_offset;
            }

            if (vic.buf_offset >= vic.text_cols) {
                vic_cycle_close_h();
            } else {
                vic.fetch_state = VIC_FETCH_MATRIX;
            }
            break;
    }
}

/* ------------------------------------------------------------------------- */

void vic_cycle(void)
{
    if (vic.area == VIC_AREA_IDLE) {
        /* Check for vertical flipflop */
        if (vic.regs[1] == (vic.raster_line >> 1)) {
            vic_cycle_open_v();
        }
    }

    /* Next cycle */
    vic.raster_cycle++;
    if (vic.raster_cycle == vic.cycles_per_line) {
        vic_cycle_end_of_line();
        if (vic.raster_line == vic.screen_height) {
            vic_cycle_end_of_frame();
        }
    }

    if ((vic.area == VIC_AREA_DISPLAY) || (vic.area == VIC_AREA_PENDING)) {
        /* Check for horizontal flipflop */
        if ((vic.fetch_state == VIC_FETCH_IDLE) && (vic.regs[0] & 0x7fu) == vic.raster_cycle) {
            vic_cycle_open_h();
        }

        /* Handle memptr */
        if ((vic.area == VIC_AREA_DISPLAY) && (vic.raster_cycle == 0)) {
            vic_cycle_handle_memptr();
        }
    }

    if (vic.raster_line == 0) {
        /* Retrigger light pen if line is still held low */
        if (vic.raster_cycle == 1) {
            vic.light_pen.triggered = 0;

            if (vic.light_pen.state) {
                vic.light_pen.trigger_cycle = maincpu_clk + 1;
            }
        }

        /* Latch number of rows */
        if (vic.raster_cycle == 2) {
            vic_cycle_latch_rows();
        }
    }

    /* Latch number of columns */
    if (vic.raster_cycle == 1) {
        vic_cycle_latch_columns();
    }

    /* trigger light pen if scheduled */
    if (vic.light_pen.trigger_cycle == maincpu_clk) {
        vic_trigger_light_pen_internal(0);
    }

    /* Perform fetch */
    vic_cycle_fetch();
}
