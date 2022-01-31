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
#include "joyport.h" /* for JOYPORT_MAX_PORTS */

extern int joystick_init(void);
extern int joystick_resources_init(void);
extern int joystick_cmdline_options_init(void);

/* SDL-specific functions. */
extern int joy_sdl_init(void);
extern int joy_sdl_resources_init(void);
extern int joy_sdl_cmdline_options_init(void);

extern int joystick_check_set(signed long key, int keysetnum, unsigned int joyport);
extern int joystick_check_clr(signed long key, int keysetnum, unsigned int joyport);
extern void joystick_joypad_clear(void);

extern void joystick_set_axis_value(unsigned int index, uint8_t value);
extern uint8_t joystick_get_axis_value(unsigned int index);

extern void joystick_set_value_absolute(unsigned int joyport, uint16_t value);
extern void joystick_set_value_or(unsigned int joyport, uint16_t value);
extern void joystick_set_value_and(unsigned int joyport, uint16_t value);
extern void joystick_clear(unsigned int joyport);
extern void joystick_clear_all(void);

extern void joystick_event_playback(CLOCK offset, void *data);
extern void joystick_event_delayed_playback(void *data);
extern void joystick_register_delay(unsigned int delay);

extern void linux_joystick_init(void);
extern void usb_joystick_init(void);
extern void joy_hidlib_init(void);
extern void joy_hidlib_exit(void);
extern int win32_directinput_joystick_init(void);

extern uint16_t get_joystick_value(int index);

typedef void (*joystick_machine_func_t)(void);
extern void joystick_register_machine(joystick_machine_func_t func);

/* the mapping of real devices to emulated joystick ports */
extern int joystick_port_map[JOYPORT_MAX_PORTS];

extern void joystick_set_snes_mapping(int port);

/** \brief  Use keypad as predefined keys for joystick emulation
 *
 * Should always be defined for proper VICE, can be undef'ed for ports
 */
//#define COMMON_JOYKEYS

#define JOYSTICK_KEYSET_NUM_KEYS     16 /* 4 directions, 4 diagonals, 8 fire */

#ifdef COMMON_JOYKEYS

#define JOYSTICK_KEYSET_NUM          3
#define JOYSTICK_KEYSET_IDX_NUMBLOCK 0
#define JOYSTICK_KEYSET_IDX_A        1
#define JOYSTICK_KEYSET_IDX_B        2
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
    JOYSTICK_KEYSET_NE,
    JOYSTICK_KEYSET_FIRE2,
    JOYSTICK_KEYSET_FIRE3,
    JOYSTICK_KEYSET_FIRE4,
    JOYSTICK_KEYSET_FIRE5,
    JOYSTICK_KEYSET_FIRE6,
    JOYSTICK_KEYSET_FIRE7,
    JOYSTICK_KEYSET_FIRE8
} joystick_direction_t;
#endif

/* standard devices */
#define JOYDEV_NONE      0
#define JOYDEV_NUMPAD    1
#define JOYDEV_KEYSET1   2
#define JOYDEV_KEYSET2   3

#define JOYDEV_DEFAULT   JOYDEV_NUMPAD

#define JOYDEV_REALJOYSTICK_MIN (JOYDEV_KEYSET2 + 1)

typedef struct joystick_driver_s {
    void (*poll)(int, void*);
    void (*close)(void*);
} joystick_driver_t;

extern void register_joystick_driver(
   struct joystick_driver_s *driver,
   const char *jname,
   void *priv,
   int num_axes,
   int num_buttons,
   int num_hats);

typedef enum joystick_axis_value_e {
   JOY_AXIS_MIDDLE,
   JOY_AXIS_POSITIVE,
   JOY_AXIS_NEGATIVE
} joystick_axis_value_t;

extern void joy_axis_event(uint8_t joynum, uint8_t axis, joystick_axis_value_t value);
extern void joy_button_event(uint8_t joynum, uint8_t button, uint8_t value);
extern void joy_hat_event(uint8_t joynum, uint8_t button, uint8_t value);
extern void joystick(void);
extern void joystick_close(void);
extern void joystick_ui_reset_device_list(void);
extern const char *joystick_ui_get_next_device_name(int *id);

#define JOYSTICK_DIRECTION_UP    1
#define JOYSTICK_DIRECTION_DOWN  2
#define JOYSTICK_DIRECTION_LEFT  4
#define JOYSTICK_DIRECTION_RIGHT 8

#define JOYSTICK_AUTOFIRE_OFF   0
#define JOYSTICK_AUTOFIRE_ON    1

#define JOYSTICK_AUTOFIRE_MODE_PRESS       0
#define JOYSTICK_AUTOFIRE_MODE_PERMANENT   1

#define JOYSTICK_AUTOFIRE_SPEED_DEFAULT    10   /* default autofire speed, button will be on this many times per second */

#endif
