/*
 * cbm2.h
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

#ifndef VICE_CBM2_H
#define VICE_CBM2_H

#include "mem.h"

/*
    the C6x0/C7x0 series has a CRTC
    - cpu clock is 18.0Mhz / 9 = 2.0Mhz

    "The kernal is identical. The video initialization depends on
     two input signals on one of the 6525 chips. One switches between PAL and NTSC
     (and also between 50 and 60 Hz), the other between internal (CBM 7x0) and
     external (CBM 6x0) monitor."
*/

/* NOTE: Except for the exact CYCLES_PER_SEC those are only reasonable defaults.
         They get overwritten when writing to the CRTC (usually by the kernal).
         they are defined here for reference, and to initialize the emulation
         with reasonable defaults when needed.
*/

/* FIXME: handshake with a 2031 disk drive has a timing issue, where
   the CBM-II is slightly too fast. Reducing speed for about 1.3% from
   the real 2MHz seems to help here...

#define C610_PAL_CYCLES_PER_SEC   1974000
*/
#define C610_PAL_CYCLES_PER_SEC   2000000
#define C610_PAL_CYCLES_PER_LINE  128
#define C610_PAL_SCREEN_LINES     313
#define C610_PAL_CYCLES_PER_RFSH  (C610_PAL_SCREEN_LINES * C610_PAL_CYCLES_PER_LINE)
#define C610_PAL_RFSH_PER_SEC     (1.0 / ((double)C610_PAL_CYCLES_PER_RFSH / (double)C610_PAL_CYCLES_PER_SEC))

#define C610_NTSC_CYCLES_PER_SEC  2000000
#define C610_NTSC_CYCLES_PER_LINE 128
#define C610_NTSC_SCREEN_LINES    264
#define C610_NTSC_CYCLES_PER_RFSH (C610_NTSC_SCREEN_LINES * C610_NTSC_CYCLES_PER_LINE)
#define C610_NTSC_RFSH_PER_SEC    (1.0 / ((double)C610_NTSC_CYCLES_PER_RFSH / (double)C610_NTSC_CYCLES_PER_SEC))

/*
    the C500 series has a VIC-II
*/

#define C500_PAL_CYCLES_PER_SEC   985248
#define C500_PAL_CYCLES_PER_LINE  63
#define C500_PAL_SCREEN_LINES     312
#define C500_PAL_CYCLES_PER_RFSH  (C500_PAL_SCREEN_LINES * C500_PAL_CYCLES_PER_LINE)
#define C500_PAL_RFSH_PER_SEC     (1.0 / ((double)C500_PAL_CYCLES_PER_RFSH / (double)C500_PAL_CYCLES_PER_SEC))

#define C500_NTSC_CYCLES_PER_SEC  1022730
#define C500_NTSC_CYCLES_PER_LINE 65
#define C500_NTSC_SCREEN_LINES    263
#define C500_NTSC_CYCLES_PER_RFSH (C500_NTSC_SCREEN_LINES * C500_NTSC_CYCLES_PER_LINE)
#define C500_NTSC_RFSH_PER_SEC    (1.0 / ((double)C500_NTSC_CYCLES_PER_RFSH / (double)C500_NTSC_CYCLES_PER_SEC))

struct snapshot_s;

extern int cbm2_c500_snapshot_write_module(struct snapshot_s *p);
extern int cbm2_c500_snapshot_read_module(struct snapshot_s *p);

struct cia_context_s;
struct tpi_context_s;

typedef struct machine_context_s {
    struct cia_context_s *cia1;
    struct tpi_context_s *tpi1;
    struct tpi_context_s *tpi2;
} machine_context_t;

extern machine_context_t machine_context;

extern read_func_ptr_t *_mem_read_ind_tab_ptr;
extern store_func_ptr_t *_mem_write_ind_tab_ptr;

#endif
