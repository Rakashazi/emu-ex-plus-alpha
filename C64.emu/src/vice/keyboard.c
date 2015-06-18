/*
 * keyboard.c - Common keyboard emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Based on old code by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Bernhard Kuhn <kuhn@eikon.e-technik.tu-muenchen.de>
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef RAND_MAX
#include <limits.h>
#define RAND_MAX INT_MAX
#endif

#include "alarm.h"
#include "archdep.h"
#include "cmdline.h"
#include "joystick.h"
#include "joy.h"
#include "kbd.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "network.h"
#include "resources.h"
#include "snapshot.h"
#include "sysfile.h"
#include "translate.h"
#include "types.h"
#include "util.h"
#include "vice-event.h"

/* #define DBGKBD */

#ifdef DBGKBD
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#define KEYBOARD_RAND() lib_unsigned_rand(1, machine_get_cycles_per_frame())

/* Keyboard array.  */
int keyarr[KBD_ROWS];
int rev_keyarr[KBD_COLS];

/* Shift lock state.  */
int keyboard_shiftlock = 0;

/* Keyboard status to be latched into the keyboard array.  */
static int latch_keyarr[KBD_ROWS];
static int latch_rev_keyarr[KBD_COLS];

static int network_keyarr[KBD_ROWS];
static int network_rev_keyarr[KBD_COLS];

static alarm_t *keyboard_alarm = NULL;

static log_t keyboard_log = LOG_DEFAULT;

static keyboard_machine_func_t keyboard_machine_func = NULL;

static CLOCK keyboard_delay;

static int keyboard_clear = 0;

static alarm_t *restore_alarm = NULL; /* restore key alarm context */

static void keyboard_latch_matrix(CLOCK offset)
{
    if (network_connected()) {
        memcpy(keyarr, network_keyarr, sizeof(keyarr));
        memcpy(rev_keyarr, network_rev_keyarr, sizeof(rev_keyarr));
    } else {
        memcpy(keyarr, latch_keyarr, sizeof(keyarr));
        memcpy(rev_keyarr, latch_rev_keyarr, sizeof(rev_keyarr));
    }
    if (keyboard_machine_func != NULL) {
        keyboard_machine_func(keyarr);
    }
}

static int keyboard_set_latch_keyarr(int row, int col, int value)
{
    if (row < 0 || col < 0) {
        return -1;
    }
    if (value) {
        latch_keyarr[row] |= 1 << col;
        latch_rev_keyarr[col] |= 1 << row;
    } else {
        latch_keyarr[row] &= ~(1 << col);
        latch_rev_keyarr[col] &= ~(1 << row);
    }

    return 0;
}

/*-----------------------------------------------------------------------*/
#ifdef COMMON_KBD
static void keyboard_key_clear_internal(void);
#endif

static void keyboard_event_record(void)
{
    event_record(EVENT_KEYBOARD_MATRIX, (void *)keyarr, sizeof(keyarr));
}

void keyboard_event_playback(CLOCK offset, void *data)
{
    int row, col;

    memcpy(latch_keyarr, data, sizeof(keyarr));

    for (row = 0; row < KBD_ROWS; row++) {
        for (col = 0; col < KBD_COLS; col++) {
            keyboard_set_latch_keyarr(row, col, latch_keyarr[row] & (1 << col));
        }
    }

    keyboard_latch_matrix(offset);
}

void keyboard_restore_event_playback(CLOCK offset, void *data)
{
    machine_set_restore_key((int)(*(DWORD *)data));
}

static void keyboard_latch_handler(CLOCK offset, void *data)
{
    alarm_unset(keyboard_alarm);
    alarm_context_update_next_pending(keyboard_alarm->context);

    keyboard_latch_matrix(offset);

    keyboard_event_record();
}

void keyboard_event_delayed_playback(void *data)
{
    int row, col;

    memcpy(network_keyarr, data, sizeof(network_keyarr));

    for (row = 0; row < KBD_ROWS; row++) {
        for (col = 0; col < KBD_COLS; col++) {
            if (network_keyarr[row] & (1 << col)) {
                network_rev_keyarr[col] |= 1 << row;
            } else {
                network_rev_keyarr[col] &= ~(1 << row);
            }
        }
    }

    if (keyboard_clear == 1) {
#ifdef COMMON_KBD
        keyboard_key_clear_internal();
#endif
        keyboard_clear = 0;
    }

    alarm_set(keyboard_alarm, maincpu_clk + keyboard_delay);
}
/*-----------------------------------------------------------------------*/

void keyboard_set_keyarr(int row, int col, int value)
{
    if (keyboard_set_latch_keyarr(row, col, value) < 0) {
        return;
    }

    alarm_set(keyboard_alarm, maincpu_clk + KEYBOARD_RAND());
}

void keyboard_clear_keymatrix(void)
{
    memset(keyarr, 0, sizeof(keyarr));
    memset(rev_keyarr, 0, sizeof(rev_keyarr));
    memset(latch_keyarr, 0, sizeof(latch_keyarr));
    memset(latch_rev_keyarr, 0, sizeof(latch_rev_keyarr));
}

void keyboard_register_machine(keyboard_machine_func_t func)
{
    keyboard_machine_func = func;
}

void keyboard_register_delay(unsigned int delay)
{
    keyboard_delay = delay;
}

void keyboard_register_clear(void)
{
    keyboard_clear = 1;
}
/*-----------------------------------------------------------------------*/

#ifdef COMMON_KBD

enum shift_type {
    NO_SHIFT = 0,             /* Key is not shifted. */
    VIRTUAL_SHIFT = (1 << 0), /* The key needs a shift on the real machine. */
    LEFT_SHIFT = (1 << 1),    /* Key is left shift. */
    RIGHT_SHIFT = (1 << 2),   /* Key is right shift. */
    ALLOW_SHIFT = (1 << 3),   /* Allow key to be shifted. */
    DESHIFT_SHIFT = (1 << 4), /* Although SHIFT might be pressed, do not
                                 press shift on the real machine. */
    ALLOW_OTHER = (1 << 5),   /* Allow another key code to be assigned if
                                 SHIFT is pressed. */
    SHIFT_LOCK = (1 << 6),    /* Key is shift lock. */

    ALT_MAP  = (1 << 8)       /* Key is used for an alternative keyboard
                                 mapping */
};

struct keyboard_conv_s {
    signed long sym;
    int row;
    int column;
    enum shift_type shift;
    char *comment;
};
typedef struct keyboard_conv_s keyboard_conv_t;

/* Is the resource code ready to load the keymap?  */
static int load_keymap_ok = 0;

/* Memory size of array in sizeof(keyconv_t), 0 = static.  */
static int keyc_mem = 0;

/* Number of convs used in sizeof(keyconv_t).  */
static int keyc_num = 0;

/* Two possible restore keys.  */
static signed long key_ctrl_restore1 = -1;
static signed long key_ctrl_restore2 = -1;

/* 40/80 column key.  */
static signed long key_ctrl_column4080 = -1;
static key_ctrl_column4080_func_t key_ctrl_column4080_func = NULL;

/* CAPS (ASCII/DIN) key.  */
static signed long key_ctrl_caps = -1;
static key_ctrl_caps_func_t key_ctrl_caps_func = NULL;

/* Is an alternative mapping active? */
static int key_alternative = 0;

static keyboard_conv_t *keyconvmap = NULL;

static int kbd_lshiftrow;
static int kbd_lshiftcol;
static int kbd_rshiftrow;
static int kbd_rshiftcol;

#define KEY_NONE   0
#define KEY_RSHIFT 1
#define KEY_LSHIFT 2

static int vshift = KEY_NONE;
static int shiftl = KEY_NONE;

/*-----------------------------------------------------------------------*/

static int left_shift_down, right_shift_down, virtual_shift_down;
static int key_latch_row, key_latch_column;

static void keyboard_key_deshift(void)
{
    keyboard_set_latch_keyarr(kbd_lshiftrow, kbd_lshiftcol, 0);
    keyboard_set_latch_keyarr(kbd_rshiftrow, kbd_rshiftcol, 0);
}

static void keyboard_key_shift(void)
{
    if (left_shift_down > 0
        || (virtual_shift_down > 0 && vshift == KEY_LSHIFT)
        || (keyboard_shiftlock > 0 && shiftl == KEY_LSHIFT)) {
        keyboard_set_latch_keyarr(kbd_lshiftrow, kbd_lshiftcol, 1);
    }
    if (right_shift_down > 0
        || (virtual_shift_down > 0 && vshift == KEY_RSHIFT)
        || (keyboard_shiftlock > 0 && shiftl == KEY_RSHIFT)) {
        keyboard_set_latch_keyarr(kbd_rshiftrow, kbd_rshiftcol, 1);
    }
}

static int keyboard_key_pressed_matrix(int row, int column, int shift)
{
    if (row >= 0) {
        key_latch_row = row;
        key_latch_column = column;

        if (shift == NO_SHIFT || shift & DESHIFT_SHIFT) {
            keyboard_key_deshift();
        } else {
            if (shift & VIRTUAL_SHIFT) {
                virtual_shift_down = 1;
            }
            if (shift & LEFT_SHIFT) {
                left_shift_down = 1;
            }
            if (shift & RIGHT_SHIFT) {
                right_shift_down = 1;
            }
            if (shift & SHIFT_LOCK) {
                keyboard_shiftlock = 1;
            }
            keyboard_key_shift();
        }

        return 1;
    }

    return 0;
}

/*
    restore key handling. restore key presses are distributed randomly
    across a frame.

    FIXME: when network play is active this is not the case yet
*/

static int restore_raw = 0;
static int restore_delayed = 0;
static int restore_quick_release = 0;

static void restore_alarm_triggered(CLOCK offset, void *data)
{
    DWORD event_data;
    alarm_unset(restore_alarm);

    event_data = (DWORD)restore_delayed;
    machine_set_restore_key(restore_delayed);
    event_record(EVENT_KEYBOARD_RESTORE, (void*)&event_data, sizeof(DWORD));
    restore_delayed = 0;

    if (restore_quick_release) {
        restore_quick_release = 0;
        alarm_set(restore_alarm, maincpu_clk + KEYBOARD_RAND());
    }
}

static void keyboard_restore_pressed(void)
{
    DWORD event_data;
    event_data = (DWORD)1;
    if (network_connected()) {
        network_event_record(EVENT_KEYBOARD_RESTORE, (void*)&event_data, sizeof(DWORD));
    } else {
        if (restore_raw == 0) {
            restore_delayed = 1;
            restore_quick_release = 0;
            alarm_set(restore_alarm, maincpu_clk + KEYBOARD_RAND());
        }
    }
    restore_raw = 1;
}

static void keyboard_restore_released(void)
{
    DWORD event_data;
    event_data = (DWORD)0;
    if (network_connected()) {
        network_event_record(EVENT_KEYBOARD_RESTORE, (void*)&event_data, sizeof(DWORD));
    } else {
        if (restore_raw == 1) {
            if (restore_delayed) {
                restore_quick_release = 1;
            } else {
                alarm_set(restore_alarm, maincpu_clk + KEYBOARD_RAND());
            }
        }
    }
    restore_raw = 0;
}

void keyboard_key_pressed(signed long key)
{
    int i, latch;

    if (event_playback_active()) {
        return;
    }

    /* Restore */
    if (((key == key_ctrl_restore1) || (key == key_ctrl_restore2))
        && machine_has_restore_key()) {
        keyboard_restore_pressed();
        return;
    }

    if (key == key_ctrl_column4080) {
        if (key_ctrl_column4080_func != NULL) {
            key_ctrl_column4080_func();
        }
        return;
    }

    if (key == key_ctrl_caps) {
        if (key_ctrl_caps_func != NULL) {
            key_ctrl_caps_func();
        }
        return;
    }

    for (i = 0; i < JOYSTICK_NUM; ++i) {
        if (joystick_port_map[i] == JOYDEV_NUMPAD
            || joystick_port_map[i] == JOYDEV_KEYSET1
            || joystick_port_map[i] == JOYDEV_KEYSET2) {
            if (joystick_check_set(key, joystick_port_map[i] - JOYDEV_NUMPAD, 1 + i)) {
                return;
            }
        }
    }

    if (keyconvmap == NULL) {
        return;
    }

    latch = 0;

    for (i = 0; i < keyc_num; ++i) {
        if (key == keyconvmap[i].sym) {
            if ((keyconvmap[i].shift & ALT_MAP) && !key_alternative) {
                continue;
            }

            if (keyboard_key_pressed_matrix(keyconvmap[i].row,
                                            keyconvmap[i].column,
                                            keyconvmap[i].shift)) {
                latch = 1;
                if (!(keyconvmap[i].shift & ALLOW_OTHER)
                    || (right_shift_down + left_shift_down) == 0) {
                    break;
                }
            }
        }
    }

    if (latch) {
        keyboard_set_latch_keyarr(key_latch_row, key_latch_column, 1);
        if (network_connected()) {
            CLOCK keyboard_delay = KEYBOARD_RAND();
            network_event_record(EVENT_KEYBOARD_DELAY, (void *)&keyboard_delay, sizeof(keyboard_delay));
            network_event_record(EVENT_KEYBOARD_MATRIX, (void *)latch_keyarr, sizeof(latch_keyarr));
        } else {
            alarm_set(keyboard_alarm, maincpu_clk + KEYBOARD_RAND());
        }
    }
}

static int keyboard_key_released_matrix(int row, int column, int shift)
{
    int skip_release = 0;

    if (row >= 0) {
        key_latch_row = row;
        key_latch_column = column;

        if (shift & VIRTUAL_SHIFT) {
            virtual_shift_down = 0;
        }
        if (shift & LEFT_SHIFT) {
            left_shift_down = 0;
            if (keyboard_shiftlock && (shiftl == KEY_LSHIFT)) {
                skip_release = 1;
            }
        }
        if (shift & RIGHT_SHIFT) {
            right_shift_down = 0;
            if (keyboard_shiftlock && (shiftl == KEY_RSHIFT)) {
                skip_release = 1;
            }
        }
        if (shift & SHIFT_LOCK) {
            keyboard_shiftlock = 0;
            if (((shiftl == KEY_RSHIFT) && right_shift_down)
                || ((shiftl == KEY_LSHIFT) && left_shift_down)) {
                skip_release = 1;
            }
        }

        /* Map shift keys. */
        if (right_shift_down > 0
            || (virtual_shift_down > 0 && vshift == KEY_RSHIFT)
            || (keyboard_shiftlock > 0 && shiftl == KEY_RSHIFT)) {
            keyboard_set_latch_keyarr(kbd_rshiftrow, kbd_rshiftcol, 1);
        } else {
            keyboard_set_latch_keyarr(kbd_rshiftrow, kbd_rshiftcol, 0);
        }

        if (left_shift_down > 0
            || (virtual_shift_down > 0 && vshift == KEY_LSHIFT)
            || (keyboard_shiftlock > 0 && shiftl == KEY_LSHIFT)) {
            keyboard_set_latch_keyarr(kbd_lshiftrow, kbd_lshiftcol, 1);
        } else {
            keyboard_set_latch_keyarr(kbd_lshiftrow, kbd_lshiftcol, 0);
        }

        return !skip_release;
    }

    return 0;
}

void keyboard_key_released(signed long key)
{
    int i, latch;

    if (event_playback_active()) {
        return;
    }

    /* Restore */
    if (((key == key_ctrl_restore1) || (key == key_ctrl_restore2))
        && machine_has_restore_key()) {
        keyboard_restore_released();
        return;
    }

    for (i = 0; i < JOYSTICK_NUM; ++i) {
        if (joystick_port_map[i] == JOYDEV_NUMPAD
            || joystick_port_map[i] == JOYDEV_KEYSET1
            || joystick_port_map[i] == JOYDEV_KEYSET2) {
            if (joystick_check_clr(key, joystick_port_map[i] - JOYDEV_NUMPAD, 1 + i)) {
                return;
            }
        }
    }

    if (keyconvmap == NULL) {
        return;
    }

    latch = 0;

    for (i = 0; i < keyc_num; i++) {
        if (key == keyconvmap[i].sym) {
            if ((keyconvmap[i].shift & ALT_MAP) && !key_alternative) {
                continue;
            }

            if (keyboard_key_released_matrix(keyconvmap[i].row,
                                             keyconvmap[i].column,
                                             keyconvmap[i].shift)) {
                latch = 1;
                keyboard_set_latch_keyarr(keyconvmap[i].row,
                                          keyconvmap[i].column, 0);
                if (!(keyconvmap[i].shift & ALLOW_OTHER)
                    /*|| (right_shift_down + left_shift_down) == 0*/) {
                    break;
                }
            }
        }
    }

    if (latch) {
        if (network_connected()) {
            CLOCK keyboard_delay = KEYBOARD_RAND();
            network_event_record(EVENT_KEYBOARD_DELAY, (void *)&keyboard_delay, sizeof(keyboard_delay));
            network_event_record(EVENT_KEYBOARD_MATRIX, (void *)latch_keyarr, sizeof(latch_keyarr));
        } else {
            alarm_set(keyboard_alarm, maincpu_clk + KEYBOARD_RAND());
        }
    }
}

static void keyboard_key_clear_internal(void)
{
    keyboard_clear_keymatrix();
    joystick_clear_all();
    virtual_shift_down = left_shift_down = right_shift_down = keyboard_shiftlock = 0;
    joystick_joypad_clear();
}

void keyboard_key_clear(void)
{
    if (event_playback_active()) {
        return;
    }

    if (network_connected()) {
        network_event_record(EVENT_KEYBOARD_CLEAR, NULL, 0);
        return;
    }

    keyboard_key_clear_internal();
}

/* FIXME: joystick mapping not handled here, is it needed? */
void keyboard_set_keyarr_any(int row, int col, int value)
{
    signed long sym;

    if (row < 0) {
        if (row == -3 && col == 0) {
            sym = key_ctrl_restore1;
        } else if (row == -3 && col == 1) {
            sym = key_ctrl_restore2;
        } else if (row == -4 && col == 0) {
            sym = key_ctrl_column4080;
        } else if (row == -4 && col == 1) {
            sym = key_ctrl_caps;
        } else {
            return;
        }

        if (value) {
            keyboard_key_pressed(sym);
        } else {
            keyboard_key_released(sym);
        }
    } else {
        keyboard_set_keyarr(row, col, value);
    }
}

/*-----------------------------------------------------------------------*/

void keyboard_alternative_set(int alternative)
{
    key_alternative = alternative;
}

/*-----------------------------------------------------------------------*/

static void keyboard_keyconvmap_alloc(void)
{
#define KEYCONVMAP_SIZE_MIN 150

    keyconvmap = lib_malloc(KEYCONVMAP_SIZE_MIN * sizeof(keyboard_conv_t));
    keyc_num = 0;
    keyc_mem = KEYCONVMAP_SIZE_MIN - 1;
    keyconvmap[0].sym = ARCHDEP_KEYBOARD_SYM_NONE;
}

static void keyboard_keyconvmap_free(void)
{
    lib_free(keyconvmap);
    keyconvmap = NULL;
}

static void keyboard_keyconvmap_realloc(void)
{
    keyc_mem += keyc_mem / 2;
    keyconvmap = lib_realloc(keyconvmap, (keyc_mem + 1) * sizeof(keyboard_conv_t));
}

/*-----------------------------------------------------------------------*/

static int keyboard_parse_keymap(const char *filename, int child);

static void keyboard_keyword_lshift(void)
{
    char *p;

    p = strtok(NULL, " \t,");
    if (p != NULL) {
        kbd_lshiftrow = atoi(p);
        p = strtok(NULL, " \t,");
        if (p != NULL) {
            kbd_lshiftcol = atoi(p);
        }
    }
}

static void keyboard_keyword_rshift(void)
{
    char *p;

    p = strtok(NULL, " \t,");
    if (p != NULL) {
        kbd_rshiftrow = atoi(p);
        p = strtok(NULL, " \t,");
        if (p != NULL) {
            kbd_rshiftcol = atoi(p);
        }
    }
}

static int keyboard_keyword_vshiftl(void)
{
    char *p;

    p = strtok(NULL, " \t,\r");

    if (!strcmp(p, "RSHIFT")) {
        return KEY_RSHIFT;
    } else if (!strcmp(p, "LSHIFT")) {
        return KEY_LSHIFT;
    } else {
        return KEY_NONE;
    }
}

static void keyboard_keyword_vshift(void)
{
    vshift = keyboard_keyword_vshiftl();
}

static void keyboard_keyword_shiftl(void)
{
    shiftl = keyboard_keyword_vshiftl();
}

static void keyboard_keyword_clear(void)
{
    keyc_num = 0;
    keyconvmap[0].sym = ARCHDEP_KEYBOARD_SYM_NONE;
    key_ctrl_restore1 = -1;
    key_ctrl_restore2 = -1;
    key_ctrl_caps = -1;
    key_ctrl_column4080 = -1;
    vshift = KEY_NONE;
    shiftl = KEY_NONE;
}

static void keyboard_keyword_include(void)
{
    char *key;

    key = strtok(NULL, " \t");
    keyboard_parse_keymap(key, 1);
}

static void keyboard_keysym_undef(signed long sym)
{
    int i;

    if (sym >= 0) {
        for (i = 0; i < keyc_num; i++) {
            if (keyconvmap[i].sym == sym) {
                if (keyc_num) {
                    keyconvmap[i] = keyconvmap[--keyc_num];
                }
                keyconvmap[keyc_num].sym = ARCHDEP_KEYBOARD_SYM_NONE;
                break;
            }
        }
    }
}

static void keyboard_keyword_undef(void)
{
    char *key;

    /* TODO: this only unsets from the main table, not for joysticks 
     *       inventing another keyword to reset joysticks only is perhaps a
     *       good idea.
     */
    key = strtok(NULL, " \t");
    keyboard_keysym_undef(kbd_arch_keyname_to_keynum(key));
}

static void keyboard_parse_keyword(char *buffer)
{
    char *key;

    key = strtok(buffer + 1, " \t:");

    if (!strcmp(key, "LSHIFT")) {
        keyboard_keyword_lshift();
    } else if (!strcmp(key, "RSHIFT")) {
        keyboard_keyword_rshift();
    } else if (!strcmp(key, "VSHIFT")) {
        keyboard_keyword_vshift();
    } else if (!strcmp(key, "SHIFTL")) {
        keyboard_keyword_shiftl();
    } else if (!strcmp(key, "CLEAR")) {
        keyboard_keyword_clear();
    } else if (!strcmp(key, "INCLUDE")) {
        keyboard_keyword_include();
    } else if (!strcmp(key, "UNDEF")) {
        keyboard_keyword_undef();
    }

    joystick_joypad_clear();
}

static void keyboard_parse_set_pos_row(signed long sym, int row, int col,
                                       int shift)
{
    int i;

    for (i = 0; i < keyc_num; i++) {
        if (sym == keyconvmap[i].sym
            && !(keyconvmap[i].shift & ALLOW_OTHER)
            && !(keyconvmap[i].shift & ALT_MAP)) {
            keyconvmap[i].row = row;
            keyconvmap[i].column = col;
            keyconvmap[i].shift = shift;
            break;
        }
    }

    /* Not in table -> add.  */
    if (i >= keyc_num) {
        /* Table too small -> realloc.  */
        if (keyc_num >= keyc_mem) {
            keyboard_keyconvmap_realloc();
        }

        if (keyc_num < keyc_mem) {
            keyconvmap[keyc_num].sym = sym;
            keyconvmap[keyc_num].row = row;
            keyconvmap[keyc_num].column = col;
            keyconvmap[keyc_num].shift = shift;
            keyconvmap[++keyc_num].sym = ARCHDEP_KEYBOARD_SYM_NONE;
        }
    }
}

static int keyboard_parse_set_neg_row(signed long sym, int row, int col)
{
    if (row == -1 && (col >= 0) && (col <= 8)) {
        joykeys[JOYSTICK_KEYSET_IDX_A][col] = sym;
    } else if (row == -2 && (col >= 0) && (col <= 8)) {
        joykeys[JOYSTICK_KEYSET_IDX_B][col] = sym;
    } else if (row == -3 && col == 0) {
        key_ctrl_restore1 = sym;
    } else if (row == -3 && col == 1) {
        key_ctrl_restore2 = sym;
    } else if (row == -4 && col == 0) {
        key_ctrl_column4080 = sym;
    } else if (row == -4 && col == 1) {
        key_ctrl_caps = sym;
    } else {
        return -1;
    }
    return 0;
}

static void keyboard_parse_entry(char *buffer)
{
    char *key, *p;
    signed long sym;
    int row, col;
    int shift = 0;

    key = strtok(buffer, " \t:");

    sym = kbd_arch_keyname_to_keynum(key);

    if (sym < 0) {
        log_error(keyboard_log, "Could not find key `%s'!", key);
        return;
    }

    p = strtok(NULL, " \t,");
    if (p != NULL) {
        row = strtol(p, NULL, 10);
        p = strtok(NULL, " \t,");
        if (p != NULL) {
            col = atoi(p);
            p = strtok(NULL, " \t");
            if (p != NULL || row < 0) {
                if (p != NULL) {
                    shift = atoi(p);
                }

                if (row >= 0) {
                    keyboard_parse_set_pos_row(sym, row, col, shift);
                } else {
                    if (keyboard_parse_set_neg_row(sym, row, col) < 0) {
                        log_error(keyboard_log,
                                  "Bad row/column value (%d/%d) for keysym `%s'.",
                                  row, col, key);
                    }
                }
            }
        }
    }
}


static int keyboard_parse_keymap(const char *filename, int child)
{
    FILE *fp;
    char *complete_path = NULL;
    char buffer[1000];

    fp = sysfile_open(filename, &complete_path, MODE_READ_TEXT);

    if (fp == NULL) {
        log_message(keyboard_log, "Error loading keymap `%s'->`%s'.", filename, complete_path ? complete_path : "<empty/null>");
        return -1;
    }

    log_message(keyboard_log, "%s keymap `%s'.", child ? " including" : "Loading", complete_path);

    do {
        buffer[0] = 0;
        if (fgets(buffer, 999, fp)) {
            char *p;

            if (strlen(buffer) == 0) {
                break;
            }

            buffer[strlen(buffer) - 1] = 0; /* remove newline */
            /* remove comments */
            if ((p = strchr(buffer, '#'))) {
                *p = 0;
            }

            switch (*buffer) {
                case 0:
                    break;
                case '!':
                    /* keyword handling */
                    keyboard_parse_keyword(buffer);
                    break;
                default:
                    /* table entry handling */
                    keyboard_parse_entry(buffer);
                    break;
            }
        }
    } while (!feof(fp));
    fclose(fp);

    lib_free(complete_path);

    return 0;
}

static int keyboard_keymap_load(const char *filename)
{
    DBG((">keyboard_keymap_load(%s)\n", filename));
    if (filename == NULL) {
        DBG(("<keyboard_keymap_load ERROR\n"));
        return -1;
    }

    if (keyconvmap != NULL) {
        keyboard_keyconvmap_free();
    }

    keyboard_keyconvmap_alloc();

    DBG(("<keyboard_keymap_load -> keyboard_parse_keymap\n"));
    return keyboard_parse_keymap(filename, 0);
}

/*-----------------------------------------------------------------------*/

void keyboard_set_map_any(signed long sym, int row, int col, int shift)
{
    if (row >= 0) {
        keyboard_parse_set_pos_row(sym, row, col, shift);
    } else {
        keyboard_parse_set_neg_row(sym, row, col);
    }
}

void keyboard_set_unmap_any(signed long sym)
{
    keyboard_keysym_undef(sym);
}

int keyboard_keymap_dump(const char *filename)
{
    FILE *fp;
    int i;

    if (filename == NULL) {
        return -1;
    }

    fp = fopen(filename, MODE_WRITE_TEXT);

    if (fp == NULL) {
        return -1;
    }

    fprintf(fp, "# VICE keyboard mapping file\n"
            "#\n"
            "# A Keyboard map is read in as patch to the current map.\n"
            "#\n"
            "# File format:\n"
            "# - comment lines start with '#'\n"
            "# - keyword lines start with '!keyword'\n"
            "# - normal line has 'keysym/scancode row column shiftflag'\n"
            "#\n"
            "# Keywords and their lines are:\n"
            "# '!CLEAR'               clear whole table\n"
            "# '!INCLUDE filename'    read file as mapping file\n"
            "# '!LSHIFT row col'      left shift keyboard row/column\n"
            "# '!RSHIFT row col'      right shift keyboard row/column\n"
            "# '!VSHIFT shiftkey'     virtual shift key (RSHIFT or LSHIFT)\n"
            "# '!SHIFTL shiftkey'     shift lock key (RSHIFT or LSHIFT)\n"
            "# '!UNDEF keysym'        remove keysym from table\n"
            "#\n"
            "# Shiftflag can have the values:\n"
            "# 0      key is not shifted for this keysym/scancode\n"
            "# 1      key is shifted for this keysym/scancode\n"
            "# 2      left shift\n"
            "# 4      right shift\n"
            "# 8      key can be shifted or not with this keysym/scancode\n"
            "# 16     deshift key for this keysym/scancode\n"
            "# 32     another definition for this keysym/scancode follows\n"
            "# 64     shift lock\n"
            "# 256    key is used for an alternative keyboard mapping\n"
            "#\n"
            "# Negative row values:\n"
            "# 'keysym -1 n' joystick keymap A, direction n\n"
            "# 'keysym -2 n' joystick keymap B, direction n\n"
            "# 'keysym -3 0' first RESTORE key\n"
            "# 'keysym -3 1' second RESTORE key\n"
            "# 'keysym -4 0' 40/80 column key\n"
            "# 'keysym -4 1' CAPS (ASCII/DIN) key\n"
            "#\n"
            "# Joystick direction values:\n"
            "# 0      Fire\n"
            "# 1      South/West\n"
            "# 2      South\n"
            "# 3      South/East\n"
            "# 4      West\n"
            "# 5      East\n"
            "# 6      North/West\n"
            "# 7      North\n"
            "# 8      North/East\n"
            "#\n\n"
            );
    fprintf(fp, "!CLEAR\n");
    fprintf(fp, "!LSHIFT %d %d\n", kbd_lshiftrow, kbd_lshiftcol);
    fprintf(fp, "!RSHIFT %d %d\n", kbd_rshiftrow, kbd_rshiftcol);
    if (vshift != KEY_NONE) {
        fprintf(fp, "!VSHIFT %s\n",
                (vshift == KEY_RSHIFT) ? "RSHIFT" : "LSHIFT");
    }
    if (shiftl != KEY_NONE) {
        fprintf(fp, "!SHIFTL %s\n",
                (shiftl == KEY_RSHIFT) ? "RSHIFT" : "LSHIFT");
    }
    fprintf(fp, "\n");

    for (i = 0; keyconvmap[i].sym != ARCHDEP_KEYBOARD_SYM_NONE; i++) {
        fprintf(fp, "%s %d %d %d\n",
                kbd_arch_keynum_to_keyname(keyconvmap[i].sym),
                keyconvmap[i].row, keyconvmap[i].column,
                keyconvmap[i].shift);
    }
    fprintf(fp, "\n");

    if ((key_ctrl_restore1 != -1) || (key_ctrl_restore2 != -1)) {
        fprintf(fp, "#\n"
                "# Restore key mappings\n"
                "#\n");
        if (key_ctrl_restore1 != -1) {
            fprintf(fp, "%s -3 0\n",
                    kbd_arch_keynum_to_keyname(key_ctrl_restore1));
        }
        if (key_ctrl_restore2 != -1) {
            fprintf(fp, "%s -3 1\n",
                    kbd_arch_keynum_to_keyname(key_ctrl_restore2));
        }
        fprintf(fp, "\n");
    }

    if (key_ctrl_column4080 != -1) {
        fprintf(fp, "#\n"
                "# 40/80 column key mapping\n"
                "#\n");
        fprintf(fp, "%s -4 0\n",
                kbd_arch_keynum_to_keyname(key_ctrl_column4080));
        fprintf(fp, "\n");
    }

    if (key_ctrl_caps != -1) {
        fprintf(fp, "#\n"
                "# CAPS (ASCII/DIN) key mapping\n"
                "#\n");
        fprintf(fp, "%s -4 1\n",
                kbd_arch_keynum_to_keyname(key_ctrl_caps));
        fprintf(fp, "\n");
    }

    for (i = 0; i < JOYSTICK_KEYSET_NUM_KEYS; i++) {
        if (joykeys[JOYSTICK_KEYSET_IDX_A][i] != ARCHDEP_KEYBOARD_SYM_NONE) {
            fprintf(fp, "#\n"
                    "# Joystick keyset A mapping\n"
                    "#\n");
            for (i = 0; i < JOYSTICK_KEYSET_NUM_KEYS; i++) {
                if (joykeys[JOYSTICK_KEYSET_IDX_A][i] != ARCHDEP_KEYBOARD_SYM_NONE) {
                    fprintf(fp, "%s -1 %d\n", kbd_arch_keynum_to_keyname(joykeys[JOYSTICK_KEYSET_IDX_A][i]), i);
                }
            }
            fprintf(fp, "\n");
            break;
        }
    }

    for (i = 0; i < JOYSTICK_KEYSET_NUM_KEYS; i++) {
        if (joykeys[JOYSTICK_KEYSET_IDX_B][i] != ARCHDEP_KEYBOARD_SYM_NONE) {
            fprintf(fp, "#\n"
                    "# Joystick keyset B mapping\n"
                    "#\n");
            for (i = 0; i < JOYSTICK_KEYSET_NUM_KEYS; i++) {
                if (joykeys[JOYSTICK_KEYSET_IDX_B][i] != ARCHDEP_KEYBOARD_SYM_NONE) {
                    fprintf(fp, "%s -2 %d\n", kbd_arch_keynum_to_keyname(joykeys[JOYSTICK_KEYSET_IDX_B][i]), i);
                }
            }
            fprintf(fp, "\n");
            break;
        }
    }

    fclose(fp);

    return 0;
}

/*-----------------------------------------------------------------------*/

void keyboard_register_column4080_key(key_ctrl_column4080_func_t func)
{
    key_ctrl_column4080_func = func;
}

void keyboard_register_caps_key(key_ctrl_caps_func_t func)
{
    key_ctrl_caps_func = func;
}
#endif

/*-----------------------------------------------------------------------*/
#define NUM_KEYBOARD_MAPPINGS 4

static char *machine_keymap_res_name_list[NUM_KEYBOARD_MAPPINGS] = {
    "KeymapSymFile",
    "KeymapPosFile",
    "KeymapUserSymFile",
    "KeymapUserPosFile",
};

char *machine_keymap_file_list[NUM_KEYBOARD_MAPPINGS] = {
    NULL, NULL, NULL, NULL
};

char *machine_get_keymap_res_name(int val)
{
    if ((val < 0) || (val >= NUM_KEYBOARD_MAPPINGS)) {
        return NULL;
    }
    return machine_keymap_res_name_list[val];
}

int machine_num_keyboard_mappings(void)
{
    return NUM_KEYBOARD_MAPPINGS;
}


#ifdef COMMON_KBD

static int machine_keyboard_mapping = 0;
static int machine_keyboard_type = 0;

static int try_set_keymap_file(int atidx, int idx, int mapping, int type);
static int switch_keymap_file(int *idxp, int *mapp, int *typep);

/* (re)load keymap at index */
int load_keymap_file(int val)
{
    const char *name, *resname;

    if ((val < 0) || (val > KBD_INDEX_LAST)) {
        return -1;
    }

    if (load_keymap_ok)
    {
        resname = machine_get_keymap_res_name(val);
        if (!resname) {
            return -1;
        }

        if (resources_get_string(resname, &name) < 0) {
            return -1;
        }

        DBG(("load_keymap_file(%d) calls keyboard_keymap_load(%s)\n", val, name));
        if (keyboard_keymap_load(name) >= 0) {

        } else {
            log_error(keyboard_log, "Cannot load keymap `%s'.", name ? name : "<none/null>");
            return -1;
        }
    }
    return 0;
}

/*-----------------------------------------------------------------------*/

/* handle change of "KeymapXXXXFile" */
int keyboard_set_keymap_file(const char *val, void *param)
{
    int oldindex, newindex;

    newindex = vice_ptr_to_int(param);

    DBG(("keyboard_set_keymap_file '%s' newidx:%d\n", val, newindex));

    /* FIXME: remove */
    if (newindex >= machine_num_keyboard_mappings()) {
        return -1;
    }

    if (resources_get_int("KeymapIndex", &oldindex) < 0) {
        return -1;
    }

    if (util_string_set(&machine_keymap_file_list[newindex], val)) {
        return 0;
    }

    /* reset oldindex -> reload keymap file if this keymap is active */
    if (oldindex == newindex) {
        if (resources_set_int("KeymapIndex", oldindex) < 0) {
            return -1;
        }
    }
    return 0;
}

/* handle change of "KeymapIndex" */
int keyboard_set_keymap_index(int val, void *param)
{
    int mapping;
    int type;

    DBG(("*keyboard_set_keymap_index(%d)\n", val));

    if ((val < 0) || (val > KBD_INDEX_LAST)) {
        return -1;
    }

    mapping = machine_keyboard_mapping;
    type = machine_keyboard_type;

    DBG((">keyboard_set_keymap_index(idx:%d mapping:%d type:%d)\n", val, mapping, type));

    if (val < 2) {
        if (switch_keymap_file(&val, &mapping, &type) < 0) {
            DBG(("<keyboard_set_keymap_index switch_keymap_file ERROR\n"));
            log_error(keyboard_log, "Default keymap not found, this should be fixed. Going on anyway...");
            /* return -1; */
            return 0; /* HACK: allow to start up when default keymap is missing */
        }
        machine_keyboard_mapping = mapping;
        machine_keyboard_type = type;
    }

    if (load_keymap_file(val) < 0) {
        DBG(("<keyboard_set_keymap_index load_keymap_file ERROR\n"));
        return -1;
    }

    DBG(("<keyboard_set_keymap_index OK (idx:%d mapping:%d type:%d)\n", val, mapping, type));
    machine_keymap_index = val;
    return 0;
}

/* handle change if "KeyboardType" */
int keyboard_set_keyboard_type(int val, void *param)
{
    int idx, mapping;

    mapping = machine_keyboard_mapping;
    idx = machine_keymap_index;

    DBG((">keyboard_set_keyboard_type(idx:%d mapping:%d type:%d)\n", idx, mapping, val));
    if (idx < 2) {
        if (switch_keymap_file(&idx, &mapping, &val) < 0) {
            log_error(keyboard_log, "Default keymap not found, this should be fixed. Going on anyway...");
            /* return -1; */
            return 0; /* HACK: allow to start up when default keymap is missing */
        }
        machine_keymap_index = idx;
        machine_keyboard_mapping = mapping;
    }

    if (load_keymap_file(idx) < 0) {
        DBG(("<keyboard_set_keyboard_type load_keymap_file ERROR\n"));
        return -1;
    }

    machine_keyboard_type = val;
    DBG(("<keyboard_set_keyboard_type(%d)\n", val));
    return 0;
}

/* handle change if "KeyboardMapping" */
int keyboard_set_keyboard_mapping(int val, void *param)
{
    int type;
    int idx;


    type = machine_keyboard_type;
    idx = machine_keymap_index;
    DBG((">keyboard_set_keyboard_mapping(%d,%d,%d)\n", idx, type, val));

    if (idx < 2) {
        if (switch_keymap_file(&idx, &val, &type) < 0) {
            log_error(keyboard_log, "Default keymap not found, this should be fixed. Going on anyway...");
            /* return -1; */
            return 0; /* HACK: allow to start up when default keymap is missing */
        }
        machine_keymap_index = idx;
        machine_keyboard_type = type;
    }

    if (load_keymap_file(idx) < 0) {
        DBG(("<keyboard_set_keyboard_mapping load_keymap_file ERROR\n"));
        return -1;
    }

    machine_keyboard_mapping = val;
    DBG(("<keyboard_set_keyboard_mapping(%d,%d,%d)\n", idx, type, val));

    return 0;
}

/* return number of available keyboard maps for gives "type" and "index" (sym/pos) */
int keyboard_get_num_mappings(void)
{
    return KBD_MAPPING_NUM;
}

static mapping_info_t kbdinfo[KBD_MAPPING_NUM + 1] = {
    { "American (us)", KBD_MAPPING_US, "" },
    { "British (uk)", KBD_MAPPING_UK, "uk" },
    { "German (de)", KBD_MAPPING_DE, "de" },
    { "Danish (da)", KBD_MAPPING_DA, "da" },
    { "Norwegian (no)", KBD_MAPPING_NO, "no" },
    { "Finnish (fi)", KBD_MAPPING_FI, "fi" },
    { "Italian (it)", KBD_MAPPING_IT, "it" },
    { NULL, 0, 0 }
};

mapping_info_t *keyboard_get_info_list(void)
{
    return &kbdinfo[0];
}

static char *keyboard_get_mapping_name(int mapping)
{
    return kbdinfo[mapping].mapping_name;
}

static int try_set_keymap_file(int atidx, int idx, int mapping, int type)
{
    char *sympos[2] = { "sym", "pos"};
    char *mapname;
    char *name = NULL, *tstr = NULL;
    char *complete_path;

    DBG((">try_set_keymap_file idx %d mapping %d type %d\n", idx, mapping, type));
    if (type >= 0) {
        tstr = machine_get_keyboard_type_name(type);
    }
    mapname = keyboard_get_mapping_name(mapping);
#if 1
    /* <port>_<type>_<idx>_<mapping>.vkm */
    if ((mapping == 0) && (tstr == NULL)) {
        name = util_concat(KBD_PORT_PREFIX, "_", sympos[idx], ".vkm", NULL);
    } else if ((mapping != 0) && (tstr == NULL)) {
        name = util_concat(KBD_PORT_PREFIX, "_", sympos[idx], "_", mapname, ".vkm", NULL);
    } else if ((mapping == 0) && (tstr != NULL)) {
        name = util_concat(KBD_PORT_PREFIX, "_", tstr, "_", sympos[idx], ".vkm", NULL);
    } else if ((mapping != 0) && (tstr != NULL)) {
        name = util_concat(KBD_PORT_PREFIX, "_", tstr, "_", sympos[idx], "_", mapname, ".vkm", NULL);
    }
#else
    /* FIXME: alternative solution for targets with 8.3 filenames */
#endif
    DBG(("try_set_keymap_file: (port:%s type:%s idx:%d mapping:%d) '%s' = '%s'\n",
                KBD_PORT_PREFIX, tstr ? tstr : "-", idx, mapping,
                idx ? "KeymapPosFile" : "KeymapSymFile", name));

    util_string_set(&machine_keymap_file_list[atidx], name);

    DBG(("try_set_keymap_file calls sysfile_locate(%s)\n", name));
    if (sysfile_locate(name, &complete_path) != 0) {
        lib_free(name);
        DBG(("<try_set_keymap_file ERROR locating keymap `%s'.\n", name ? name : "(null)"));
        return -1;
    }
    lib_free(name);
    DBG(("<try_set_keymap_file OK\n"));
    return 0;
}

static int switch_keymap_file(int *idxp, int *mapp, int *typep)
{
    int type = *typep;
    int mapping = *mapp;
    int idx = *idxp;
    int atidx = *idxp;

    DBG((">switch_keymap_file idx %d mapping %d type %d\n", *idxp, *mapp, *typep));
    if(try_set_keymap_file(atidx, idx, mapping, type) >= 0) {
        goto ok;
    }
    /* if a positional map was not found, we cant really do any better
       than trying a symbolic map for the same keyboard instead */
    if (idx != KBD_INDEX_SYM) {
        idx = KBD_INDEX_SYM;
        if(try_set_keymap_file(atidx, idx, mapping, type) >= 0) {
            goto ok;
        }
    }
    /*  as last resort, always use <port>_sym.vkm (which MUST exist)  */
    /* type = -1; */ /* FIXME: use default type? */
    mapping = KBD_MAPPING_US;
    if(try_set_keymap_file(atidx, idx, mapping, -1) >= 0) {
        type = 0; /* FIXME */
        goto ok;
    }
    DBG(("<switch_keymap_file ERROR idx %d mapping %d type %d\n", idx, mapping, type));
    return -1;

ok:
    DBG(("<switch_keymap_file OK idx %d mapping %d type %d\n", idx, mapping, type));
    *idxp = idx;
    *mapp = mapping;
    *typep = type;
    return 0;
}

/* called by keyboard_resources_init to create the default keymap(s) */
static int keyboard_set_default_keymap_file(int idx)
{
    int mapping = 0;
    int type = 0;

    DBG((">keyboard_set_default_keymap_file(%d)\n", idx));

    if ((idx != KBD_INDEX_SYM) && (idx != KBD_INDEX_POS)) {
        return -1;
    }
    if (resources_get_int("KeyboardMapping", &mapping) < 0) {
        return -1;
    }
    if (resources_get_int("KeyboardType", &type) < 0) {
        return -1;
    }

    if(switch_keymap_file(&idx, &mapping, &type) < 0) {
        /* return -1; */
        DBG(("<keyboard_set_default_keymap_file(FAILURE: idx: %d type: %d mapping: %d)\n", idx, type, mapping));
        return 0; /* always return success to allow starting up without valid keymap */
    }

    machine_keymap_index = idx;
    machine_keyboard_type = type;
    machine_keyboard_mapping = mapping;

    DBG(("<keyboard_set_default_keymap_file(OK: idx: %d type: %d mapping: %d)\n", idx, type, mapping));
    return 0; /* success */
}

/*--------------------------------------------------------------------------*/

static char *resources_string_d0 = NULL;
static char *resources_string_d1 = NULL;
static char *resources_string_d2 = NULL;
static char *resources_string_d3 = NULL;

static const resource_string_t resources_string[] = {
    { "KeymapSymFile", "", RES_EVENT_NO, NULL,
      &machine_keymap_file_list[KBD_INDEX_SYM], keyboard_set_keymap_file, (void *)KBD_INDEX_SYM },
    { "KeymapPosFile", "", RES_EVENT_NO, NULL,
      &machine_keymap_file_list[KBD_INDEX_POS], keyboard_set_keymap_file, (void *)KBD_INDEX_POS },
    { "KeymapUserSymFile", "", RES_EVENT_NO, NULL,
      &machine_keymap_file_list[KBD_INDEX_USERSYM], keyboard_set_keymap_file, (void *)KBD_INDEX_USERSYM },
    { "KeymapUserPosFile", "", RES_EVENT_NO, NULL,
      &machine_keymap_file_list[KBD_INDEX_USERPOS], keyboard_set_keymap_file, (void *)KBD_INDEX_USERPOS },
    { NULL }
};

static const resource_int_t resources_int[] = {
    { "KeymapIndex", KBD_INDEX_SYM, RES_EVENT_NO, NULL,
      &machine_keymap_index, keyboard_set_keymap_index, NULL },
    { "KeyboardType", 0, RES_EVENT_NO, NULL,
      &machine_keyboard_type, keyboard_set_keyboard_type, NULL },
    { "KeyboardMapping", 0, RES_EVENT_NO, NULL,
      &machine_keyboard_mapping, keyboard_set_keyboard_mapping, NULL },
    { NULL }
};

/*--------------------------------------------------------------------------*/

int keyboard_resources_init(void)
{
    int nsym, npos, mapping, idx, type;
    const char *name;

    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }

    npos = (machine_keymap_file_list[KBD_INDEX_POS] == NULL) || (machine_keymap_file_list[KBD_INDEX_POS][0] == 0);
    nsym = (machine_keymap_file_list[KBD_INDEX_SYM] == NULL) || (machine_keymap_file_list[KBD_INDEX_SYM][0] == 0);

    DBG(("keyboard_resources_init(first start:%s)\n", (npos && nsym) ? "yes" : "no"));

    if (npos && nsym) {
        mapping = kbd_arch_get_host_mapping();
        log_verbose("Setting up default keyboard mapping for host type %d (%s)",
                    mapping, keyboard_get_mapping_name(mapping));
        if (resources_set_int("KeymapIndex", KBD_INDEX_SYM) < 0) {
            /* return -1; */
        }
        if (resources_set_int("KeyboardMapping", mapping) < 0) {
            /* return -1; */
        }
        keyboard_set_default_keymap_file(KBD_INDEX_POS);
        if (resources_get_string("KeymapPosFile", &name) < 0) {
            return -1;
        }
        util_string_set(&resources_string_d1, name);
        util_string_set(&resources_string_d3, name);

        log_verbose("Default positional map is: %s", name);
        keyboard_set_default_keymap_file(KBD_INDEX_SYM);
        if (resources_get_string("KeymapSymFile", &name) < 0) {
            return -1;
        }
        log_verbose("Default symbolic map is: %s", name);
        util_string_set(&resources_string_d0, name);
        util_string_set(&resources_string_d2, name);

        /* copy current values into the factory values */
        resources_set_default_string("KeymapSymFile", resources_string_d0);
        resources_set_default_string("KeymapPosFile", resources_string_d1);
        resources_set_default_string("KeymapUserSymFile", resources_string_d2);
        resources_set_default_string("KeymapUserPosFile", resources_string_d3);

        idx = type = mapping = 0;
        if (resources_get_int("KeymapIndex", &idx) < 0) {
            return -1;
        }
        if (resources_get_int("KeyboardType", &type) < 0) {
            return -1;
        }
        if (resources_get_int("KeyboardMapping", &mapping) < 0) {
            return -1;
        }
        resources_set_default_int("KeymapIndex", idx);
        resources_set_default_int("KeyboardType", type);
        resources_set_default_int("KeyboardMapping", mapping);
    }
    return 0;
}

void keyboard_resources_shutdown(void)
{
    lib_free(machine_keymap_file_list[KBD_INDEX_SYM]);
    lib_free(machine_keymap_file_list[KBD_INDEX_POS]);
    lib_free(machine_keymap_file_list[KBD_INDEX_USERSYM]);
    lib_free(machine_keymap_file_list[KBD_INDEX_USERPOS]);
}

#endif /* COMMON_KBD */

/*--------------------------------------------------------------------------*/

#ifdef COMMON_KBD
static cmdline_option_t const cmdline_options[] =
{
    { "-keymap", SET_RESOURCE, 1,
      NULL, NULL, "KeymapIndex", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NUMBER, IDCLS_SPECIFY_KEYMAP_FILE_INDEX,
      NULL, NULL },
/* FIXME: build description dynamically */
    { "-keyboardmapping", SET_RESOURCE, 1,
      NULL, NULL, "KeyboardMapping", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NUMBER, IDCLS_SPECIFY_KEYBOARD_MAPPING,
      NULL, NULL },
/* FIXME: build description dynamically */
    { "-keyboardtype", SET_RESOURCE, 1,
      NULL, NULL, "KeyboardType", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NUMBER, IDCLS_SPECIFY_KEYBOARD_TYPE,
      NULL, NULL },
    { "-symkeymap", SET_RESOURCE, 1,
      NULL, NULL, "KeymapUserSymFile", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_SYM_KEYMAP_FILE_NAME,
      NULL, NULL },
    { "-poskeymap", SET_RESOURCE, 1,
      NULL, NULL, "KeymapUserPosFile", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_POS_KEYMAP_FILE_NAME,
      NULL, NULL },
    { NULL}
};

int keyboard_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
#endif  /* COMMON_KBD */

/*--------------------------------------------------------------------------*/

void keyboard_init(void)
{
    keyboard_log = log_open("Keyboard");

    keyboard_alarm = alarm_new(maincpu_alarm_context, "Keyboard",
                            keyboard_latch_handler, NULL);
#ifdef COMMON_KBD
    restore_alarm = alarm_new(maincpu_alarm_context, "Restore",
                            restore_alarm_triggered, NULL);

    kbd_arch_init();

    if (machine_class != VICE_MACHINE_VSID) {
        load_keymap_ok = 1;
        keyboard_set_keymap_index(machine_keymap_index, NULL);
    }
#endif
}

void keyboard_shutdown(void)
{
#ifdef COMMON_KBD
    keyboard_keyconvmap_free();
    keyboard_resources_shutdown();      /* FIXME: perhaps call from elsewhere? */
#endif
}

/*--------------------------------------------------------------------------*/
int keyboard_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, "KEYBOARD", 1, 0);
    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_DWA(m, (DWORD *)keyarr, KBD_ROWS) < 0
        || SMW_DWA(m, (DWORD *)rev_keyarr, KBD_COLS) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (snapshot_module_close(m) < 0) {
        return -1;
    }

    return 0;
}

int keyboard_snapshot_read_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, "KEYBOARD",
                             &major_version, &minor_version);
    if (m == NULL) {
        return 0;
    }

    if (0
        || SMR_DWA(m, (DWORD *)keyarr, KBD_ROWS) < 0
        || SMR_DWA(m, (DWORD *)rev_keyarr, KBD_COLS) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}
