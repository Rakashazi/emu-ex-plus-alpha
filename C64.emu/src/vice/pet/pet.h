/*
 * pet.h
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_PET_H
#define VICE_PET_H

/*

    The PET CPU clock is exactly (within chip tolerances) 1MHz
    (16.0Mhz / 16 or 8.0Mhz / 8 = 1.0Mhz).

    The screen's "50Hz" or "60Hz" are actually derived from this clock by
    counting cycles in the CRTC.

    The 50 resp. 60Hz interrupt is triggered by the vertical sync signal.
    Which in turn is derived from the CPU clock by the CRTC counting an
    appropriate number of cycles.

    Only the ROMs are different for 60Hz and 50Hz machines

    The 50Hz machines (at least some) have a ROM with 313 lines with 64
    cycles each, which result in about 20ms screen time - thus 50Hz.

    PAL defines 313 rasterlines for one half-image (actually 625 per full
    image) with 64us (i.e. 64 cycles) per rasterline. That results in 20032
    cycles per screen - 49.920Hz. The second half-image is defined as 312
    raster lines, which makes 19968 cycles - 50.080 Hz. Both together
    are 40000 cycles for two screens.

*/

/* NOTE: Except for the exact CYCLES_PER_SEC those are only reasonable defaults.
         They get overwritten when writing to the CRTC (usually by the editor ROM).
         they are defined here for reference, and to initialize the emulation
         with reasonable defaults when needed.
*/

#define PET_PAL_CYCLES_PER_SEC   1000000
/* #define PET_PAL_CYCLES_PER_SEC  999600 */ /* works with "8296d diagnostics" if editor rom 901474-04 (50Hz) is used */

#define PET_PAL_CYCLES_PER_LINE  64
#define PET_PAL_SCREEN_LINES     313
#define PET_PAL_CYCLES_PER_RFSH  (PET_PAL_SCREEN_LINES * PET_PAL_CYCLES_PER_LINE)
#define PET_PAL_RFSH_PER_SEC     (1.0 / ((double)PET_PAL_CYCLES_PER_RFSH / (double)PET_PAL_CYCLES_PER_SEC))

#define PET_NTSC_CYCLES_PER_SEC  1000000

#define PET_NTSC_CYCLES_PER_LINE 64
#define PET_NTSC_SCREEN_LINES    264
#define PET_NTSC_CYCLES_PER_RFSH (PET_NTSC_SCREEN_LINES * PET_NTSC_CYCLES_PER_LINE)
#define PET_NTSC_RFSH_PER_SEC    (1.0 / ((double)PET_NTSC_CYCLES_PER_RFSH / (double)PET_NTSC_CYCLES_PER_SEC))

#define PET_COLOUR_TYPE_OFF     0
#define PET_COLOUR_TYPE_RGBI    1
#define PET_COLOUR_TYPE_ANALOG  2

extern void pet_crtc_set_screen(void);

struct printer_context_s;
struct via_context_s;

typedef struct machine_context_s {
    struct via_context_s *via;
    struct printer_context_s *printer[3];
} machine_context_t;

extern machine_context_t machine_context;

#endif
