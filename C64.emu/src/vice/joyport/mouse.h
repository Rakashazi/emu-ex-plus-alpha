/*
 * mouse.h - Common mouse handling (header)
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_MOUSE_H
#define VICE_MOUSE_H

#include <stdbool.h>

#include "types.h"
#include "snapshot.h"

typedef struct mouse_func_s {
    void (*mbl)(int pressed);
    void (*mbr)(int pressed);
    void (*mbm)(int pressed);
    void (*mbu)(int pressed);
    void (*mbd)(int pressed);
} mouse_func_t;

extern int mouse_resources_init(void);
extern int mouse_cmdline_options_init(void);
extern void mouse_init(void);
extern void mouse_reset(void);
extern void mouse_shutdown(void);

extern int _mouse_enabled;
extern int mouse_type;

extern void mouse_set_machine_parameter(long clock_rate);

extern void mouse_move(float dx, float dy);
extern void mouse_poll(void);

extern void mouse_get_raw_int16(int16_t *x, int16_t *y);
extern void mouse_get_last_int16(int16_t *x, int16_t *y);

extern int mouse_get_mouse_sx(void);
extern int mouse_get_mouse_sy(void);

extern int read_mouse_common_snapshot(snapshot_module_t *m);
extern int write_mouse_common_snapshot(snapshot_module_t *m);

#define MOUSE_TYPE_1351     0
#define MOUSE_TYPE_NEOS     1
#define MOUSE_TYPE_AMIGA    2
#define MOUSE_TYPE_PADDLE   3
#define MOUSE_TYPE_CX22     4
#define MOUSE_TYPE_ST       5
#define MOUSE_TYPE_SMART    6
#define MOUSE_TYPE_MICROMYS 7
#define MOUSE_TYPE_KOALAPAD 8
#define MOUSE_TYPE_MF_JOY   9
#define MOUSE_TYPE_NUM      10

#define PADDLES_INPUT_MOUSE    0
#define PADDLES_INPUT_JOY_AXIS 1

extern int mouse_type_to_id(int mt);
extern int mouse_id_to_type(int id);

#endif
