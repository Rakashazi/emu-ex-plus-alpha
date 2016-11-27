/*
 * joystick.h - Common joystick emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_JOYSTICK_H
#define VICE_JOYSTICK_H

#include "types.h"

extern int joystick_init(void);
extern int joystick_resources_init(void);
extern int joystick_cmdline_options_init(void);

/* These joy_arch_* functions need to be defined in every port. */
extern int joy_arch_init(void);
extern int joy_arch_resources_init(void);
extern int joy_arch_cmdline_options_init(void);
extern int joy_arch_set_device(int port_idx, int new_dev);

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

/*! the number of joysticks that can be attached to the emu */
#define JOYSTICK_NUM 5

/* the values used internally to represent joystick state
FIXME: this is only an extern because of 
src/c64dtv/c64dtvcia1.c and
src/c64dtv/hummeradc.c */
extern BYTE joystick_value[JOYSTICK_NUM + 1];

/* the mapping of real devices to emulated joystick ports */
extern int joystick_port_map[JOYSTICK_NUM];

#if !defined(EMUFRAMEWORK_BUILD) && (!defined(__OS2__) || defined(USE_SDLUI) || defined(USE_SDLUI2))
#define COMMON_JOYKEYS

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
