/*
 * plus4.h
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

#ifndef VICE_PLUS4_H
#define VICE_PLUS4_H

#define PLUS4_PAL_CYCLES_PER_SEC  1773447
#define PLUS4_PAL_CYCLES_PER_LINE 114
#define PLUS4_PAL_SCREEN_LINES    312
#define PLUS4_PAL_CYCLES_PER_RFSH (PLUS4_PAL_SCREEN_LINES * PLUS4_PAL_CYCLES_PER_LINE)
#define PLUS4_PAL_RFSH_PER_SEC    (1.0 / ((double)PLUS4_PAL_CYCLES_PER_RFSH \
                                          / (double)PLUS4_PAL_CYCLES_PER_SEC))

#define PLUS4_NTSC_CYCLES_PER_SEC  1789772
#define PLUS4_NTSC_CYCLES_PER_LINE 114
#define PLUS4_NTSC_SCREEN_LINES    262
#define PLUS4_NTSC_CYCLES_PER_RFSH (PLUS4_NTSC_SCREEN_LINES * PLUS4_NTSC_CYCLES_PER_LINE)
#define PLUS4_NTSC_RFSH_PER_SEC    (1.0 / ((double)PLUS4_NTSC_CYCLES_PER_RFSH \
                                           / (double)PLUS4_NTSC_CYCLES_PER_SEC))

struct printer_context_s;

typedef struct machine_context_s {
    struct printer_context_s *printer[3];
} machine_context_t;

extern machine_context_t machine_context;

#endif
