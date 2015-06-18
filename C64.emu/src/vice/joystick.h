/*
 * joystick.h - Common joystick emulation.
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

#ifndef VICE_JOYSTICK_H
#define VICE_JOYSTICK_H

#include "types.h"

struct snapshot_s;

extern int joystick_init(void);
extern int joystick_resources_init(void);
extern int joystick_extra_init_resources(void);

extern int joystick_cmdline_options_init(void);
extern int joystick_arch_cmdline_options_init(void);

extern int joystick_check_set(signed long key, int keysetnum, unsigned int joyport);
extern int joystick_check_clr(signed long key, int keysetnum, unsigned int joyport);
extern void joystick_joypad_clear(void);

extern void joystick_set_value_absolute(unsigned int joyport, BYTE value);
extern void joystick_set_value_or(unsigned int joyport, BYTE value);
extern void joystick_set_value_and(unsigned int joyport, BYTE value);
extern void joystick_clear(unsigned int joyport);
extern void joystick_clear_all(void);

extern void joystick_event_playback(CLOCK offset, void *data);
extern void joystick_event_delayed_playback(void *data);
extern void joystick_register_delay(unsigned int delay);

extern BYTE get_joystick_value(int index);

typedef void (*joystick_machine_func_t)(void);
extern void joystick_register_machine(joystick_machine_func_t func);

extern int joystick_snapshot_write_module(struct snapshot_s *s);
extern int joystick_snapshot_read_module(struct snapshot_s *s);

/*! the number of joysticks that can be attached to the emu */
#define JOYSTICK_NUM 4

/* virtual joystick mapping */
extern int joystick_port_map[JOYSTICK_NUM];

#ifdef COMMON_KBD
#define JOYSTICK_KEYSET_NUM 3
#define JOYSTICK_KEYSET_NUM_KEYS 9
#define JOYSTICK_KEYSET_IDX_NUMBLOCK 0
#define JOYSTICK_KEYSET_IDX_A 1
#define JOYSTICK_KEYSET_IDX_B 2
extern int joykeys[JOYSTICK_KEYSET_NUM][JOYSTICK_KEYSET_NUM_KEYS];

/* several things depend on the order/exact values of the members in this enum,
 * DO NOT CHANGE!
 */
typedef enum {
    JOYSTICK_KEYSET_FIRE,
    JOYSTICK_KEYSET_SW,
    JOYSTICK_KEYSET_S,
    JOYSTICK_KEYSET_SE,
    JOYSTICK_KEYSET_W,
    JOYSTICK_KEYSET_E,
    JOYSTICK_KEYSET_NW,
    JOYSTICK_KEYSET_N,
    JOYSTICK_KEYSET_NE
} joystick_direction_t;
#endif

#endif
