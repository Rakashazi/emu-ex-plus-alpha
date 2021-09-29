/** \file   keyboard.c
 * \brief   Common keyboard emulation.
 *
 * \author  Andreas Boose <viceteam@t-online.de>
 * \author  Ettore Perazzoli <ettore@comm2000.it>
 * \author  Jouko Valta <jopi@stekt.oulu.fi>
 * \author  Andre Fachat <fachat@physik.tu-chemnitz.de>
 * \author  Bernhard Kuhn <kuhn@eikon.e-technik.tu-muenchen.de>
 */

/*
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
#include <ctype.h>

#ifndef RAND_MAX
#include <limits.h>
#define RAND_MAX INT_MAX
#endif

#include "alarm.h"
#include "archdep.h"
#include "archdep_kbd_get_host_mapping.h"
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
#include "types.h"
#include "util.h"
#include "vice-event.h"

/* #define DBGKBD */

#ifdef DBGKBD
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#define KEYBOARD_RAND() lib_unsigned_rand(1, (unsigned int)machine_get_cycles_per_frame())

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

static CLOCK keyboard_delay = 0;

static int keyboard_clear = 0;

static alarm_t *restore_alarm = NULL; /* restore key alarm context */


/** \brief  Resource value for KdbStatusbar
 *
 * Determines whether to show the keyboard debugging widget on the statusbar.
 */
static int kbd_statusbar_enabled = 0;


/** \brief  Resource handler for 'KbdStatusbar'
 *
 * Enables/disables the display of the keyboard debugging on the statusbar
 *
 * \param[in]   val     enable display of widget
 * \param[in]   param   extra data (unused)
 *
 * \return 0
 */
static int keyboard_set_keyboard_statusbar(int val, void *param)
{
    kbd_statusbar_enabled = val ? 1 : 0;
    return 0;   /* Okidoki */
}


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
    /* printf("keyboard_set_latch_keyarr %d: %d %d\n", value, row, col); */
    if (value) {
        latch_keyarr[row] |= 1 << col;
        latch_rev_keyarr[col] |= 1 << row;
    } else {
        latch_keyarr[row] &= ~(1 << col);
        latch_rev_keyarr[col] &= ~(1 << row);
    }
#if 0
    {
        int r, c;
        for (r = 0; r < 8; r++) {
            for (c = 0; c < 8; c++) {
                printf("%c", latch_keyarr[r] & (1 << c) ? '*' : '.');
            }
            printf("\n");
        }
    }
#endif
    return 0;
}

/*-----------------------------------------------------------------------*/

static void keyboard_key_clear_internal(void);

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
    machine_set_restore_key((int)(*(uint32_t *)data));
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
        keyboard_key_clear_internal();
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
    keyboard_shiftlock = 0;
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

/* 40/80 column key.  */
static signed long key_ctrl_column4080 = -1;
static key_ctrl_column4080_func_t key_ctrl_column4080_func = NULL;

/* CAPS (ASCII/DIN) key.  */
static signed long key_ctrl_caps = -1;
static key_ctrl_caps_func_t key_ctrl_caps_func = NULL;

/* joyport attached keypad. */
static signed long key_joy_keypad[KBD_JOY_KEYPAD_ROWS][KBD_JOY_KEYPAD_COLS];
static key_joy_keypad_func_t key_joy_keypad_func = NULL;

void keyboard_register_column4080_key(key_ctrl_column4080_func_t func)
{
    key_ctrl_column4080_func = func;
}

void keyboard_register_caps_key(key_ctrl_caps_func_t func)
{
    key_ctrl_caps_func = func;
}

void keyboard_register_joy_keypad(key_joy_keypad_func_t func)
{
    key_joy_keypad_func = func;
}

/*-----------------------------------------------------------------------*/

enum shift_type {
    NO_SHIFT = 0,                 /* Key is not shifted. Keys will be deshifted,
                                     no other flags will be checked */

    VIRTUAL_SHIFT     = (1 << 0), /* The key needs a shift on the emulated machine. */
    LEFT_SHIFT        = (1 << 1), /* Key is left shift on the emulated machine. */
    RIGHT_SHIFT       = (1 << 2), /* Key is right shift on the emulated machine. */
    ALLOW_SHIFT       = (1 << 3), /* Allow key to be shifted. */
    DESHIFT_SHIFT     = (1 << 4), /* Although SHIFT might be pressed, do not
                                     press shift on the emulated machine. */
    ALLOW_OTHER       = (1 << 5), /* Allow another key code to be assigned if
                                     SHIFT is pressed. */
    SHIFT_LOCK        = (1 << 6), /* Key is shift lock on the emulated machine */
    MAP_MOD_SHIFT     = (1 << 7), /* Key requires SHIFT to be pressed on host */

    ALT_MAP           = (1 << 8), /* Key is used for an alternative keyboard mapping (x128) */

    MAP_MOD_RIGHT_ALT = (1 << 9), /* Key requires right ALT (Alt-gr) to be pressed on host */
    MAP_MOD_CTRL     = (1 << 10), /* Key requires control to be pressed on host */

    VIRTUAL_CBM      = (1 << 11), /* The key is combined with CBM on the emulated machine */
    VIRTUAL_CTRL     = (1 << 12), /* The key is combined with CTRL on the emulated machine */

    LEFT_CBM         = (1 << 13), /* Key is CBM on the emulated machine */
    LEFT_CTRL        = (1 << 14)  /* Key is CTRL on the emulated machine */
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

/* Is an alternative mapping active? */
static int key_alternative = 0;

static keyboard_conv_t *keyconvmap = NULL;

/* matrix locations for the modifier keys */
static int kbd_lshiftrow = -1;
static int kbd_lshiftcol = -1;
static int kbd_rshiftrow = -1;
static int kbd_rshiftcol = -1;
static int kbd_lcbmrow   = -1;
static int kbd_lcbmcol   = -1;
static int kbd_lctrlrow  = -1;
static int kbd_lctrlcol  = -1;

#define KEY_NONE   0
#define KEY_RSHIFT 1
#define KEY_LSHIFT 2
#define KEY_LCBM   3
#define KEY_LCTRL  4

static int vshift = KEY_NONE;   /* virtual shift */
static int vcbm   = KEY_NONE;   /* virtual cbm */
static int vctrl  = KEY_NONE;   /* virtual ctrl */

static int shiftl = KEY_NONE;   /* shift-lock */

/*-----------------------------------------------------------------------*/

static int left_shift_down, right_shift_down, 
            left_cbm_down, left_ctrl_down,
            virtual_shift_down, virtual_cbm_down, virtual_ctrl_down;
static int key_latch_row, key_latch_column;

static inline int rshift_defined(void) {
    if ((kbd_rshiftrow != -1) && (kbd_rshiftcol != -1)) {
        return 1;
    }
    return 0;
}

static inline int lshift_defined(void) {
    if ((kbd_lshiftrow != -1) && (kbd_lshiftcol != -1)) {
        return 1;
    }
    return 0;
}

static inline int lcbm_defined(void) {
    if ((kbd_lcbmrow != -1) && (kbd_lcbmcol != -1)) {
        return 1;
    }
    return 0;
}

static inline int lctrl_defined(void) {
    if ((kbd_lctrlrow != -1) && (kbd_lctrlcol != -1)) {
        return 1;
    }
    return 0;
}

static inline int vshift_defined(void) {
    return !(vshift == KEY_NONE);
}

static inline int vctrl_defined(void) {
    return !(vctrl == KEY_NONE);
}

static inline int vcbm_defined(void) {
    return !(vcbm == KEY_NONE);
}

static inline int shiftlock_defined(void) {
    return !(shiftl == KEY_NONE);
}

static void keyboard_key_deshift(void)
{
    if (lshift_defined()) {
        keyboard_set_latch_keyarr(kbd_lshiftrow, kbd_lshiftcol, 0);
    }
    if (rshift_defined()) {
        keyboard_set_latch_keyarr(kbd_rshiftrow, kbd_rshiftcol, 0);
    }
    if (lcbm_defined()) {
        keyboard_set_latch_keyarr(kbd_lcbmrow,   kbd_lcbmcol,   0);
    }
    if (lctrl_defined()) {
        keyboard_set_latch_keyarr(kbd_lctrlrow,  kbd_lctrlcol,  0);
    }
}

static void keyboard_key_shift(void)
{
    if (lshift_defined()) {
        if (left_shift_down > 0
            || (virtual_shift_down > 0 && vshift == KEY_LSHIFT)
            || (keyboard_shiftlock > 0 && shiftl == KEY_LSHIFT)) {
            keyboard_set_latch_keyarr(kbd_lshiftrow, kbd_lshiftcol, 1);
        }
    }
    if (rshift_defined()) {
        if (right_shift_down > 0
            || (virtual_shift_down > 0 && vshift == KEY_RSHIFT)
            || (keyboard_shiftlock > 0 && shiftl == KEY_RSHIFT)) {
            keyboard_set_latch_keyarr(kbd_rshiftrow, kbd_rshiftcol, 1);
        }
    }
    if (lcbm_defined()) {
        if (left_cbm_down > 0
            || (virtual_cbm_down > 0 && vcbm == KEY_LCBM)) {
            keyboard_set_latch_keyarr(kbd_lcbmrow, kbd_lcbmcol, 1);
        }
    }
    if (lctrl_defined()) {
        if (left_ctrl_down > 0
            || (virtual_ctrl_down > 0 && vctrl == KEY_LCTRL)) {
            keyboard_set_latch_keyarr(kbd_lctrlrow, kbd_lctrlcol, 1);
        }
    }
}

static int keyboard_key_pressed_matrix(int row, int column, int shift)
{
    if (row >= 0) {
        key_latch_row = row;
        key_latch_column = column;

        if (shift == NO_SHIFT) {
            keyboard_key_deshift();
        } else {
            /* FIXME: somehow make sure virtual shift/cbm/ctrl is really only
                      valid for one combined keypress. the shift/ctrl/cbm
                      status should not get permanently altered by deshifting */
            if (shift & DESHIFT_SHIFT) {
                /* FIXME: should this really remove ALL modifiers? */
                keyboard_key_deshift();
            }
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
                keyboard_shiftlock ^= 1;
            }
            if (lcbm_defined()) {
                if (shift & VIRTUAL_CBM) {
                    virtual_cbm_down = 1;
                }
                if (shift & LEFT_CBM) {
                    left_cbm_down = 1;
                }
            }
            if (lctrl_defined()) {
                if (shift & VIRTUAL_CTRL) {
                    virtual_ctrl_down = 1;
                }
                if (shift & LEFT_CTRL) {
                    left_ctrl_down = 1;
                }
            }

            if (shift & DESHIFT_SHIFT) {
                /* FIXME: should this really remove ALL modifiers? */
                left_shift_down = 0;
                right_shift_down = 0;
                left_ctrl_down = 0;
                left_cbm_down = 0;
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
    uint32_t event_data;
    alarm_unset(restore_alarm);

    event_data = (uint32_t)restore_delayed;
    machine_set_restore_key(restore_delayed);
    event_record(EVENT_KEYBOARD_RESTORE, (void*)&event_data, sizeof(uint32_t));
    restore_delayed = 0;

    if (restore_quick_release) {
        restore_quick_release = 0;
        alarm_set(restore_alarm, maincpu_clk + KEYBOARD_RAND());
    }
}

static void keyboard_restore_pressed(void)
{
    uint32_t event_data;
    event_data = (uint32_t)1;
    if (network_connected()) {
        network_event_record(EVENT_KEYBOARD_RESTORE, (void*)&event_data, sizeof(uint32_t));
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
    uint32_t event_data;
    event_data = (uint32_t)0;
    if (network_connected()) {
        network_event_record(EVENT_KEYBOARD_RESTORE, (void*)&event_data, sizeof(uint32_t));
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

/* press a key, this is called by the UI */
void keyboard_key_pressed(signed long key, int mod)
{
    int i, j, latch;

    /* log_debug("%s:  %3i %04x", __func__, key, mod); */

    if (event_playback_active()) {
        return;
    }

    /* Restore */
    if (((key == key_ctrl_restore1) || (key == key_ctrl_restore2))
        && machine_has_restore_key()) {
        keyboard_restore_pressed();
        return;
    }

    /* c128 40/80 column key */
    if (key == key_ctrl_column4080) {
        if (key_ctrl_column4080_func != NULL) {
            key_ctrl_column4080_func();
        }
        return;
    }

    /* c128 caps lock key */
    if (key == key_ctrl_caps) {
        if (key_ctrl_caps_func != NULL) {
            key_ctrl_caps_func();
        }
        return;
    }

    if (key_joy_keypad_func != NULL) {
        for (i = 0; i < KBD_JOY_KEYPAD_ROWS; ++i) {
            for (j = 0; j < KBD_JOY_KEYPAD_COLS; ++j) {
                if (key == key_joy_keypad[i][j]) {
                    key_joy_keypad_func(i, j, 1);
                    return;
                }
            }
        }
    }

#ifdef COMMON_JOYKEYS
    for (i = 0; i < JOYSTICK_NUM; ++i) {
        if (joystick_port_map[i] == JOYDEV_NUMPAD
            || joystick_port_map[i] == JOYDEV_KEYSET1
            || joystick_port_map[i] == JOYDEV_KEYSET2) {
            if (joystick_check_set(key, joystick_port_map[i] - JOYDEV_NUMPAD, 1 + i)) {
                return;
            }
        }
    }
#endif

    if (keyconvmap == NULL) {
        return;
    }

    latch = 0;

    for (i = 0; i < keyc_num; ++i) {
        if (key == keyconvmap[i].sym) {
            /* skip keys from alternative keyset */
            if ((keyconvmap[i].shift & ALT_MAP) && !key_alternative) {
                continue;
            }

            /* find explicit matches on modifiers pressed on host */
            if ((keyconvmap[i].shift & MAP_MOD_RIGHT_ALT) && (!(mod & KBD_MOD_RALT)) ) {
                continue;
            }
            if ((keyconvmap[i].shift & MAP_MOD_CTRL) && (!(mod & (KBD_MOD_LCTRL | KBD_MOD_RCTRL))) ) {
                continue;
            }
            if ((keyconvmap[i].shift & MAP_MOD_SHIFT) && (!(mod & (KBD_MOD_LSHIFT | KBD_MOD_RSHIFT))) ) {
                continue;
            }

            if (keyboard_key_pressed_matrix(keyconvmap[i].row,
                                            keyconvmap[i].column,
                                            keyconvmap[i].shift)) {
                latch = 1;
                if (!(keyconvmap[i].shift & ALLOW_OTHER)
                    /*|| (right_shift_down + left_shift_down) == 0*/) {
                    break;
                }
            }
        }
    }

    if (latch) {
        keyboard_set_latch_keyarr(key_latch_row, key_latch_column, 1);
        if (network_connected()) {
            CLOCK delay = KEYBOARD_RAND();
            network_event_record(EVENT_KEYBOARD_DELAY, (void *)&delay, sizeof(delay));
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
#if 0
        if (shift & SHIFT_LOCK) {
            keyboard_shiftlock = 0;
            if (((shiftl == KEY_RSHIFT) && right_shift_down)
                || ((shiftl == KEY_LSHIFT) && left_shift_down)) {
                skip_release = 1;
            }
        }
#endif
        /* when shift lock is released and shift lock is "locked", then exit
           early and do nothing */
        if (shift & SHIFT_LOCK) {
            if (keyboard_shiftlock) {
                return 0;
            }
        }
        
        if (lcbm_defined()) {
            if (shift & VIRTUAL_CBM) {
                virtual_cbm_down = 0;
            }
            if (shift & LEFT_CBM) {
                left_cbm_down = 0;
            }
        }
        
        if (lctrl_defined()) {
            if (shift & VIRTUAL_CTRL) {
                virtual_ctrl_down = 0;
            }
            if (shift & LEFT_CTRL) {
                left_ctrl_down = 0;
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

        if (lcbm_defined()) {
            if (left_cbm_down > 0
                || (virtual_cbm_down > 0 && vcbm == KEY_LCBM)) {
                keyboard_set_latch_keyarr(kbd_lcbmrow, kbd_lcbmcol, 1);
            } else {
                keyboard_set_latch_keyarr(kbd_lcbmrow, kbd_lcbmcol, 0);
            }
        }

        if (lctrl_defined()) {
            if (left_ctrl_down > 0
                || (virtual_ctrl_down > 0 && vctrl == KEY_LCTRL)) {
                keyboard_set_latch_keyarr(kbd_lctrlrow, kbd_lctrlcol, 1);
            } else {
                keyboard_set_latch_keyarr(kbd_lctrlrow, kbd_lctrlcol, 0);
            }
        }
        return !skip_release;
    }

    return 0;
}

/* release a key, this is called by the UI */
void keyboard_key_released(signed long key, int mod)
{
    int i, j, latch;

    /* log_debug("%s: %3i %04x", __func__, key, mod); */

    if (event_playback_active()) {
        return;
    }

    /* Restore */
    if (((key == key_ctrl_restore1) || (key == key_ctrl_restore2))
        && machine_has_restore_key()) {
        keyboard_restore_released();
        return;
    }

    if (key_joy_keypad_func != NULL) {
        for (i = 0; i < KBD_JOY_KEYPAD_ROWS; ++i) {
            for (j = 0; j < KBD_JOY_KEYPAD_COLS; ++j) {
                if (key == key_joy_keypad[i][j]) {
                    key_joy_keypad_func(i, j, 0);
                    return;
                }
            }
        }
    }

#ifdef COMMON_JOYKEYS
    for (i = 0; i < JOYSTICK_NUM; ++i) {
        if (joystick_port_map[i] == JOYDEV_NUMPAD
            || joystick_port_map[i] == JOYDEV_KEYSET1
            || joystick_port_map[i] == JOYDEV_KEYSET2) {
            if (joystick_check_clr(key, joystick_port_map[i] - JOYDEV_NUMPAD, 1 + i)) {
                return;
            }
        }
    }
#endif

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
            CLOCK delay = KEYBOARD_RAND();
            network_event_record(EVENT_KEYBOARD_DELAY, (void *)&delay, sizeof(delay));
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
    virtual_cbm_down = virtual_shift_down = 
        left_shift_down = right_shift_down = keyboard_shiftlock = 0;
#ifdef COMMON_JOYKEYS
    joystick_joypad_clear();
#endif
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

void keyboard_set_keyarr_any(int row, int col, int value)
{
    signed long sym;

    if (row < 0) {
        if ((row == KBD_ROW_RESTORE_1) && (col == KBD_COL_RESTORE_1)) {
            sym = key_ctrl_restore1;
        } else if ((row == KBD_ROW_RESTORE_2) && (col == KBD_COL_RESTORE_2)) {
            sym = key_ctrl_restore2;
        } else if ((row == KBD_ROW_4080COLUMN) && (col == KBD_COL_4080COLUMN)) {
            sym = key_ctrl_column4080;
        } else if ((row == KBD_ROW_CAPSLOCK) && (col == KBD_COL_CAPSLOCK)) {
            sym = key_ctrl_caps;
        } else if ((row == KBD_ROW_JOY_KEYPAD) &&
            (col >= 0) && (col < KBD_JOY_KEYPAD_NUMKEYS)) {
            sym = key_joy_keypad[col / KBD_JOY_KEYPAD_COLS][col % KBD_JOY_KEYPAD_COLS];
        } else {
            return;
        }

        if (value) {
            keyboard_key_pressed(sym, 0);
        } else {
            keyboard_key_released(sym, 0);
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

static int keyboard_keyword_rowcol(int *row, int *col)
{
    int r, c;
    char *p;

    p = strtok(NULL, " \t,");
    if (p != NULL) {
        r = atoi(p);
        p = strtok(NULL, " \t,");
        if (p != NULL) {
            c = atoi(p);
            /* no error */
            *row = r; *col = c;
            return 0;
        }
    }
    return -1;
}

static int keyboard_keyword_lshift(void)
{
    return keyboard_keyword_rowcol(&kbd_lshiftrow, &kbd_lshiftcol);
}

static int keyboard_keyword_rshift(void)
{
    return keyboard_keyword_rowcol(&kbd_rshiftrow, &kbd_rshiftcol);
}

static int keyboard_keyword_vshiftl(void)
{
    char *p = strtok(NULL, " \t,\r");

    if (!strcmp(p, "RSHIFT")) {
        return KEY_RSHIFT;
    } else if (!strcmp(p, "LSHIFT")) {
        return KEY_LSHIFT;
    }
    
    return -1;
}

static int keyboard_keyword_vshift(void)
{
    int ret = keyboard_keyword_vshiftl();
    if (ret < 0) {
        return -1;
    }
    vshift = ret;
    return 0;
}

static int keyboard_keyword_shiftl(void)
{
    int ret = keyboard_keyword_vshiftl();
    if (ret < 0) {
        return -1;
    }
    shiftl = ret;
    return 0;
}

static int keyboard_keyword_lcbm(void)
{
    return keyboard_keyword_rowcol(&kbd_lcbmrow, &kbd_lcbmcol);
}

static int keyboard_keyword_cbm(void)
{
    char *p = strtok(NULL, " \t,\r");

    if (!strcmp(p, "LCBM")) {
        return KEY_LCBM;
    }
    return -1;
}

static int keyboard_keyword_vcbm(void)
{
    int ret = keyboard_keyword_cbm();
    if (ret < 0) {
        return -1;
    }
    vcbm = ret;
    return 0;
}

static int keyboard_keyword_lctrl(void)
{
    return keyboard_keyword_rowcol(&kbd_lctrlrow, &kbd_lctrlcol);
}

static int keyboard_keyword_ctrl(void)
{
    char *p = strtok(NULL, " \t,\r");

    if (!strcmp(p, "LCTRL")) {
        return KEY_LCTRL;
    }
    return -1;
}

static int keyboard_keyword_vctrl(void)
{
    int ret = keyboard_keyword_ctrl();
    if (ret < 0) {
        return -1;
    }
    vctrl = ret;
    return 0;
}

static void keyboard_keyword_clear(void)
{
    int i, j;

    keyc_num = 0;
    keyconvmap[0].sym = ARCHDEP_KEYBOARD_SYM_NONE;
    key_ctrl_restore1 = -1;
    key_ctrl_restore2 = -1;
    key_ctrl_caps = -1;
    key_ctrl_column4080 = -1;
    vshift = KEY_NONE;
    shiftl = KEY_NONE;
    vcbm = KEY_NONE;
    vctrl = KEY_NONE;
    kbd_lshiftrow = -1;
    kbd_lshiftcol = -1;
    kbd_rshiftrow = -1;
    kbd_rshiftcol = -1;
    kbd_lcbmrow   = -1;
    kbd_lcbmcol   = -1;
    kbd_lctrlrow  = -1;
    kbd_lctrlcol  = -1;
    
    for (i = 0; i < KBD_JOY_KEYPAD_ROWS; ++i) {
        for (j = 0; j < KBD_JOY_KEYPAD_COLS; ++j) {
            key_joy_keypad[i][j] = -1;
        }
    }
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

static void keyboard_parse_keyword(char *buffer, int line, const char *filename)
{
    int ret = 0;
    char *key;

    key = strtok(buffer + 1, " \t:");

    if (!strcmp(key, "LSHIFT")) {
        ret = keyboard_keyword_lshift();
    } else if (!strcmp(key, "RSHIFT")) {
        ret = keyboard_keyword_rshift();
    } else if (!strcmp(key, "VSHIFT")) {
        ret = keyboard_keyword_vshift();
    } else if (!strcmp(key, "SHIFTL")) {
        ret = keyboard_keyword_shiftl();
    } else if (!strcmp(key, "LCBM")) {
        ret = keyboard_keyword_lcbm();
    } else if (!strcmp(key, "VCBM")) {
        ret = keyboard_keyword_vcbm();
    } else if (!strcmp(key, "LCTRL")) {
        ret = keyboard_keyword_lctrl();
    } else if (!strcmp(key, "VCTRL")) {
        ret = keyboard_keyword_vctrl();
    } else if (!strcmp(key, "CLEAR")) {
        keyboard_keyword_clear();
    } else if (!strcmp(key, "INCLUDE")) {
        keyboard_keyword_include();
    } else if (!strcmp(key, "UNDEF")) {
        keyboard_keyword_undef();
    } else {
        log_error(keyboard_log, "%s:%d: unknown keyword (%s).", filename, line, key);
    }

    if (ret) {
        log_error(keyboard_log, "%s:%d: Bad keyword (%s).", filename, line, key);
    }
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
    if ((row == KBD_ROW_JOY_KEYMAP_A) &&
        (col >= 0) && (col < JOYSTICK_KEYSET_NUM_KEYS)) {
#ifdef COMMON_JOYKEYS
        joykeys[JOYSTICK_KEYSET_IDX_A][col] = (int)sym;
#endif
    } else if ((row == KBD_ROW_JOY_KEYMAP_B) &&
        (col >= 0) && (col < JOYSTICK_KEYSET_NUM_KEYS)) {
#ifdef COMMON_JOYKEYS
        joykeys[JOYSTICK_KEYSET_IDX_B][col] = (int)sym;
#endif
    } else if ((row == KBD_ROW_RESTORE_1) && (col == KBD_COL_RESTORE_1)) {
        key_ctrl_restore1 = sym;
    } else if ((row == KBD_ROW_RESTORE_2) && (col == KBD_COL_RESTORE_2)) {
        key_ctrl_restore2 = sym;
    } else if ((row == KBD_ROW_4080COLUMN) && (col == KBD_COL_4080COLUMN)) {
        key_ctrl_column4080 = sym;
    } else if ((row == KBD_ROW_CAPSLOCK) && (col == KBD_COL_CAPSLOCK)) {
        key_ctrl_caps = sym;
    } else if ((row == KBD_ROW_JOY_KEYPAD) &&
        (col >= 0) && (col < KBD_JOY_KEYPAD_NUMKEYS)) {
        key_joy_keypad[col / KBD_JOY_KEYPAD_COLS][col % KBD_JOY_KEYPAD_COLS] = sym;
    } else {
        return -1;
    }
    return 0;
}

static void keyboard_parse_entry(char *buffer, int line, const char *filename)
{
    char *key, *p;
    signed long sym;
    long row;
    int col;
    int shift = 0;

    key = strtok(buffer, " \t:");

    sym = kbd_arch_keyname_to_keynum(key);

    /* log_debug("%s: %s %i", __func__, key, sym); */

    if (sym < 0) {
        log_error(keyboard_log, "Could not find key `%s'!", key);
        return;
    }

    p = strtok(NULL, " \t,");
    if (p != NULL) {
        row = strtol(p, NULL, 10);
        p = strtok(NULL, " \t,");
        if (p != NULL) {
            col = atoi(p);  /* YUCK! */
            p = strtok(NULL, " \t");
            if (p != NULL || row < 0) {
                if (p != NULL) {
                    shift = atoi(p);
                }

                if (row >= 0) {
                    keyboard_parse_set_pos_row(sym, (int)row, col, shift);
                } else {
                    if (keyboard_parse_set_neg_row(sym, (int)row, col) < 0) {
                        log_error(keyboard_log,
                                  "%s:%d: Bad row/column value (%ld/%d) for keysym `%s'.",
                                  filename, line, row, col, key);
                    }
                }

                /* printf("%s:%d: %s %d %d (%04x)\n", filename, line, key, row, col, shift); */

                /* sanity checks */

                if (((shift & LEFT_SHIFT) && ((shift & RIGHT_SHIFT) || (shift & SHIFT_LOCK))) ||
                    ((shift & RIGHT_SHIFT) && ((shift & LEFT_SHIFT) || (shift & SHIFT_LOCK))) ||
                    ((shift & SHIFT_LOCK) && ((shift & RIGHT_SHIFT) || (shift & LEFT_SHIFT)))) {
                    log_warning(keyboard_log, "%s:%d: only one of \"right shift\", \"left shift\" or \"shift lock\" flags should be used.", filename, line);
                }
                if (((shift & VIRTUAL_SHIFT) && ((shift & VIRTUAL_CBM) || (shift & VIRTUAL_CTRL))) ||
                    ((shift & VIRTUAL_CBM) && ((shift & VIRTUAL_SHIFT) || (shift & VIRTUAL_CTRL))) ||
                    ((shift & VIRTUAL_CTRL) && ((shift & VIRTUAL_CBM) || (shift & VIRTUAL_SHIFT)))) {
                    log_warning(keyboard_log, "%s:%d: only one of \"virtual shift\", \"virtual ctrl\" or \"virtual cbm\" flags should be used.", filename, line);
                }

                /* sanity checks for shift */
                
                if (shift & VIRTUAL_SHIFT) {
                    if (!vshift_defined()) {
                        log_warning(keyboard_log, "%s:%d: virtual shift flag used but no !VSHIFT defined", filename, line);
                    }
                }
                
                if (shift & LEFT_SHIFT) {
                    if (!lshift_defined()) {
                        log_warning(keyboard_log, "%s:%d: SHIFT flag used but no !LSHIFT defined", filename, line);
                    } else {
                        if ((row != kbd_lshiftrow) || (col != kbd_lshiftcol)) {
                            log_warning(keyboard_log, "%s:%d: SHIFT flag used but row and/or col differs from !LSHIFT definition", filename, line);
                        }
                    }
                }
                if (shift & RIGHT_SHIFT) {
                    if (!rshift_defined()) {
                        log_warning(keyboard_log, "%s:%d: SHIFT flag used but no !RSHIFT defined", filename, line);
                    } else {
                        if ((row != kbd_rshiftrow) || (col != kbd_rshiftcol)) {
                            log_warning(keyboard_log, "%s:%d: SHIFT flag used but row and/or col differs from !RSHIFT definition", filename, line);
                        }
                    }
                }
                if (shift & SHIFT_LOCK) {
                    if (!shiftlock_defined()) {
                        log_warning(keyboard_log, "%s:%d: SHIFT-lock flag used but no !SHIFTL defined", filename, line);
                    } else {
                        if (shiftl == KEY_RSHIFT) {
                            if ((row != kbd_rshiftrow) || (col != kbd_rshiftcol)) {
                                log_warning(keyboard_log, "%s:%d: SHIFT-lock flag used but row and/or col differs from !RSHIFT definition", filename, line);
                            }
                        } else if (shiftl == KEY_LSHIFT) {
                            if ((row != kbd_lshiftrow) || (col != kbd_lshiftcol)) {
                                log_warning(keyboard_log, "%s:%d: SHIFT-lock flag used but row and/or col differs from !LSHIFT definition", filename, line);
                            }
                        }
                    }
                }
                
                if (lshift_defined()) {
                    if ((row == kbd_lshiftrow) && (col == kbd_lshiftcol)) {
                        if ((!(shift & LEFT_SHIFT)) && (!(shift & (RIGHT_SHIFT | SHIFT_LOCK)))) {
                            log_warning(keyboard_log, "%s:%d: !LSHIFT defined but key does not use SHIFT flag", filename, line);
                        }
                    }
                }
                if (rshift_defined()) {
                    if ((row == kbd_rshiftrow) && (col == kbd_rshiftcol)) {
                        if ((!(shift & RIGHT_SHIFT)) && (!(shift & (RIGHT_SHIFT | SHIFT_LOCK)))) {
                            log_warning(keyboard_log, "%s:%d: !RSHIFT defined but key does not use SHIFT flag", filename, line);
                        }
                    }
                }
                if (shiftlock_defined()) {
                        if (shiftl == KEY_RSHIFT) {
                            if ((row == kbd_rshiftrow) && (col == kbd_rshiftcol)) {
                                if ((!(shift & SHIFT_LOCK)) && (!(shift & (RIGHT_SHIFT | LEFT_SHIFT)))) {
                                    log_warning(keyboard_log, "%s:%d: !SHIFTL defined but key does not use SHIFT-lock flag", filename, line);
                                }
                            }
                        } else if (shiftl == KEY_LSHIFT) {
                            if ((row == kbd_lshiftrow) && (col == kbd_lshiftcol)) {
                                if ((!(shift & SHIFT_LOCK)) && (!(shift & (RIGHT_SHIFT | LEFT_SHIFT)))) {
                                    log_warning(keyboard_log, "%s:%d: !SHIFTL defined but key does not use SHIFT-lock flag", filename, line);
                                }
                            }
                        }
                }
                
                /* sanity checks for cbm */
                if (shift & VIRTUAL_CBM) {
                    if (!vcbm_defined()) {
                        log_warning(keyboard_log, "%s:%d: virtual CBM flag used but no !VCBM defined", filename, line);
                    }
                }
                if (shift & LEFT_CBM) {
                    if (!lcbm_defined()) {
                        log_warning(keyboard_log, "%s:%d: CBM flag used but no !LCBM defined", filename, line);
                    } else {
                        if ((row != kbd_lcbmrow) || (col != kbd_lcbmcol)) {
                            log_warning(keyboard_log, "%s:%d: CBM flag used but row and/or col differs from !LCBM definition", filename, line);
                        }
                    }
                }
                if (lcbm_defined()) {
                    if ((row == kbd_lcbmrow) && (col == kbd_lcbmcol)) {
                        if (!(shift & LEFT_CBM)) {
                            log_warning(keyboard_log, "%s:%d: !LCBM defined but key does not use CBM flag", filename, line);
                        }
                    }
                }
                /* sanity checks for ctrl */
                if (shift & VIRTUAL_CTRL) {
                    if (!vctrl_defined()) {
                        log_warning(keyboard_log, "%s:%d: virtual CTRL flag used but no !VCTRL defined", filename, line);
                    }
                }
                if (shift & LEFT_CTRL) {
                    if (!lctrl_defined()) {
                        log_warning(keyboard_log, "%s:%d: CTRL flag used but no !LCTRL defined", filename, line);
                    } else {
                        if ((row != kbd_lctrlrow) || (col != kbd_lctrlcol)) {
                            log_warning(keyboard_log, "%s:%d: CTRL flag used but row and/or col differs from !LCTRL definition", filename, line);
                        }
                    }
                }
                if (lctrl_defined()) {
                    if ((row == kbd_lctrlrow) && (col == kbd_lctrlcol)) {
                        if (!(shift & LEFT_CTRL)) {
                            log_warning(keyboard_log, "%s:%d: !LCTRL defined but key does not use CTRL flag", filename, line);
                        }
                    }
                }
            }
        }
    }
}

static int check_modifiers(const char *filename)
{
    int n = 0;
    char *ms[8] = {
        "!LSHIFT ", "!RSHIFT ", "!VSHIFT! ", "!LCBM ", "!VCBM ", "!LCTRL ", "!VCTRL ", "!SHIFTL "
    };
    
    if (!lshift_defined()) {
        n |= (1 << 0);
    }
    if (!rshift_defined()) {
        n |= (1 << 1);
    }
    if (!vshift_defined()) {
        n |= (1 << 2);
    }
    if (!lcbm_defined()) {
        n |= (1 << 3);
    }
    if (!vcbm_defined()) {
        n |= (1 << 4);
    }
    if (!lctrl_defined()) {
        n |= (1 << 5);
    }
    if (!vctrl_defined()) {
        n |= (1 << 6);
    }
    if (!shiftlock_defined()) {
        n |= (1 << 7);
    }
    if (n) {
        log_warning(keyboard_log, "%s: %s%s%s%s%s%s%s%snot defined.",
            filename,        
            n & (1 << 0) ? ms[0] : "",
            n & (1 << 1) ? ms[1] : "",
            n & (1 << 2) ? ms[2] : "",
            n & (1 << 3) ? ms[3] : "",
            n & (1 << 4) ? ms[4] : "",
            n & (1 << 5) ? ms[5] : "",
            n & (1 << 6) ? ms[6] : "",
            n & (1 << 7) ? ms[7] : ""
        );
        return -1;
    }
    return 0;
}

static int keyboard_parse_keymap(const char *filename, int child)
{
    FILE *fp;
    char *complete_path = NULL;
    char buffer[1024];
    int line = 0;

    DBG((">keyboard_parse_keymap(%s)\n", filename));

    /* open in binary mode so the newline system doesn't matter */
    fp = sysfile_open(filename, &complete_path, "rb");

    if (fp == NULL) {
        log_message(keyboard_log, "Error loading keymap `%s'->`%s'.", filename, complete_path ? complete_path : "<empty/null>");
        DBG(("<keyboard_parse_keymap(%s) ERROR\n", filename));
        return -1;
    }

    log_message(keyboard_log, "%s keymap `%s'.", child ? " including" : "Loading", complete_path);

    do {
        buffer[0] = 0;
        if (fgets(buffer, 999, fp)) {
            char *p;
            long blen = (long)strlen(buffer);

            line++;
            
            if (blen == 0) {
                break;
            }

            /* remove trailing CR or/and LF */
            blen--;
            while (blen >= 0 && (buffer[blen] == '\n' || buffer[blen] == '\r')) {
                buffer[blen--] = '\0';
            }

            /* remove comments */
            if ((p = strchr(buffer, '#'))) {
                *p = 0;
            }

            switch (*buffer) {
                case 0:
                    break;
                case '!':
                    /* keyword handling */
                    keyboard_parse_keyword(buffer, line, filename);
                    break;
                default:
                    /* table entry handling */
                    keyboard_parse_entry(buffer, line, filename);
                    break;
            }
        }
    } while (!feof(fp));
    fclose(fp);

    lib_free(complete_path);
    
    check_modifiers(filename);
    
    DBG(("<keyboard_parse_keymap OK\n"));
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
    int i, j;

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
            "# '!LCTRL row col'       left control keyboard row/column\n"
            "# '!VCTRL ctrlkey'       virtual control key (LCTRL)\n"
            "# '!LCBM row col'        left CBM keyboard row/column\n"
            "# '!VCBM cbmkey'         virtual CBM key (LCBM)\n"
            "# '!UNDEF keysym'        remove keysym from table\n"
            "#\n"
            "# Shiftflag can have the values:\n"
            "# 0      key is not shifted for this keysym/scancode\n"
            "# 1      key is combined with shift for this keysym/scancode\n"
            "# 2      key is left shift on emulated machine\n"
            "# 4      key is right shift on emulated machine\n"
            "# 8      key can be shifted or not with this keysym/scancode\n"
            "# 16     deshift key for this keysym/scancode\n"
            "# 32     another definition for this keysym/scancode follows\n"
            "# 64     key is shift-lock on emulated machine\n"
            "# 128    shift modifier required on host\n"
            "# 256    key is used for an alternative keyboard mapping\n"
            "# 512    alt-r (alt-gr) modifier required on host\n"
            "# 1024   ctrl modifier required on host\n"
            "# 2048   key is combined with cbm for this keysym/scancode\n"
            "# 4096   key is combined with ctrl for this keysym/scancode\n"
            "# 8192   key is (left) cbm on emulated machine\n"
            "# 16384  key is (left) ctrl on emulated machine\n"
            "#\n"
            "# to migrate older keymaps and use the CBM and/or CTRL related features:\n"
            "#\n"
            "# - define !LCTRL, !VCTRL, !LCBM, !VCBM\n"
            "# - add 'key is (left) cbm/ctrl on emulated machine' flags to\n"
            "#   all keys that map to the cbm or ctrl key respectively.\n"
            "#\n"
            "# after that the virtual cbm/ctrl flags and requiring host modifiers\n"
            "# should work as expected. keep an eye on the error messages.\n"
            "#\n"
            "# Negative row values:\n"
            "# 'keysym -1 n' joystick keymap A, direction n\n"
            "# 'keysym -2 n' joystick keymap B, direction n\n"
            "# 'keysym -3 0' first RESTORE key\n"
            "# 'keysym -3 1' second RESTORE key\n"
            "# 'keysym -4 0' 40/80 column key\n"
            "# 'keysym -4 1' CAPS (ASCII/DIN) key\n"
            "# 'keysym -5 n' joyport keypad, key n\n"
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
            "#\n"
            "# Joyport keypad key layout:\n"
            "# --------------------------\n"
            "# |  0 |  1 |  2 |  3 |  4 |\n"
            "# --------------------------\n"
            "# |  5 |  6 |  7 |  8 |  9 |\n"
            "# --------------------------\n"
            "# | 10 | 11 | 12 | 13 | 14 |\n"
            "# --------------------------\n"
            "# | 15 | 16 | 17 | 18 | 19 |\n"
            "# --------------------------\n"
            "#\n"
            "# When a bigger spaced key is used,\n"
            "# it uses the upper left most key value.\n"
           );

    /* FIXME: output the keyboard matrix for the respective target */

    fprintf(fp, "!CLEAR\n");
    if (lshift_defined()) {
        fprintf(fp, "!LSHIFT %d %d\n", kbd_lshiftrow, kbd_lshiftcol);
    }
    if (rshift_defined()) {
        fprintf(fp, "!RSHIFT %d %d\n", kbd_rshiftrow, kbd_rshiftcol);
    }
    if (vshift_defined()) {
        fprintf(fp, "!VSHIFT %s\n", (vshift == KEY_RSHIFT) ? "RSHIFT" : "LSHIFT");
    }
    if (shiftlock_defined()) {
        fprintf(fp, "!SHIFTL %s\n", (shiftl == KEY_RSHIFT) ? "RSHIFT" : "LSHIFT");
    }
    if (lctrl_defined()) {
        fprintf(fp, "!LCTRL %d %d\n", kbd_lctrlrow, kbd_lctrlcol);
    }
    if (vctrl_defined()) {
        fprintf(fp, "!VCTRL %s\n", (vctrl == KEY_LCTRL) ? "LCTRL" : "?");
    }
    if (lcbm_defined()) {
        fprintf(fp, "!LCBM %d %d\n", kbd_lcbmrow, kbd_lcbmcol);
    }
    if (vcbm_defined()) {
        fprintf(fp, "!VCBM %s\n", (vcbm == KEY_LCBM) ? "LCBM" : "?");
    }
    fprintf(fp, "\n");

    for (i = 0; keyconvmap[i].sym != ARCHDEP_KEYBOARD_SYM_NONE; i++) {
        fprintf(fp, "%s %d %d %u\n",
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

    fprintf(fp, "#\n"
                "# joyport attached keypad key mapping\n"
                "#\n");
    for (i = 0; i < KBD_JOY_KEYPAD_ROWS; ++i) {
        for (j = 0; j < KBD_JOY_KEYPAD_COLS; ++j) {
            if (key_joy_keypad[i][j] != -1) {
                fprintf(fp, "%s -5 %d\n",
                    kbd_arch_keynum_to_keyname(key_joy_keypad[i][j]), (i * KBD_JOY_KEYPAD_COLS) + j);
            }
        }
    }
#ifdef COMMON_JOYKEYS
    for (i = 0; i < JOYSTICK_KEYSET_NUM_KEYS; i++) {
        if (joykeys[JOYSTICK_KEYSET_IDX_A][i] != ARCHDEP_KEYBOARD_SYM_NONE) {
            fprintf(fp, "#\n"
                    "# Joystick keyset A mapping\n"
                    "#\n");
            for (i = 0; i < JOYSTICK_KEYSET_NUM_KEYS; i++) {
                if (joykeys[JOYSTICK_KEYSET_IDX_A][i] != ARCHDEP_KEYBOARD_SYM_NONE) {
                    fprintf(fp, "%s -1 %d\n",
                        kbd_arch_keynum_to_keyname(joykeys[JOYSTICK_KEYSET_IDX_A][i]), i);
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
                    fprintf(fp, "%s -2 %d\n",
                        kbd_arch_keynum_to_keyname(joykeys[JOYSTICK_KEYSET_IDX_B][i]), i);
                }
            }
            fprintf(fp, "\n");
            break;
        }
    }
#endif

    fclose(fp);

    return 0;
}

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


static int machine_keyboard_mapping = 0;
static int machine_keyboard_type = 0;

static int try_set_keymap_file(int atidx, int idx, int mapping, int type);

#define KBD_SWITCH_DEFAULT      0
#define KBD_SWITCH_MAPPING      1
#define KBD_SWITCH_INDEX        2
#define KBD_SWITCH_TYPE         3
static int switch_keymap_file(int sw, int *idxp, int *mapp, int *typep);

/* (re)load keymap at index */
static int load_keymap_file(int val)
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
        if (switch_keymap_file(KBD_SWITCH_INDEX, &val, &mapping, &type) < 0) {
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

/* handle change of "KeyboardType" */
static int keyboard_set_keyboard_type(int val, void *param)
{
    int idx, mapping;

    mapping = machine_keyboard_mapping;
    idx = machine_keymap_index;

    DBG((">keyboard_set_keyboard_type(idx:%d mapping:%d type:%d)\n", idx, mapping, val));
    if (idx < 2) {
        if (switch_keymap_file(KBD_SWITCH_TYPE, &idx, &mapping, &val) < 0) {
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
static int keyboard_set_keyboard_mapping(int val, void *param)
{
    int type;
    int idx;


    type = machine_keyboard_type;
    idx = machine_keymap_index;
    DBG((">keyboard_set_keyboard_mapping(%d,%d,%d)\n", idx, type, val));

    if (idx < 2) {
        if (switch_keymap_file(KBD_SWITCH_MAPPING, &idx, &val, &type) < 0) {
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

/* (keep in sync with constants in keyboard.h) */
static mapping_info_t kbdinfo[KBD_MAPPING_NUM + 1] = {
    { "American (us)", KBD_MAPPING_US, "" },    /* this must be first (=0) always */
    { "British (uk)", KBD_MAPPING_UK, "uk" },
    { "Danish (da)", KBD_MAPPING_DA, "da" },
    { "Dutch (nl)", KBD_MAPPING_NL, "nl" },
    { "Finnish (fi)", KBD_MAPPING_FI, "fi" },
    { "German (de)", KBD_MAPPING_DE, "de" },
    { "Italian (it)", KBD_MAPPING_IT, "it" },
    { "Norwegian (no)", KBD_MAPPING_NO, "no" },
    { "Swedish (se)", KBD_MAPPING_SE, "se" },
    { "Swiss (ch)", KBD_MAPPING_CH, "ch" },
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

static char *keyboard_get_keymap_name(int idx, int mapping, int type)
{
    char *sympos[2] = { "sym", "pos"};
    char *mapname;
    char *name = NULL, *tstr = NULL;

    DBG((">keyboard_get_keymap_name idx %d mapping %d type %d\n", idx, mapping, type));
    if (type >= 0) {
        tstr = machine_get_keyboard_type_name(type);
    }
    mapname = keyboard_get_mapping_name(mapping);

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

    DBG(("keyboard_get_keymap_name: (port:%s type:%s idx:%d mapping:%d) '%s' = '%s'\n",
                KBD_PORT_PREFIX, tstr ? tstr : "-", idx, mapping,
                idx ? "KeymapPosFile" : "KeymapSymFile", name));

    return name;
}

/** \brief  Check if a keymap exists for given layout / mapping / emulated keyboard
 *
 * \param[in]   sympos      Symbolic or Positional mapping , KBD_INDEX_SYM or KBD_INDEX_POS
 * \param[in]   hosttype    Type of Host Layout, KBD_MAPPING_... (mapping_info_t .mapping)
 * \param[in]   kbdtype     Emulated Keyboard type, KBD_TYPE_... (kbdtype_info_t .type) or -1 if no different types exist
 *
 * \return  0: ok !=0: error
 */
int keyboard_is_keymap_valid(int sympos, int hosttype, int kbdtype)
{
    char *name = NULL;
    char *complete_path;
    int res;

    name = keyboard_get_keymap_name(sympos, hosttype, kbdtype);
    res = sysfile_locate(name, &complete_path);

    lib_free(name);
    lib_free(complete_path);
    return res;
}

/** \brief  Check if a keymap exists for given host layout
 *
 * \param[in]   hosttype    Type of Host Layout, KBD_MAPPING_... (mapping_info_t .mapping)
 *
 * \return  0: ok !=0: error
 */
int keyboard_is_hosttype_valid(int hosttype)
{
    int numtypes = machine_get_num_keyboard_types();
    kbdtype_info_t *typelist = machine_get_keyboard_info_list();
    int i, type;

    for (i = 0; i < numtypes; i++) {
        if (typelist) {
            type = typelist[i].type;
        } else {
            type = 0;
        }
        if ((keyboard_is_keymap_valid(KBD_INDEX_SYM, hosttype, type) == 0) ||
            (keyboard_is_keymap_valid(KBD_INDEX_POS, hosttype, type) == 0)) {
            return 0;
        }
    }
    return -1;
}

static int try_set_keymap_file(int atidx, int idx, int mapping, int type)
{
    char *name = NULL;
    char *complete_path;

    name = keyboard_get_keymap_name(idx, mapping, type);

    util_string_set(&machine_keymap_file_list[atidx], name);
    DBG(("try_set_keymap_file calls sysfile_locate(%s)\n", name));
    if (sysfile_locate(name, &complete_path) != 0) {
        DBG(("<try_set_keymap_file ERROR locating keymap `%s'.\n", name ? name : "(null)"));
        lib_free(name);
        lib_free(complete_path);
        return -1;
    }
    lib_free(name);
    lib_free(complete_path);
    DBG(("<try_set_keymap_file OK\n"));
    return 0;
}

static int switch_keymap_file(int sw, int *idxp, int *mapp, int *typep)
{
    int type = *typep;
    int mapping = *mapp;
    int idx = *idxp;
    int atidx = *idxp;

    DBG((">switch_keymap_file idx %d mapping %d type %d\n", *idxp, *mapp, *typep));
    if(try_set_keymap_file(atidx, idx, mapping, type) >= 0) {
        goto ok;
    }

    /* when switching host layout or emulated keyboard type, try the "other" 
       index first if the current one does not exist */
    if ((sw == KBD_SWITCH_MAPPING) || (sw == KBD_SWITCH_TYPE)) {
        switch (idx) {
            case KBD_INDEX_SYM:
                if(try_set_keymap_file(atidx, KBD_INDEX_POS, mapping, type) >= 0) {
                    idx = KBD_INDEX_POS;
                    goto ok;
                }
                break;
            case KBD_INDEX_POS:
                if(try_set_keymap_file(atidx, KBD_INDEX_SYM, mapping, type) >= 0) {
                    idx = KBD_INDEX_SYM;
                    goto ok;
                }
                break;
        }
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

/* called by keyboard_resources_init to create the default keymap(s) 
   idx is the index to the resource for the setting ("KeymapIndex") 
 */
static int keyboard_set_default_keymap_file(int idx)
{
    int mapping = 0;
    int type = 0;

    DBG((">keyboard_set_default_keymap_file(%d)\n", idx));

    if ((idx != KBD_INDEX_SYM) && (idx != KBD_INDEX_POS)) {
        /* it's a user keymap, do not set a default */
        return -1;
    }
    /* host keyboard layout type */
    if (resources_get_int("KeyboardMapping", &mapping) < 0) {
        return -1;
    }
    /* emulated keyboard type */
    if (resources_get_int("KeyboardType", &type) < 0) {
        return -1;
    }
    
    if(switch_keymap_file(KBD_SWITCH_DEFAULT, &idx, &mapping, &type) < 0) {
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
      &machine_keymap_file_list[KBD_INDEX_SYM],
      keyboard_set_keymap_file, (void *)KBD_INDEX_SYM },
    { "KeymapPosFile", "", RES_EVENT_NO, NULL,
      &machine_keymap_file_list[KBD_INDEX_POS],
      keyboard_set_keymap_file, (void *)KBD_INDEX_POS },
    { "KeymapUserSymFile", "", RES_EVENT_NO, NULL,
      &machine_keymap_file_list[KBD_INDEX_USERSYM],
      keyboard_set_keymap_file, (void *)KBD_INDEX_USERSYM },
    { "KeymapUserPosFile", "", RES_EVENT_NO, NULL,
      &machine_keymap_file_list[KBD_INDEX_USERPOS],
      keyboard_set_keymap_file, (void *)KBD_INDEX_USERPOS },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "KeymapIndex", KBD_INDEX_SYM, RES_EVENT_NO, NULL,
      &machine_keymap_index, keyboard_set_keymap_index, NULL },
    { "KeyboardType", 0, RES_EVENT_NO, NULL,
      &machine_keyboard_type, keyboard_set_keyboard_type, NULL },
    { "KeyboardMapping", 0, RES_EVENT_NO, NULL,
      &machine_keyboard_mapping, keyboard_set_keyboard_mapping, NULL },
    { "KbdStatusbar", 0, RES_EVENT_NO, NULL,
      &kbd_statusbar_enabled, keyboard_set_keyboard_statusbar, NULL },
    RESOURCE_INT_LIST_END
};

/*--------------------------------------------------------------------------*/

int keyboard_resources_init(void)
{
    int nsym, npos, mapping, idx, type;
    const char *name;

    /* VSID doesn't have a keyboard */
    if (machine_class == VICE_MACHINE_VSID) {
        return 0;
    }

    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    if (resources_register_int(resources_int) < 0) {
        return -1;
    }

    npos = (machine_keymap_file_list[KBD_INDEX_POS] == NULL) || (machine_keymap_file_list[KBD_INDEX_POS][0] == 0);
    nsym = (machine_keymap_file_list[KBD_INDEX_SYM] == NULL) || (machine_keymap_file_list[KBD_INDEX_SYM][0] == 0);

    DBG((">>keyboard_resources_init(first start:%s)\n", (npos && nsym) ? "yes" : "no"));

    if (npos && nsym) {
        mapping = archdep_kbd_get_host_mapping();
        log_verbose("Setting up default keyboard mapping for host type %d (%s)",
                    mapping, keyboard_get_mapping_name(mapping));
        if (resources_set_int("KeymapIndex", KBD_INDEX_SYM) < 0) {
            /* return -1; */
        }
        /* host keyboard mapping */
        if (resources_set_int("KeyboardMapping", mapping) < 0) {
            /* return -1; */
        }
        
        keyboard_set_default_keymap_file(KBD_INDEX_POS);
        if (resources_get_string("KeymapPosFile", &name) < 0) {
            DBG(("<<keyboard_resources_init(error)\n"));
            return -1;
        }
        util_string_set(&resources_string_d1, name);
        util_string_set(&resources_string_d3, name);

        log_verbose("Default positional map is: %s", name);
        keyboard_set_default_keymap_file(KBD_INDEX_SYM);
        if (resources_get_string("KeymapSymFile", &name) < 0) {
            DBG(("<<keyboard_resources_init(error)\n"));
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
            DBG(("<<keyboard_resources_init(error)\n"));
            return -1;
        }
        if (resources_get_int("KeyboardType", &type) < 0) {
            DBG(("<<keyboard_resources_init(error)\n"));
            return -1;
        }
        if (resources_get_int("KeyboardMapping", &mapping) < 0) {
            DBG(("<<keyboard_resources_init(error)\n"));
            return -1;
        }
        resources_set_default_int("KeymapIndex", idx);
        resources_set_default_int("KeyboardType", type);
        resources_set_default_int("KeyboardMapping", mapping);
    }
    DBG(("<<keyboard_resources_init(ok)\n"));
    return 0;
}

static void keyboard_resources_shutdown(void)
{
    /* VSID doesn't have a keyboard */
    if (machine_class == VICE_MACHINE_VSID) {
        return;
    }
    lib_free(machine_keymap_file_list[KBD_INDEX_SYM]);
    lib_free(machine_keymap_file_list[KBD_INDEX_POS]);
    lib_free(machine_keymap_file_list[KBD_INDEX_USERSYM]);
    lib_free(machine_keymap_file_list[KBD_INDEX_USERPOS]);
    lib_free(resources_string_d0);
    lib_free(resources_string_d1);
    lib_free(resources_string_d2);
    lib_free(resources_string_d3);
}

/*--------------------------------------------------------------------------*/

static cmdline_option_t const cmdline_options[] =
{
    { "-keymap", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KeymapIndex", NULL,
      "<number>", "Specify index of keymap file (0=symbolic, 1=positional, 2=symbolic (user), 3=positional (user))" },
/* FIXME: build description dynamically */
    { "-keyboardmapping", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KeyboardMapping", NULL,
      "<number>", "Specify host keyboard layout" },
/* FIXME: build description dynamically */
    { "-keyboardtype", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KeyboardType", NULL,
      "<number>", "Specify emulated keyboard type" },
    { "-symkeymap", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KeymapUserSymFile", NULL,
      "<Name>", "Specify name of symbolic keymap file" },
    { "-poskeymap", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KeymapUserPosFile", NULL,
      "<Name>", "Specify name of positional keymap file" },

    /* enable keyboard debugging display in the statusbar */
    { "-kbdstatusbar", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "KbdStatusbar", (resource_value_t)1,
        NULL, "Enable keyboard-status bar" },

    /* disable keyboard debugging display in the statusbar */
    { "+kbdstatusbar", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "KbdStatusbar", (resource_value_t)0,
        NULL, "Enable keyboard-status bar" },

    CMDLINE_LIST_END
};

int keyboard_cmdline_options_init(void)
{
    if (machine_class != VICE_MACHINE_VSID) {
        return cmdline_register_options(cmdline_options);
    }
    return 0;
}

/*--------------------------------------------------------------------------*/

void keyboard_init(void)
{
    keyboard_log = log_open("Keyboard");

    keyboard_alarm = alarm_new(maincpu_alarm_context, "Keyboard",
                            keyboard_latch_handler, NULL);
    restore_alarm = alarm_new(maincpu_alarm_context, "Restore",
                            restore_alarm_triggered, NULL);

    kbd_arch_init();

    if (machine_class != VICE_MACHINE_VSID) {
        load_keymap_ok = 1;
        keyboard_set_keymap_index(machine_keymap_index, NULL);
    }
}

void keyboard_shutdown(void)
{
    keyboard_keyconvmap_free();
    keyboard_resources_shutdown();      /* FIXME: perhaps call from elsewhere? */
}

/*--------------------------------------------------------------------------*/

#define SNAP_MAJOR 1
#define SNAP_MINOR 0
#define SNAP_NAME  "KEYBOARD"

int keyboard_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_NAME, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_DWA(m, (uint32_t *)keyarr, KBD_ROWS) < 0
        || SMW_DWA(m, (uint32_t *)rev_keyarr, KBD_COLS) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int keyboard_snapshot_read_module(snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_NAME, &major_version, &minor_version);

    if (m == NULL) {
        return 0;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        snapshot_module_close(m);
        return -1;
    }

    if (0
        || SMR_DWA(m, (uint32_t *)keyarr, KBD_ROWS) < 0
        || SMR_DWA(m, (uint32_t *)rev_keyarr, KBD_COLS) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}
