/*
 * c128.h
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_C128_H
#define VICE_C128_H

#define C128_PAL_CYCLES_PER_SEC  985248
#define C128_PAL_CYCLES_PER_LINE 63
#define C128_PAL_SCREEN_LINES    312
#define C128_PAL_CYCLES_PER_RFSH (C128_PAL_SCREEN_LINES * C128_PAL_CYCLES_PER_LINE)
#define C128_PAL_RFSH_PER_SEC    (1.0 / ((double)C128_PAL_CYCLES_PER_RFSH / (double)C128_PAL_CYCLES_PER_SEC))

#define C128_NTSC_CYCLES_PER_SEC  1022730
#define C128_NTSC_CYCLES_PER_LINE 65
#define C128_NTSC_SCREEN_LINES    263
#define C128_NTSC_CYCLES_PER_RFSH (C128_NTSC_SCREEN_LINES * C128_NTSC_CYCLES_PER_LINE)
#define C128_NTSC_RFSH_PER_SEC    (1.0 / ((double)C128_NTSC_CYCLES_PER_RFSH / (double)C128_NTSC_CYCLES_PER_SEC))

/* $01 bit 7 fall-off cycles (1->0), average is about 53 msec for a 8502 */
#define C128_CPU8502_DATA_PORT_FALL_OFF_CYCLES 53000

#define C128_MACHINE_INT       0
#define C128_MACHINE_FINNISH   1
#define C128_MACHINE_FRENCH    2
#define C128_MACHINE_GERMAN    3
#define C128_MACHINE_ITALIAN   4
#define C128_MACHINE_NORWEGIAN 5
#define C128_MACHINE_SWEDISH   6
#define C128_MACHINE_SWISS     7

struct cia_context_s;
struct printer_context_s;
struct tpi_context_s;

/* The first part must be identical to the C64 context struct.  */
typedef struct machine_context_s {
    struct cia_context_s *cia1;
    struct cia_context_s *cia2;
    struct printer_context_s *printer[3];
} machine_context_t;

extern machine_context_t machine_context;

extern void machine_kbdbuf_reset_c128(void);
extern void machine_kbdbuf_reset_c64(void);
extern void machine_autostart_reset_c128(void);
extern void machine_autostart_reset_c64(void);
extern void machine_tape_init_c64(void);
extern void machine_tape_init_c128(void);

#endif
