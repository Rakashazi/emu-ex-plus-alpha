/*
 * vic20.h
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_VIC20_H
#define VICE_VIC20_H

#include "vic.h"

#define VIC20_PAL_CYCLES_PER_SEC        1108405
#define VIC20_PAL_CYCLES_PER_LINE       71
#define VIC20_PAL_SCREEN_LINES          312
#define VIC20_PAL_CYCLE_OFFSET          0

#define VIC20_PAL_CYCLES_PER_RFSH (VIC20_PAL_SCREEN_LINES * VIC20_PAL_CYCLES_PER_LINE)
#define VIC20_PAL_RFSH_PER_SEC    (1.0 / ((double)VIC20_PAL_CYCLES_PER_RFSH / (double)VIC20_PAL_CYCLES_PER_SEC))

#define VIC20_NTSC_CYCLES_PER_SEC       1022727
#define VIC20_NTSC_CYCLES_PER_LINE      65

/* non interlaced */
#define VIC20_NTSC_CYCLES_FIRST_LINE    32
#define VIC20_NTSC_CYCLES_LAST_LINE     33
#define VIC20_NTSC_LAST_LINE            261
#define VIC20_NTSC_SCREEN_LINES         261 /* really 262, but first and last add up to one complete line */
#define VIC20_NTSC_CYCLE_OFFSET         37

#define VIC20_NTSC_CYCLES_PER_RFSH (VIC20_NTSC_SCREEN_LINES * VIC20_NTSC_CYCLES_PER_LINE)
#define VIC20_NTSC_RFSH_PER_SEC    (1.0 / ((double)VIC20_NTSC_CYCLES_PER_RFSH / (double)VIC20_NTSC_CYCLES_PER_SEC))

/* interlaced */
#define VIC20_NTSC_INTERLACE_FIELD1_SCREEN_LINES         263    /* really 262 and a half */
#define VIC20_NTSC_INTERLACE_FIELD1_CYCLES_FIRST_LINE    32     /* adds up to 65 with field2 last line */
#define VIC20_NTSC_INTERLACE_FIELD1_CYCLES_LAST_LINE     65
#define VIC20_NTSC_INTERLACE_FIELD1_LAST_LINE            262

#define VIC20_NTSC_INTERLACE_FIELD2_SCREEN_LINES         262    /* really 262 and a half */
#define VIC20_NTSC_INTERLACE_FIELD2_CYCLES_FIRST_LINE    65
#define VIC20_NTSC_INTERLACE_FIELD2_CYCLES_LAST_LINE     33     /* adds up to 65 with field1 first line */
#define VIC20_NTSC_INTERLACE_FIELD2_LAST_LINE            262

/* FIXME: these are not used yet */
#define VIC20_NTSC_INTERLACE_CYCLES_PER_RFSH \
    (((VIC20_NTSC_INTERLACE_FIELD1_SCREEN_LINES + VIC20_NTSC_INTERLACE_FIELD2_SCREEN_LINES) \
      * VIC20_NTSC_CYCLES_PER_LINE) / 2)
#define VIC20_NTSC_INTERLACE_RFSH_PER_SEC    (1.0 / ((double)VIC20_NTSC_INTERLACE_CYCLES_PER_RFSH / (double)VIC20_NTSC_CYCLES_PER_SEC))

#define VIC_SCREEN_PAL_NORMAL_WIDTH  (VIC_PAL_DISPLAY_WIDTH * 2)
#define VIC_SCREEN_PAL_NORMAL_HEIGHT (VIC20_PAL_LAST_DISPLAYED_LINE - VIC20_PAL_FIRST_DISPLAYED_LINE)
#define VIC_SCREEN_NTSC_NORMAL_WIDTH  (VIC_NTSC_DISPLAY_WIDTH * 2)
#define VIC_SCREEN_NTSC_NORMAL_HEIGHT (VIC20_NTSC_LAST_DISPLAYED_LINE - VIC20_NTSC_FIRST_DISPLAYED_LINE)

struct printer_context_s;
struct via_context_s;

typedef struct machine_context_s {
    struct via_context_s *via1;
    struct via_context_s *via2;
    struct via_context_s *ieeevia1;
    struct via_context_s *ieeevia2;
    struct printer_context_s *printer[3];
} machine_context_t;

extern machine_context_t machine_context;

#endif
