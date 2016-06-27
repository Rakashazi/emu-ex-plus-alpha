/*
 * ted-timing.h - Timing related settings for the TED emulation.
 *
 * Written by
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

#ifndef VICE_TED_TIMING_H
#define VICE_TED_TIMING_H

/* Screen constants.  */
#define TED_PAL_SCREEN_HEIGHT           312
#define TED_NTSC_SCREEN_HEIGHT          262

/* Sideborder sizes */
#define TED_SCREEN_PAL_NORMAL_LEFTBORDERWIDTH      0x20
#define TED_SCREEN_PAL_NORMAL_RIGHTBORDERWIDTH     0x20
#define TED_SCREEN_PAL_FULL_LEFTBORDERWIDTH        0x30 /* actually 0x2e, but must be divisible by 8 */
#define TED_SCREEN_PAL_FULL_RIGHTBORDERWIDTH       0x28 /* actually 0x28, but must be divisible by 8 */
#define TED_SCREEN_PAL_DEBUG_LEFTBORDERWIDTH       0x88
#define TED_SCREEN_PAL_DEBUG_RIGHTBORDERWIDTH      0x30

#define TED_SCREEN_NTSC_NORMAL_LEFTBORDERWIDTH     0x20
#define TED_SCREEN_NTSC_NORMAL_RIGHTBORDERWIDTH    0x20
#define TED_SCREEN_NTSC_FULL_LEFTBORDERWIDTH       0x38
#define TED_SCREEN_NTSC_FULL_RIGHTBORDERWIDTH      0x30 /* actually 0x2c, but must be divisible by 8 */
#define TED_SCREEN_NTSC_DEBUG_LEFTBORDERWIDTH      0x88
#define TED_SCREEN_NTSC_DEBUG_RIGHTBORDERWIDTH     0x40


/* Y display ranges */
/* Notes:
   - "normal" shows all lines visible on a typical monitor
   - "full" shows all lines minus the vertical retrace
   - "debug" mode shows all lines, including vertical retrace
*/

/* values in the coordinate system of the Raster object */
#define TED_PAL_NO_BORDER_FIRST_DISPLAYED_LINE     59
#define TED_PAL_NO_BORDER_LAST_DISPLAYED_LINE      258

#ifdef DINGOO_NATIVE
#define TED_PAL_NORMAL_FIRST_DISPLAYED_LINE        40      /* 0x113 in TED raster counter */
#define TED_PAL_NORMAL_LAST_DISPLAYED_LINE         279     /* 0x0FA in TED raster counter */
#else
#define TED_PAL_NORMAL_FIRST_DISPLAYED_LINE        19      /* 0x113 in TED raster counter */
#define TED_PAL_NORMAL_LAST_DISPLAYED_LINE         306     /* 0x0FA in TED raster counter */
#endif

#define TED_PAL_FULL_FIRST_DISPLAYED_LINE          11
#define TED_PAL_FULL_LAST_DISPLAYED_LINE           308

#define TED_PAL_DEBUG_FIRST_DISPLAYED_LINE         0
#define TED_PAL_DEBUG_LAST_DISPLAYED_LINE          311

/*
NTSC display ranges:
*/

#define TED_NTSC_NO_BORDER_FIRST_DISPLAYED_LINE    37
#define TED_NTSC_NO_BORDER_LAST_DISPLAYED_LINE     236

#ifdef DINGOO_NATIVE
#define TED_NTSC_NORMAL_FIRST_DISPLAYED_LINE       40
#define TED_NTSC_NORMAL_LAST_DISPLAYED_LINE        279
#else
#define TED_NTSC_NORMAL_FIRST_DISPLAYED_LINE       19
#define TED_NTSC_NORMAL_LAST_DISPLAYED_LINE        260
#endif

#define TED_NTSC_FULL_FIRST_DISPLAYED_LINE         11
#define TED_NTSC_FULL_LAST_DISPLAYED_LINE          261

#define TED_NTSC_DEBUG_FIRST_DISPLAYED_LINE        0
#define TED_NTSC_DEBUG_LAST_DISPLAYED_LINE         261


/*
#define TED_SCREEN_PAL_NORMAL_WIDTH  (TED_SCREEN_PAL_BORDERWIDTH + TED_SCREEN_XPIX)
#define TED_SCREEN_PAL_NORMAL_HEIGHT (TED_PAL_LAST_DISPLAYED_LINE - TED_PAL_FIRST_DISPLAYED_LINE)
#define TED_SCREEN_NTSC_NORMAL_WIDTH  (TED_SCREEN_NTSC_BORDERWIDTH + TED_SCREEN_XPIX)
#define TED_SCREEN_NTSC_NORMAL_HEIGHT (TED_NTSC_LAST_DISPLAYED_LINE - TED_NTSC_FIRST_DISPLAYED_LINE)
*/
#define TED_SCREEN_PAL_NORMAL_WIDTH  (320 + TED_SCREEN_PAL_NORMAL_LEFTBORDERWIDTH + TED_SCREEN_PAL_NORMAL_RIGHTBORDERWIDTH)
#define TED_SCREEN_PAL_NORMAL_HEIGHT (1 + (TED_PAL_NORMAL_LAST_DISPLAYED_LINE - TED_PAL_NORMAL_FIRST_DISPLAYED_LINE))
#define TED_SCREEN_NTSC_NORMAL_WIDTH  (320 + TED_SCREEN_NTSC_NORMAL_LEFTBORDERWIDTH + TED_SCREEN_NTSC_NORMAL_RIGHTBORDERWIDTH)
#define TED_SCREEN_NTSC_NORMAL_HEIGHT (1 + (TED_NTSC_NORMAL_LAST_DISPLAYED_LINE - TED_NTSC_NORMAL_FIRST_DISPLAYED_LINE))

struct machine_timing_s;

extern void ted_timing_set(struct machine_timing_s *machine_timing, int bordermode);

#endif
