/*
 * psid.h - PSID file handling.
 *
 * Written by
 *  Dag Lem <resid@nimrod.no>
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

#ifndef VICE_PSID_H
#define VICE_PSID_H

#include "types.h"

extern int psid_resources_init(void);
extern int psid_cmdline_options_init(void);
extern void psid_shutdown(void);
extern int psid_load_file(const char* filename);
extern void psid_init_tune(int install_driver_hook);
extern void psid_set_tune(int tune);
extern int psid_tunes(int* default_tune);
extern int psid_basic_rsid_to_autostart(WORD *address, BYTE **data, WORD *length);
extern void psid_init_driver(void);
extern unsigned int psid_increment_frames(void);
extern int reloc65(char** buf, int* fsize, int addr);

#endif
