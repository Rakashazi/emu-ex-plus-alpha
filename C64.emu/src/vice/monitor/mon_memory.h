/*
 * mon_memory.h - The VICE built-in monitor memory functions.
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

#ifndef VICE_MON_MEMORY_H
#define VICE_MON_MEMORY_H

#include "montypes.h"

typedef enum mon_display_format_e {
    DF_PETSCII,
    DF_SCREEN_CODE
} mon_display_format_t;

extern void mon_memory_move(MON_ADDR start_addr, MON_ADDR end_addr,
                            MON_ADDR dest);
extern void mon_memory_compare(MON_ADDR start_addr, MON_ADDR end_addr,
                               MON_ADDR dest);
extern void mon_memory_fill(MON_ADDR start_addr, MON_ADDR end_addr,
                            unsigned char *data);
extern void mon_memory_hunt(MON_ADDR start_addr, MON_ADDR end_addr,
                            unsigned char *data);
extern void mon_memory_display(int radix_type, MON_ADDR start_addr,
                               MON_ADDR end_addr, mon_display_format_t format);
extern void mon_memory_display_data(MON_ADDR start_addr, MON_ADDR end_addr,
                                    unsigned int x, unsigned int y);

#endif
