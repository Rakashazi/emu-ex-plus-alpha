/*
 * plus4speech.h - v364 speech support
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#ifndef VICE_PLUS4SPEECH_H
#define VICE_PLUS4SPEECH_H

#include "types.h"
#include "sound.h"

struct machine_context_s;

extern int speech_cart_enabled(void);

extern void speech_setup_context(struct machine_context_s *machine_context);
extern int speech_cmdline_options_init(void);

extern int speech_resources_init(void);
extern void speech_resources_shutdown(void);
extern void speech_shutdown(void);

extern BYTE speech_read(WORD addr);
extern BYTE speech_peek(WORD addr);
extern void speech_store(WORD addr, BYTE value);

extern void speech_sound_chip_init(void);

extern int speech_dump(void *ctx);

#endif
