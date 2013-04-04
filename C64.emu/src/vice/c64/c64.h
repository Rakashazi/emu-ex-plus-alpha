
/*! \file c64/c64.h
 *
 *  \brief Definitions for C64 Machine context and -timing.
 *
 *  \author Andreas Boose <viceteam@t-online.de>
 *  \author Ettore Perazzoli <ettore@comm2000.it>
 */

/*
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

#ifndef VICE_C64_H
#define VICE_C64_H

#define C64_PAL_CYCLES_PER_SEC  985248
#define C64_PAL_CYCLES_PER_LINE 63
#define C64_PAL_SCREEN_LINES    312
#define C64_PAL_CYCLES_PER_RFSH (C64_PAL_SCREEN_LINES * C64_PAL_CYCLES_PER_LINE)
/* PAL refresh rate: 50.123432124542124 */
#define C64_PAL_RFSH_PER_SEC    (1.0 / ((double)C64_PAL_CYCLES_PER_RFSH / (double)C64_PAL_CYCLES_PER_SEC))

#define C64_NTSC_CYCLES_PER_SEC  1022730
#define C64_NTSC_CYCLES_PER_LINE 65
#define C64_NTSC_SCREEN_LINES    263
#define C64_NTSC_CYCLES_PER_RFSH (C64_NTSC_SCREEN_LINES * C64_NTSC_CYCLES_PER_LINE)
#define C64_NTSC_RFSH_PER_SEC    (1.0 / ((double)C64_NTSC_CYCLES_PER_RFSH / (double)C64_NTSC_CYCLES_PER_SEC))

#define C64_NTSCOLD_CYCLES_PER_SEC  1022730
#define C64_NTSCOLD_CYCLES_PER_LINE 64
#define C64_NTSCOLD_SCREEN_LINES    262
#define C64_NTSCOLD_CYCLES_PER_RFSH (C64_NTSCOLD_SCREEN_LINES * C64_NTSCOLD_CYCLES_PER_LINE)
#define C64_NTSCOLD_RFSH_PER_SEC  (1.0 / ((double)C64_NTSCOLD_CYCLES_PER_RFSH / (double)C64_NTSCOLD_CYCLES_PER_SEC))

#define C64_PALN_CYCLES_PER_SEC   1023440
#define C64_PALN_CYCLES_PER_LINE 65
#define C64_PALN_SCREEN_LINES    312
#define C64_PALN_CYCLES_PER_RFSH (C64_PALN_SCREEN_LINES \
                                  * C64_PALN_CYCLES_PER_LINE)
#define C64_PALN_RFSH_PER_SEC  (1.0 / ((double)C64_PALN_CYCLES_PER_RFSH \
                                       / (double)C64_PALN_CYCLES_PER_SEC))

/*
    NOTE: fall-off cycles are heavily chip- and temperature dependant. as a
          consequence it is very hard to find suitable realistic values that
          always work and we can only tweak them based on testcases. (unless we
          want to make it configureable or emulate temperature over time =))

          it probably makes sense to tweak the values for a warmed up CPU, since
          this is likely how (old) programs were coded and tested :)

          see testprogs/CPU/cpuport for details and tests
*/

/* $01 bits 6 and 7 fall-off cycles (1->0), average is about 350 msec for a 6510 */
/* NOTE: the unused bits of the 6510 seem to be much more temperature dependant
         and the fall-off time decreases quicker and more drastically than on a
         8500
*/
#define C64_CPU6510_DATA_PORT_FALL_OFF_CYCLES 350000
/*
   cpuports.prg from the lorenz testsuite will fail when the falloff takes less
   than 5984 cycles. he explicitly delays by ~1280 cycles and mentions capacitance, 
   so he probably even was aware of what happens.
 */

/* $01 bits 6 and 7 fall-off cycles (1->0), average is about 1500 msec for a 8500 */
#define C64_CPU8500_DATA_PORT_FALL_OFF_CYCLES 1500000

struct cia_context_s;
struct printer_context_s;

typedef struct machine_context_s {
    struct cia_context_s *cia1;
    struct cia_context_s *cia2;
    struct printer_context_s *printer[3];
} machine_context_t;

extern machine_context_t machine_context;

#endif
