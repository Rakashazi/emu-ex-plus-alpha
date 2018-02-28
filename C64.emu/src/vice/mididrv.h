/*
 * mididrv.h - MIDI driver interface.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_MIDIDRV_H
#define VICE_MIDIDRV_H

#include "types.h"

extern void mididrv_init(void);

/* Opens a MIDI device */
extern int mididrv_in_open(void);
extern int mididrv_out_open(void);

/* Closes the MIDI device */
extern void mididrv_in_close(void);
extern void mididrv_out_close(void);

/* MIDI device I/O */
/* return: -1 if error, 1 if a byte was read to *b, 0 if no new bytes */
extern int mididrv_in(BYTE *b);
extern void mididrv_out(BYTE b);

extern int mididrv_resources_init(void);
extern void mididrv_resources_shutdown(void);
extern int mididrv_cmdline_options_init(void);

#endif
