/*
 * traps.h - Allow VICE to replace ROM code with C function calls.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Jarkko Sonninen <sonninen@lut.fi>
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

#ifndef VICE_TRAPS_H
#define VICE_TRAPS_H

#include "mem.h"
#include "types.h"

/* JAM (0x02) is the trap-opcode.               */
/* Note: If you change this, you need to change */
/*       6510core.c and 6510dtvcore.c too!      */
#define TRAP_OPCODE 0x02

typedef struct trap_s {
    const char *name;
    uint16_t address;
    uint16_t resume_address;
    uint8_t check[3];
    int (*func)(void);
    read_func_t *readfunc;
    store_func_t *storefunc;
} trap_t;

void traps_init(void);
void traps_shutdown(void);
int traps_resources_init(void);
int traps_cmdline_options_init(void);
int traps_add(const trap_t *trap);
int traps_remove(const trap_t *trap);
void traps_refresh(void);
uint32_t traps_handler(void);
int traps_checkaddr(unsigned int addr);

#endif
