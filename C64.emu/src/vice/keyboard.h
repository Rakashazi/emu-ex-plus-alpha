/*
 * keyboard.h - Common keyboard emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Tibor Biczo <crown@mail.matav.hu>
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

#include "joystick.h"

/* Maximum of keyboard array (CBM-II values
 * (8 for C64/VIC20, 10 for PET, 11 for C128; we need max).  */
#define KBD_ROWS    16

/* (This is actually the same for all the machines.) */
/* (All have 8, except CBM-II that has 6) */
#define KBD_COLS    8

struct snapshot_s;

extern void keyboard_init(void);
extern void keyboard_shutdown(void);
extern void keyboard_set_keyarr(int row, int col, int value);
extern void keyboard_set_keyarr_any(int row, int col, int value);
extern void keyboard_clear_keymatrix(void);
extern void keyboard_event_playback(CLOCK offset, void *data);
extern void keyboard_restore_event_playback(CLOCK offset, void *data);
extern int keyboard_snapshot_write_module(struct snapshot_s *s);
extern int keyboard_snapshot_read_module(struct snapshot_s *s);
extern void keyboard_event_delayed_playback(void *data);
extern void keyboard_register_delay(unsigned int delay);
extern void keyboard_register_clear(void);
extern void keyboard_set_map_any(signed long sym, int row, int col, int shift);
extern void keyboard_set_unmap_any(signed long sym);

extern int keyboard_set_keymap_index(int vak, void *param);
extern int keyboard_set_keymap_file(const char *val, void *param);
extern int keyboard_keymap_dump(const char *filename);

extern void keyboard_key_pressed(signed long key);
extern void keyboard_key_released(signed long key);
extern void keyboard_key_clear(void);

typedef void (*key_ctrl_column4080_func_t)(void);
extern void keyboard_register_column4080_key(key_ctrl_column4080_func_t func);

typedef void (*key_ctrl_caps_func_t)(void);
extern void keyboard_register_caps_key(key_ctrl_caps_func_t func);

typedef void (*keyboard_machine_func_t)(int *);
extern void keyboard_register_machine(keyboard_machine_func_t func);

extern void keyboard_alternative_set(int alternative);

/* These ugly externs will go away sooner or later.  */
extern int keyarr[KBD_ROWS];
extern int rev_keyarr[KBD_COLS];
extern int keyboard_shiftlock;

extern BYTE joystick_value[JOYSTICK_NUM + 1];

extern int c64_kbd_init(void);
extern int c128_kbd_init(void);
extern int vic20_kbd_init(void);
extern int pet_kbd_init(void);
extern int plus4_kbd_init(void);
extern int cbm2_kbd_init(void);

extern int kbd_cmdline_options_init(void);
extern int kbd_resources_init(void);

extern int pet_kbd_cmdline_options_init(void);
extern int pet_kbd_resources_init(void);

#endif
