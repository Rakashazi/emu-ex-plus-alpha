/*
 * keyboard.h - Common keyboard emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Tibor Biczo <crown@mail.matav.hu>
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

#ifndef VICE_KEYBOARD_H
#define VICE_KEYBOARD_H

#include "types.h"

/* Maximum of keyboard array (CBM-II values
 * (8 for C64/VIC20, 10 for PET, 11 for C128; we need max).  */
#define KBD_ROWS    16

/* (This is actually the same for all the machines.) */
/* (All have 8, except CBM-II that has 6) */
#define KBD_COLS    8

/* negative rows/columns for extra keys */
#define KBD_ROW_JOY_KEYMAP_A    -1
#define KBD_ROW_JOY_KEYMAP_B    -2

#define KBD_ROW_RESTORE_1       -3
#define KBD_ROW_RESTORE_2       -3
#define KBD_COL_RESTORE_1        0
#define KBD_COL_RESTORE_2        1

#define KBD_ROW_4080COLUMN      -4
#define KBD_ROW_CAPSLOCK        -4
#define KBD_COL_4080COLUMN       0
#define KBD_COL_CAPSLOCK         1

#define KBD_ROW_JOY_KEYPAD      -5

/* joystick port attached keypad */
#define KBD_JOY_KEYPAD_ROWS      4
#define KBD_JOY_KEYPAD_COLS      5

#define KBD_JOY_KEYPAD_NUMKEYS   (KBD_JOY_KEYPAD_ROWS * KBD_JOY_KEYPAD_COLS)

#define KBD_MOD_LSHIFT      (1 << 0)
#define KBD_MOD_RSHIFT      (1 << 1)
#define KBD_MOD_LCTRL       (1 << 2)
#define KBD_MOD_RCTRL       (1 << 3)
#define KBD_MOD_LALT        (1 << 4)
#define KBD_MOD_RALT        (1 << 5)
#define KBD_MOD_SHIFTLOCK   (1 << 6)

struct snapshot_s;

/* custom keys for individual emulators (mostly c128)
    the function is called with "pressed" indicating what state we want to set
    the key to (pressed or released). the function returns the value the key was
    actually set to.
*/
typedef int (*key_custom_func_t)(int pressed);

typedef struct {
    char *name;
    key_custom_func_t func;     /* pointer to key handling function */
    int id;                     /* ID (see below) */
    int pressed;                /* is the key currently pressed? */
    int state;                  /* current state of the (toggle) switch */
    int *keysym;                /* pointer to variable keeping the host key symbol */
    int *keyflags;              /* pointer to variable keeping the host key flags */
} key_custom_info_t;

#define KBD_CUSTOM_NONE     0
#define KBD_CUSTOM_RESTORE1 1
#define KBD_CUSTOM_RESTORE2 2
#define KBD_CUSTOM_CAPS     3
#define KBD_CUSTOM_4080     4
#define KBD_CUSTOM_NUM      5

void keyboard_init(void);
void keyboard_shutdown(void);

int keyboard_resources_init(void);
int keyboard_cmdline_options_init(void);

int keyboard_snapshot_write_module(struct snapshot_s *s);
int keyboard_snapshot_read_module(struct snapshot_s *s);

void keyboard_set_keyarr(int row, int col, int value);
void keyboard_set_keyarr_any(int row, int col, int value);

void keyboard_clear_keymatrix(void);

void keyboard_event_playback(CLOCK offset, void *data);
void keyboard_restore_event_playback(CLOCK offset, void *data);
void keyboard_event_delayed_playback(void *data);
void keyboard_register_delay(unsigned int delay);
void keyboard_register_clear(void);

/* called by the ui */
void keyboard_key_pressed(signed long key, int mod);
void keyboard_key_released(signed long key, int mod);
void keyboard_key_clear(void);

/* shift/lock handling, the emulation may also call this */
void keyboard_set_shiftlock(int state);
int keyboard_get_shiftlock(void);

/* extra custom keys (see above) */
void keyboard_register_custom_key(int id, key_custom_func_t func, char *name, int *keysym, int *keyflags);
int keyboard_custom_key_get(int id);
int keyboard_custom_key_set(int id, int pressed);
int keyboard_custom_key_toggle(int id);

/* extra handler for keypad on the joystick port */
typedef void (*key_joy_keypad_func_t)(int row, int col, int pressed);
void keyboard_register_joy_keypad(key_joy_keypad_func_t func);

/* machine specific utility function that is called after the keyboard was latched */
typedef void (*keyboard_machine_func_t)(int *);
void keyboard_register_machine(keyboard_machine_func_t func);

/* switch to alternative set (x128) */
void keyboard_alternative_set(int alternative);

/* FIXME: These ugly externs should go away sooner or later.
   currently these two arrays are the interface to the emulation (eg CIA core) */
extern int keyarr[KBD_ROWS];
extern int rev_keyarr[KBD_COLS];

int keyboard_statusbar_enabled(void);

#endif
