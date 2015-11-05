/*
 * vic-mem.c - Memory interface for the VIC-I emulation.
 *
 * Written by
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maincpu.h"
#include "raster-changes.h"
#include "types.h"
#include "vic.h"
#include "victypes.h"
#include "vic-mem.h"
#include "vic20mem.h"
#include "vic20sound.h"
#include "viewport.h"

#ifdef HAVE_MOUSE
#include "lightpen.h"
#include "mouse.h"
#endif

/* VIC access functions. */

void vic_store(WORD addr, BYTE value)
{
    addr &= 0xf;
    vic.regs[addr] = value;
    VIC_DEBUG_REGISTER (("VIC: write $90%02X, value = $%02X.", addr, value));

    switch (addr) {
#if 0
        /* handled in vic_cycle.c */
        case 0:                   /* $9000  Screen X Location. */
        /*
            VIC checks in cycle n for peek($9000)=n
            and in this case opens the horizontal flipflop
        */
        case 1:                   /* $9001  Screen Y Location. */
        /*
            VIC checks from cycle 1 of line r*2 to cycle 0 of line r*2+2
            if peek($9001)=r is true and in this case it opens the vertical
            flipflop
        */
        case 2:                   /* $9002  Columns Displayed. */
        case 5:                   /* $9005  Video and char matrix base
                                   address. */
        /* read-only registers */
        case 4:                   /* $9004  Raster line count -- read only. */
        case 6:                   /* $9006. */
        case 7:                   /* $9007  Light Pen X,Y. */
        case 8:                   /* $9008. */
        case 9:                   /* $9009  Paddle X,Y. */
#endif
        default:
            return;

        case 3:                   /* $9003  Rows Displayed, Character size . */
            {
                int new_char_height = (value & 0x1) ? 16 : 8;

                vic.row_increase_line = new_char_height;
                vic.char_height = new_char_height;
            }
            return;

        case 10:                  /* $900A  Bass Enable and Frequency. */
        case 11:                  /* $900B  Alto Enable and Frequency. */
        case 12:                  /* $900C  Soprano Enable and Frequency. */
        case 13:                  /* $900D  Noise Enable and Frequency. */
            vic_sound_store(addr, value);
            return;

        case 14:                  /* $900E  Auxiliary Colour, Master Volume. */
            /*
                changes of auxiliary color in cycle n is visible at pixel 4*(n-7)+1
            */
            {
                static int old_aux_color = -1;
                int new_aux_color = value >> 4;

                if (new_aux_color != old_aux_color) {
                    /* integer part */
                    raster_changes_foreground_add_int(&vic.raster,
                                                      VIC_RASTER_CHAR_INT(vic.raster_cycle + 1),
                                                      &vic.auxiliary_color,
                                                      new_aux_color);
                    /* fractional part (half chars) */
                    raster_changes_foreground_add_int(&vic.raster,
                                                      VIC_RASTER_CHAR_INT(vic.raster_cycle + 1),
                                                      &vic.half_char_flag,
                                                      VIC_RASTER_CHAR_FRAC(vic.raster_cycle + 1));
                    /* old_mc_auxilary_color is used by vic-draw.c to handle the
                       one hires vic pixel lateness of change */
                    raster_changes_foreground_add_int(&vic.raster,
                                                      VIC_RASTER_CHAR(vic.raster_cycle + 2),
                                                      &vic.old_auxiliary_color,
                                                      new_aux_color);

                    old_aux_color = new_aux_color;
                }
            }

            vic_sound_store(addr, value);
            return;

        case 15:                  /* $900F  Screen and Border Colors,
                                     Reverse Video. */
            /*
                changes of border/background in cycle n are visible at pixel
                4*(n-7)+1, changes of reverse mode at pixel 4*(n-7)+3.
            */
            {
                static int old_background_color = -1;
                static int old_border_color = -1;
                static int old_reverse = -1;
                int new_background_color, new_border_color, new_reverse;

                new_background_color = value >> 4;
                new_border_color = value & 0x7;
                new_reverse = ((value & 0x8) ? 0 : 1);

                if (new_background_color != old_background_color) {
                    raster_changes_background_add_int(&vic.raster,
                                                      VIC_RASTER_X(vic.raster_cycle + 1) + VIC_PIXEL_WIDTH,
                                                      (int*)&vic.raster.background_color,
                                                      new_background_color);

                    old_background_color = new_background_color;
                }

                if (new_border_color != old_border_color) {
                    raster_changes_border_add_int(&vic.raster,
                                                  VIC_RASTER_X(vic.raster_cycle + 1) + VIC_PIXEL_WIDTH,
                                                  (int*)&vic.raster.border_color,
                                                  new_border_color);

                    /* we also need the border color in multicolor mode,
                       so we duplicate it */
                    /* integer part */
                    raster_changes_foreground_add_int(&vic.raster,
                                                      VIC_RASTER_CHAR_INT(vic.raster_cycle + 1),
                                                      &vic.mc_border_color,
                                                      new_border_color);
                    /* fractional part (half chars) */
                    raster_changes_foreground_add_int(&vic.raster,
                                                      VIC_RASTER_CHAR_INT(vic.raster_cycle + 1),
                                                      &vic.half_char_flag,
                                                      VIC_RASTER_CHAR_FRAC(vic.raster_cycle + 1));
                }

                if (new_reverse != old_reverse) {
                    /* integer part */
                    raster_changes_foreground_add_int(&vic.raster,
                                                      VIC_RASTER_CHAR_INT(vic.raster_cycle + 1),
                                                      &vic.reverse,
                                                      new_reverse);
                    /* fractional part (half chars) */
                    raster_changes_foreground_add_int(&vic.raster,
                                                      VIC_RASTER_CHAR_INT(vic.raster_cycle + 1),
                                                      &vic.half_char_flag,
                                                      VIC_RASTER_CHAR_FRAC(vic.raster_cycle + 1));
                }


                if (new_border_color != old_border_color) {
                    /* old_mc_border_color is used by vic-draw.c to handle the
                       one hires vic pixel lateness of change */
                    raster_changes_foreground_add_int(&vic.raster,
                                                      VIC_RASTER_CHAR(vic.raster_cycle + 2),
                                                      &vic.old_mc_border_color,
                                                      new_border_color);

                    old_border_color = new_border_color;
                }

                if (new_reverse != old_reverse) {
                    /* old_reverse is used by vic-draw.c to handle the
                       3 hires vic pixels lateness of change */
                    raster_changes_foreground_add_int(&vic.raster,
                                                      VIC_RASTER_CHAR(vic.raster_cycle + 2),
                                                      &vic.old_reverse,
                                                      new_reverse);

                    old_reverse = new_reverse;
                }

                return;
            }
    }
}

BYTE vic_read(WORD addr)
{
    addr &= 0xf;

    switch (addr) {
        case 3:
            return ((VIC_RASTER_Y(maincpu_clk + vic.cycle_offset) & 1) << 7)
                   | (vic.regs[3] & ~0x80);
        case 4:
            return VIC_RASTER_Y(maincpu_clk + vic.cycle_offset) >> 1;
        case 6:
            return vic.light_pen.x;
        case 7:
            return vic.light_pen.y;
#ifdef HAVE_MOUSE
        case 8:
            if (_mouse_enabled) {
                return mouse_get_x();
            } else if (lightpen_enabled) {
                return lightpen_read_button_x();
            } else {
                return 0xff;
            }
            break;
        case 9:
            if (_mouse_enabled) {
                return mouse_get_y();
            } else if (lightpen_enabled) {
                return lightpen_read_button_y();
            } else {
                return 0xff;
            }
            break;
#else
        case 8:
        case 9:
            return 0xff;
#endif
        default:
            return vic.regs[addr];
    }
}

BYTE vic_peek(WORD addr)
{
    /* No side effects (unless mouse_get_* counts) */
    return vic_read(addr);
}
