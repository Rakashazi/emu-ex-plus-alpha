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
#if (defined USE_SDLUI ||defined USE_SDL2UI)
#include "uimenu.h"
#endif

int joystick_init(void);
int joystick_resources_init(void);
int joystick_cmdline_options_init(void);

/* SDL-specific functions. */
int joy_sdl_init(void);
int joy_sdl_resources_init(void);
int joy_sdl_cmdline_options_init(void);

int joystick_check_set(signed long key, int keysetnum, unsigned int joyport);
int joystick_check_clr(signed long key, int keysetnum, unsigned int joyport);
void joystick_joypad_clear(void);

void joystick_set_axis_value(unsigned int index, unsigned int axis, uint8_t value);
uint8_t joystick_get_axis_value(unsigned int port, unsigned int pot);

void joystick_set_value_absolute(unsigned int joyport, uint16_t value);
void joystick_set_value_or(unsigned int joyport, uint16_t value);
void joystick_set_value_and(unsigned int joyport, uint16_t value);
void joystick_clear(unsigned int joyport);
void joystick_clear_all(void);

void joystick_event_playback(CLOCK offset, void *data);
void joystick_event_delayed_playback(void *data);
void joystick_register_delay(unsigned int delay);

int joystick_joyport_register(void);

void linux_joystick_init(void);
void usb_joystick_init(void);
void joy_hidlib_init(void);
void joy_hidlib_exit(void);
int win32_directinput_joystick_init(void);

uint16_t get_joystick_value(int index);

typedef void (*joystick_machine_func_t)(void);

void joystick_register_machine(joystick_machine_func_t func);

/* the mapping of real devices to emulated joystick ports */
extern int joystick_port_map[JOYPORT_MAX_PORTS];

void joystick_set_snes_mapping(int port);

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

void register_joystick_driver(
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

/** \brief  Hat direction joystick input index values
 */
typedef enum joystick_hat_direction_e {
    JOY_HAT_UP    = 0,
    JOY_HAT_DOWN  = 1,
    JOY_HAT_LEFT  = 2,
    JOY_HAT_RIGHT = 3
} joystick_hat_direction_t;

/* Actions to perform on joystick input */
typedef enum joystick_action_e {
    JOY_ACTION_NONE = 0,

    /* Joystick movement or button press */
    JOY_ACTION_JOYSTICK = 1,

    /* Keyboard key press */
    JOY_ACTION_KEYBOARD = 2,

    /* Map button */
    JOY_ACTION_MAP = 3,

    /* (De)Activate UI */
    JOY_ACTION_UI_ACTIVATE = 4,

    /* Call UI function */
    JOY_ACTION_UI_FUNCTION = 5,

    /* Joystick axis used for potentiometers */
    JOY_ACTION_POT_AXIS = 6,

    JOY_ACTION_MAX = JOY_ACTION_POT_AXIS
} joystick_action_t;

/** \brief  Joystick input types used by the vjm files
 */
typedef enum joystick_input_e {
    JOY_INPUT_AXIS   = 0,   /**< map host axis input */
    JOY_INPUT_BUTTON = 1,   /**< map host button input */
    JOY_INPUT_HAT    = 2,   /**< map host hat input */
    JOY_INPUT_BALL   = 3,   /**< map host ball input */

    JOY_INPUT_MAX    = JOY_INPUT_BALL
} joystick_input_t;

/* Input mapping for each direction/button/etc */
typedef struct joystick_mapping_s {
    /* Action to perform */
    joystick_action_t action;

    union {
        uint16_t joy_pin;

        /* key[0] = row, key[1] = column, key[1] = flags */
        int key[3];
        int ui_action;
    } value;
} joystick_mapping_t;

void joy_axis_event(uint8_t joynum, uint8_t axis, joystick_axis_value_t value);
void joy_button_event(uint8_t joynum, uint8_t button, uint8_t value);
void joy_hat_event(uint8_t joynum, uint8_t button, uint8_t value);
void joystick(void);
void joystick_close(void);
void joystick_resources_shutdown(void);
void joystick_ui_reset_device_list(void);
const char *joystick_ui_get_next_device_name(int *id);
int joy_arch_mapping_dump(const char *filename);
int joy_arch_mapping_load(const char *filename);
joystick_axis_value_t joy_axis_prev(uint8_t joynum, uint8_t axis);
char *get_joy_pot_mapping_string(int joystick_device_num, int pot);
char *get_joy_pin_mapping_string(int joystick_device, int pin);
char *get_joy_extra_mapping_string(int which);
joystick_mapping_t *joy_get_axis_mapping(uint8_t joynum, uint8_t axis, joystick_axis_value_t value, joystick_axis_value_t *prev);
joystick_mapping_t *joy_get_axis_mapping_not_setting_value(uint8_t joynum, uint8_t axis, joystick_axis_value_t value);
joystick_mapping_t *joy_get_button_mapping(uint8_t joynum, uint8_t button, uint8_t value, uint8_t *prev);
joystick_mapping_t *joy_get_button_mapping_not_setting_value(uint8_t joynum, uint8_t button, uint8_t value);
joystick_axis_value_t joy_hat_prev(uint8_t joynum, uint8_t hat);
joystick_mapping_t *joy_get_hat_mapping(uint8_t joynum, uint8_t hat, uint8_t value, uint8_t *prev);
joystick_mapping_t *joy_get_hat_mapping_not_setting_value(uint8_t joynum, uint8_t hat, uint8_t value);
void joy_set_pot_mapping(int joystick_device_num, int axis, int pot);
void joy_delete_pin_mapping(int joystick_device, int pin);
void joy_delete_pot_mapping(int joystick_device, int pot);
#if (defined USE_SDLUI ||defined USE_SDL2UI)
void joy_delete_extra_mapping(int type);
#endif

#define JOYSTICK_DIRECTION_UP    1
#define JOYSTICK_DIRECTION_DOWN  2
#define JOYSTICK_DIRECTION_LEFT  4
#define JOYSTICK_DIRECTION_RIGHT 8

#define JOYSTICK_AUTOFIRE_OFF   0
#define JOYSTICK_AUTOFIRE_ON    1

#define JOYSTICK_AUTOFIRE_MODE_PRESS       0    /* autofire only when fire is pressed */
#define JOYSTICK_AUTOFIRE_MODE_PERMANENT   1    /* autofire only when fire is NOT pressed */

#define JOYSTICK_AUTOFIRE_SPEED_DEFAULT    10   /* default autofire speed, button will be on this many times per second */
#define JOYSTICK_AUTOFIRE_SPEED_MIN        1
#define JOYSTICK_AUTOFIRE_SPEED_MAX        255

#endif
