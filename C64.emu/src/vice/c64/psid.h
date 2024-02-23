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


int psid_resources_init(void);
int psid_cmdline_options_init(void);
void psid_shutdown(void);
int psid_load_file(const char* filename);
void psid_init_tune(int install_driver_hook);
void psid_set_tune(int tune);
int psid_tunes(int* default_tune);
int psid_basic_rsid_to_autostart(uint16_t *address, uint8_t **data, uint16_t *length);
void psid_init_driver(void);
unsigned int psid_increment_frames(void);
int reloc65(char** buf, int* fsize, int addr);
int psid_ui_set_tune(int, void *param);

#endif
