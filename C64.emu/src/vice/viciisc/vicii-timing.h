/*
 * vicii-timing.h - Timing related settings for the MOS 6569 (VIC-II) emulation.
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

#ifndef VICE_VICII_TIMING_H
#define VICE_VICII_TIMING_H

/* Sideborder sizes */
#define VICII_SCREEN_PAL_NORMAL_LEFTBORDERWIDTH      0x20
#define VICII_SCREEN_PAL_NORMAL_RIGHTBORDERWIDTH     0x20
#define VICII_SCREEN_PAL_FULL_LEFTBORDERWIDTH        0x30 /* actually 0x2e, but must be divisible by 8 */
#define VICII_SCREEN_PAL_FULL_RIGHTBORDERWIDTH       0x28 /* actually 0x28, but must be divisible by 8 */
#define VICII_SCREEN_PAL_DEBUG_LEFTBORDERWIDTH       0x88 /* 17 cycles */
#define VICII_SCREEN_PAL_DEBUG_RIGHTBORDERWIDTH      0x30 /* 6 cycles */

#define VICII_SCREEN_NTSC_NORMAL_LEFTBORDERWIDTH     0x20
#define VICII_SCREEN_NTSC_NORMAL_RIGHTBORDERWIDTH    0x20
#define VICII_SCREEN_NTSC_FULL_LEFTBORDERWIDTH       0x38
#define VICII_SCREEN_NTSC_FULL_RIGHTBORDERWIDTH      0x30 /* actually 0x2c, but must be divisible by 8 */
#define VICII_SCREEN_NTSC_DEBUG_LEFTBORDERWIDTH      0x88 /* 17 cycles */
#define VICII_SCREEN_NTSC_DEBUG_RIGHTBORDERWIDTH     0x40 /* 8 cycles */

#define VICII_SCREEN_NTSCOLD_NORMAL_LEFTBORDERWIDTH  0x20
#define VICII_SCREEN_NTSCOLD_NORMAL_RIGHTBORDERWIDTH 0x20
#define VICII_SCREEN_NTSCOLD_FULL_LEFTBORDERWIDTH    0x38
#define VICII_SCREEN_NTSCOLD_FULL_RIGHTBORDERWIDTH   0x30 /* actually 0x2c, but must be divisible by 8 */
#define VICII_SCREEN_NTSCOLD_DEBUG_LEFTBORDERWIDTH   0x88 /* 17 cycles */
#define VICII_SCREEN_NTSCOLD_DEBUG_RIGHTBORDERWIDTH  0x38 /* 7 cycles */

#define VICII_SCREEN_PALN_NORMAL_LEFTBORDERWIDTH     0x20
#define VICII_SCREEN_PALN_NORMAL_RIGHTBORDERWIDTH    0x20
#define VICII_SCREEN_PALN_FULL_LEFTBORDERWIDTH       0x38
#define VICII_SCREEN_PALN_FULL_RIGHTBORDERWIDTH      0x30 /* actually 0x2c, but must be divisible by 8 */
#define VICII_SCREEN_PALN_DEBUG_LEFTBORDERWIDTH      0x88 /* 17 cycles */
#define VICII_SCREEN_PALN_DEBUG_RIGHTBORDERWIDTH     0x40 /* 8 cycles */

/* Y display ranges */
/* Notes:
   - If the last displayed line setting is larger than
     the screen height, lines 0+ are displayed in the lower
     border. This is used for NTSC display.
   - "normal" shows all lines visible on a typical monitor
   - "full" shows all lines minus the vertical retrace
   - "debug" mode shows all lines, including vertical retrace
*/
#define VICII_PAL_NORMAL_FIRST_DISPLAYED_LINE        0x10   /* 16 */
#define VICII_PAL_NORMAL_LAST_DISPLAYED_LINE         0x11f  /* 287 */
#define VICII_PAL_FULL_FIRST_DISPLAYED_LINE          0x08   /* 8 */
#define VICII_PAL_FULL_LAST_DISPLAYED_LINE           0x12c  /* 300 */
#define VICII_PAL_DEBUG_FIRST_DISPLAYED_LINE         0x00   /* 0 */
#define VICII_PAL_DEBUG_LAST_DISPLAYED_LINE          0x137  /* 311 */

/*
NTSC display ranges:
- 29 lines top border
     22-50 (0x016-0X032)
- 200 lines screen
    51-250 (0X033-0X0fa)
- 24 lines bottom border
   251-255 (0X0fb-0x0ff)
       0-7 (0x100-0x107)
      0-10 (0x000-0x00a)
- 10 lines vertical blanking (non displayed lines)
     11-21 (0x00b-0x015)
  - exactly in the middle is the vertical retrace
  - one less for old ntsc (?)
- makes 263 lines total (262 for old ntsc)
*/

#define VICII_NO_BORDER_FIRST_DISPLAYED_LINE         51
#define VICII_NO_BORDER_LAST_DISPLAYED_LINE          250

#define VICII_NTSC_NORMAL_FIRST_DISPLAYED_LINE       0x1c   /* 8 + 20 */
#define VICII_NTSC_NORMAL_LAST_DISPLAYED_LINE        0x112  /* 254 + 20 */
#define VICII_NTSC_FULL_FIRST_DISPLAYED_LINE         0x16   /* 2 + 20 */
#define VICII_NTSC_FULL_LAST_DISPLAYED_LINE          0x112  /* 254 + 20 */
#define VICII_NTSC_DEBUG_FIRST_DISPLAYED_LINE        0x14   /* 0 + 20 */
#define VICII_NTSC_DEBUG_LAST_DISPLAYED_LINE         0x11a  /* 262 + 20 */

#define VICII_NTSCOLD_NORMAL_FIRST_DISPLAYED_LINE    0x1c   /* 8 + 20 */
#define VICII_NTSCOLD_NORMAL_LAST_DISPLAYED_LINE     0x112  /* 254 + 20 */
#define VICII_NTSCOLD_FULL_FIRST_DISPLAYED_LINE      0x16   /* 2 + 20 */
#define VICII_NTSCOLD_FULL_LAST_DISPLAYED_LINE       0x112  /* 254 + 20 */
#define VICII_NTSCOLD_DEBUG_FIRST_DISPLAYED_LINE     0x14   /* 0 + 20 */
#define VICII_NTSCOLD_DEBUG_LAST_DISPLAYED_LINE      0x119  /* 261 + 20 */

#define VICII_PALN_NORMAL_FIRST_DISPLAYED_LINE       0x10   /* 16 */
#define VICII_PALN_NORMAL_LAST_DISPLAYED_LINE        0x11f  /* 287 */
#define VICII_PALN_FULL_FIRST_DISPLAYED_LINE         0x08   /* 8 */
#define VICII_PALN_FULL_LAST_DISPLAYED_LINE          0x12c  /* 300 */
#define VICII_PALN_DEBUG_FIRST_DISPLAYED_LINE        0x00   /* 0 */
#define VICII_PALN_DEBUG_LAST_DISPLAYED_LINE         0x137  /* 311 */

#define VICII_SCREEN_PAL_NORMAL_WIDTH  (320 + VICII_SCREEN_PAL_NORMAL_LEFTBORDERWIDTH + VICII_SCREEN_PAL_NORMAL_RIGHTBORDERWIDTH)
#define VICII_SCREEN_PAL_NORMAL_HEIGHT (1 + (VICII_PAL_NORMAL_LAST_DISPLAYED_LINE - VICII_PAL_NORMAL_FIRST_DISPLAYED_LINE))
#define VICII_SCREEN_PALN_NORMAL_WIDTH  (320 + VICII_SCREEN_PALN_NORMAL_LEFTBORDERWIDTH + VICII_SCREEN_PALN_NORMAL_RIGHTBORDERWIDTH)
#define VICII_SCREEN_PALN_NORMAL_HEIGHT (1 + (VICII_PALN_NORMAL_LAST_DISPLAYED_LINE - VICII_PALN_NORMAL_FIRST_DISPLAYED_LINE))
#define VICII_SCREEN_NTSC_NORMAL_WIDTH  (320 + VICII_SCREEN_NTSC_NORMAL_LEFTBORDERWIDTH + VICII_SCREEN_NTSC_NORMAL_RIGHTBORDERWIDTH)
#define VICII_SCREEN_NTSC_NORMAL_HEIGHT (1 + (VICII_NTSC_NORMAL_LAST_DISPLAYED_LINE - VICII_NTSC_NORMAL_FIRST_DISPLAYED_LINE))
#define VICII_SCREEN_NTSCOLD_NORMAL_WIDTH  (320 + VICII_SCREEN_NTSCOLD_NORMAL_LEFTBORDERWIDTH + VICII_SCREEN_NTSCOLD_NORMAL_RIGHTBORDERWIDTH)
#define VICII_SCREEN_NTSCOLD_NORMAL_HEIGHT (1 + (VICII_NTSCOLD_NORMAL_LAST_DISPLAYED_LINE - VICII_NTSCOLD_NORMAL_FIRST_DISPLAYED_LINE))

struct machine_timing_s;

extern void vicii_timing_set(struct machine_timing_s *machine_timing,
                             int border_mode);

#endif
