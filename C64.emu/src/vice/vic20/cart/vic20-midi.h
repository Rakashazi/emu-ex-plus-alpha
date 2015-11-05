/*
 * vic20-midi.c - VIC20 specific MIDI (6850 UART) emulation.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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

#ifndef VICE_VIC20_MIDI_H
#define VICE_VIC20_MIDI_H

#include "midi.h"

extern int vic20_midi_resources_init(void);
extern int vic20_midi_cmdline_options_init(void);

/* Emulated interfaces */
enum { MIDI_MODE_MAPLIN = 0   /* Electronics - Maplin magazine */
};

struct snapshot_s;
extern int vic20_midi_snapshot_read_module(struct snapshot_s *s);
extern int vic20_midi_snapshot_write_module(struct snapshot_s *s);

#endif
