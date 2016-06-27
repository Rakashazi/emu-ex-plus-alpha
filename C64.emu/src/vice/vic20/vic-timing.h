/*
 * vic-timing.h
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#ifndef VICE_VIC_TIMING_H
#define VICE_VIC_TIMING_H

/* PAL: total screen dimension of 224x283 + 284th rasterline in bordercolor */
#define VIC_PAL_NORMAL_FIRST_DISPLAYED_LINE         28
#define VIC_PAL_NORMAL_LAST_DISPLAYED_LINE          311
#define VIC_PAL_NORMAL_DISPLAY_WIDTH                224
#define VIC_PAL_NORMAL_LEFTBORDERWIDTH              12

#define VIC_PAL_FULL_FIRST_DISPLAYED_LINE           18
#define VIC_PAL_FULL_LAST_DISPLAYED_LINE            311
#define VIC_PAL_FULL_DISPLAY_WIDTH                  256
#define VIC_PAL_FULL_LEFTBORDERWIDTH                12

#define VIC_PAL_DEBUG_FIRST_DISPLAYED_LINE          0
#define VIC_PAL_DEBUG_LAST_DISPLAYED_LINE           311
#define VIC_PAL_DEBUG_DISPLAY_WIDTH                 284  /* 71 cycles * 4 pixels */
#define VIC_PAL_DEBUG_LEFTBORDERWIDTH               12

#define VIC_PAL_NO_BORDER_FIRST_DISPLAYED_LINE      76
#define VIC_PAL_NO_BORDER_LAST_DISPLAYED_LINE       259
#define VIC_PAL_NO_BORDER_DISPLAY_WIDTH             176
#define VIC_PAL_NO_BORDER_LEFTBORDERWIDTH           12


/* NTSC: total screen dimension of 200x233 + 234th rasterline in bordercolor */
#define VIC_NTSC_NORMAL_FIRST_DISPLAYED_LINE        28
#define VIC_NTSC_NORMAL_LAST_DISPLAYED_LINE         261
#define VIC_NTSC_NORMAL_DISPLAY_WIDTH               200
#define VIC_NTSC_NORMAL_LEFTBORDERWIDTH             4

#define VIC_NTSC_FULL_FIRST_DISPLAYED_LINE          4
#define VIC_NTSC_FULL_LAST_DISPLAYED_LINE           261
#define VIC_NTSC_FULL_DISPLAY_WIDTH                 232
#define VIC_NTSC_FULL_LEFTBORDERWIDTH               4

#define VIC_NTSC_DEBUG_FIRST_DISPLAYED_LINE         1
#define VIC_NTSC_DEBUG_LAST_DISPLAYED_LINE          261
#define VIC_NTSC_DEBUG_DISPLAY_WIDTH                260  /* 65 cycles * 4 pixels */
#define VIC_NTSC_DEBUG_LEFTBORDERWIDTH              4

#define VIC_NTSC_NO_BORDER_FIRST_DISPLAYED_LINE     50
#define VIC_NTSC_NO_BORDER_LAST_DISPLAYED_LINE      233
#define VIC_NTSC_NO_BORDER_DISPLAY_WIDTH            176
#define VIC_NTSC_NO_BORDER_LEFTBORDERWIDTH          5


struct machine_timing_s;
extern void vic_timing_set(struct machine_timing_s *machine_timing, int border_mode);

#endif
