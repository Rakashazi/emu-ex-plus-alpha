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

#define VIC_PAL_NORMAL_FIRST_DISPLAYED_LINE         28
#define VIC_PAL_NORMAL_LAST_DISPLAYED_LINE          311
#define VIC_PAL_NORMAL_DISPLAY_WIDTH                224

#define VIC_PAL_FULL_FIRST_DISPLAYED_LINE           18
#define VIC_PAL_FULL_LAST_DISPLAYED_LINE            311
#define VIC_PAL_FULL_DISPLAY_WIDTH                  248

#define VIC_PAL_DEBUG_FIRST_DISPLAYED_LINE          0
#define VIC_PAL_DEBUG_LAST_DISPLAYED_LINE           311
#define VIC_PAL_DEBUG_DISPLAY_WIDTH                 272

#define VIC_PAL_NO_BORDER_FIRST_DISPLAYED_LINE      76
#define VIC_PAL_NO_BORDER_LAST_DISPLAYED_LINE       259
#define VIC_PAL_NO_BORDER_DISPLAY_WIDTH             176

/*
    FIXME: in NTSC the text window is (by default) not centered on the line,
           some way to do that is needed to make "no border" work correctly.
 */
#define VIC_NTSC_NORMAL_FIRST_DISPLAYED_LINE        8
#define VIC_NTSC_NORMAL_LAST_DISPLAYED_LINE         260
#define VIC_NTSC_NORMAL_DISPLAY_WIDTH               200

#define VIC_NTSC_FULL_FIRST_DISPLAYED_LINE          4
#define VIC_NTSC_FULL_LAST_DISPLAYED_LINE           260
#define VIC_NTSC_FULL_DISPLAY_WIDTH                 204

#define VIC_NTSC_DEBUG_FIRST_DISPLAYED_LINE         0
#define VIC_NTSC_DEBUG_LAST_DISPLAYED_LINE          260
#define VIC_NTSC_DEBUG_DISPLAY_WIDTH                208

#define VIC_NTSC_NO_BORDER_FIRST_DISPLAYED_LINE     50
#define VIC_NTSC_NO_BORDER_LAST_DISPLAYED_LINE      233
#define VIC_NTSC_NO_BORDER_DISPLAY_WIDTH            (176 + 8) /* FIXME */


struct machine_timing_s;
extern void vic_timing_set(struct machine_timing_s *machine_timing, int border_mode);

#endif
