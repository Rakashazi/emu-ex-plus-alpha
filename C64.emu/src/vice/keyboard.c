/** \file   keyboard.c
 * \brief   Common keyboard emulation.
 *
 * \author  Andreas Boose <viceteam@t-online.de>
 * \author  Ettore Perazzoli <ettore@comm2000.it>
 * \author  Jouko Valta <jopi@stekt.oulu.fi>
 * \author  Andre Fachat <fachat@physik.tu-chemnitz.de>
 * \author  Bernhard Kuhn <kuhn@eikon.e-technik.tu-muenchen.de>
 * \author  groepaz <groepaz@gmx.net>
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

/* #define DBGKBD */
/* #define DBGKBD_KEYS */
/* #define DBGKBD_MODIFIERS */
/* #define DBGKBD_MATRIX */

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
#include "cmdline.h"
#include "joystick.h"
#include "kbd.h"
#include "keyboard.h"
#include "keymap.h"
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

#ifdef DBGKBD
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

#ifdef DBGKBD_KEYS
#define DBGKEY(x)  log_debug x
#else
#define DBGKEY(x)
#endif

#ifdef DBGKBD_MODIFIERS
#define DBGMOD(x)  log_debug x
#else
#define DBGMOD(x)
#endif

#ifdef DBGKBD_MATRIX
#define DBGMTX(x)  log_debug x
#else
#define DBGMTX(x)
#endif

#define KEYBOARD_RAND() lib_unsigned_rand(1, (unsigned int)machine_get_cycles_per_frame())

static log_t keyboard_log = LOG_DEFAULT;

/* Keyboard array passed to keyboard_machine_func(). copied from either
   latch_keyarr or network_keyarr. */
int keyarr[KBD_ROWS];
int rev_keyarr[KBD_COLS];

/* Keyboard status to be latched into the keyboard array.  */
static int latch_keyarr[KBD_ROWS];
static int latch_rev_keyarr[KBD_COLS];

static int network_keyarr[KBD_ROWS];
static int network_rev_keyarr[KBD_COLS];

/* Shift lock state. (emulated keyboard) */
static int keyboard_shiftlock = 0;

static alarm_t *keyboard_alarm = NULL;

/* machine specific utility function that is called after the keyboard was latched */
static keyboard_machine_func_t keyboard_machine_func = NULL;

static CLOCK keyboard_delay = 0; /* event playback */

static int keyboard_clear = 0; /* event playback, =1 when keyboard reset is queued */

/* Is an alternative mapping active? */
static int key_alternative = 0;

/* host modifier flags */
static int left_shift_down;
static int right_shift_down;
static int left_cbm_down;
static int left_ctrl_down;

/* virtual modifiers (combined) */
static int virtual_shift_down;
static int virtual_cbm_down;
static int virtual_ctrl_down;
static int virtual_deshift;

/* virtual modifiers per emulated/mapped key */
static int virtual_modifier_flags[KBD_ROWS][KBD_COLS];

/* the last timestamp/delay goes here */
static CLOCK keyboard_latch_timestamp = 0;

/* joyport attached keypad. */
static key_joy_keypad_func_t key_joy_keypad_func = NULL;
signed long key_joy_keypad[KBD_JOY_KEYPAD_ROWS][KBD_JOY_KEYPAD_COLS]; /* FIXME */

/** \brief  Resource value for KdbStatusbar
 *
 * Determines whether to show the keyboard debugging widget on the statusbar.
 */
static int kbd_statusbar_enabled = 0;

typedef struct {
    signed long key;
    int mod;
    int pressed;
} KBD_QUEUE ;

#define KBD_QUEUE_MAX   8  /* that should be enough really */

static int kbd_queue_read = 0, kbd_queue_write = 0;
static KBD_QUEUE kbd_queue[KBD_QUEUE_MAX];

static int restore_raw = 0;
static int restore_delayed = 0;
static int restore_quick_release = 0;
static alarm_t *restore_alarm = NULL; /* restore key alarm context */

static int keyboard_restore_pressed(void);
static int keyboard_restore_released(void);

static void keyboard_event_record(void);

/* custom keys */
static int keyboard_custom_key_func_by_keysym(int keysym, int pressed);

static void keyboard_key_clear_internal(void);

/* keyboard alarm handler, this consumes the host keyboard queue */
static void keyboard_alarm_handler(CLOCK offset, void *data);

/******************************************************************************/

/* FIXME: the SDL port uses this directly */
void keyboard_clear_keymatrix(void)
{
    memset(keyarr, 0, sizeof(keyarr));
    memset(rev_keyarr, 0, sizeof(rev_keyarr));
    memset(latch_keyarr, 0, sizeof(latch_keyarr));
    memset(latch_rev_keyarr, 0, sizeof(latch_rev_keyarr));
    keyboard_shiftlock = 0;
}

/* register a machine specific utility function that is called after the keyboard was latched */
void keyboard_register_machine(keyboard_machine_func_t func)
{
    keyboard_machine_func = func;
}

void keyboard_register_delay(unsigned int delay)
{
    keyboard_delay = delay;
}

/* used from event code, queue a keyboard reset */
void keyboard_register_clear(void)
{
    keyboard_clear = 1;
}

/******************************************************************************/

/* make a new random timestep for the next keyboard event. :
   - the timestamp will always be after keyboard_latch_timestamp
   - the period until next timestep will get gradually shorter, depending on
     "num", the number of keyboard events already queued.
*/
static CLOCK kbd_make_event_timestap(CLOCK offset, int num)
{
    int maxdiff = (int)machine_get_cycles_per_frame() * 2;

    if (offset < maincpu_clk) {
        offset = maincpu_clk;
    }
    if (offset < keyboard_latch_timestamp) {
        offset = keyboard_latch_timestamp;
    }
    /* add a small constant amount of cycles */
    offset += 1000;

    /* add a random amount of cycles, at most one frame */
    if (num == 0) { num = 1; }
    offset += (KEYBOARD_RAND() / num);

    /* limit to at most two frames */
    if (offset > (maincpu_clk + maxdiff)) {
        offset = maincpu_clk + maxdiff;
    }

    keyboard_latch_timestamp = offset;
    return offset;
}

static void kbd_limit_pointers(void)
{
#if 0
    kbd_queue_read &= (KBD_QUEUE_MAX - 1);
    kbd_queue_write &= (KBD_QUEUE_MAX - 1);
#endif
    if ((kbd_queue_read < 0) ||
        (kbd_queue_write < 0) ||
        (kbd_queue_read > (KBD_QUEUE_MAX - 1)) ||
        (kbd_queue_write > (KBD_QUEUE_MAX - 1))) {
        log_error(keyboard_log, "kbd_limit_pointers wth?");
        kbd_queue_read = kbd_queue_write = 0;
        keyboard_key_clear_internal();
        alarm_set(keyboard_alarm, kbd_make_event_timestap(maincpu_clk, 0));
    }
}

static void kbd_retrigger_alarm(void)
{
    int queue_num;
    alarm_unset(keyboard_alarm);
    queue_num = 0;
    kbd_limit_pointers();
    if (kbd_queue_write > kbd_queue_read) {
        queue_num = kbd_queue_write - kbd_queue_read;
    } else if (kbd_queue_write < kbd_queue_read) {
        queue_num = kbd_queue_read - kbd_queue_write;
    }
    alarm_set(keyboard_alarm, kbd_make_event_timestap(maincpu_clk, queue_num));
}

/* push one (host) key to the processing queue.
   returns 0 on error (queue full) or 1 on success */
static int kbd_queue_pushkey(int key, int mod, int pressed)
{
    static int last_key = -1, last_mod =-1, last_pressed = -1;
    if ((key != last_key) ||
        (mod != last_mod) ||
        (pressed != last_pressed)) {
        kbd_limit_pointers();
        if (((kbd_queue_write + 1) & (KBD_QUEUE_MAX - 1)) != kbd_queue_read) {
            /* printf("kbd_queue_pushkey: %d key %d\n", kbd_queue_write, key); */
            kbd_queue[kbd_queue_write].key = last_key = key;
            kbd_queue[kbd_queue_write].mod = last_mod = mod;
            kbd_queue[kbd_queue_write].pressed = last_pressed = pressed;
            kbd_queue_write = (kbd_queue_write + 1) & (KBD_QUEUE_MAX - 1);
            return 1;
        } else {
            DBGKEY(("kbd_queue_pushkey buffer full!"));
#if 0
            keyboard_key_clear_internal();
            kbd_queue_read = kbd_queue_write;
            /* trigger alarm right now, to make room in the queue asap */
            keyboard_alarm_handler(maincpu_clk, NULL);
#endif
            kbd_retrigger_alarm();
        }
    }
    return 0;
}

/* get one (host) key from the processing queue.
   returns 0 on error (queue full) or 1 on success */
static int kbd_queue_popkey(int *key, int *mod, int *pressed)
{
    kbd_limit_pointers();
    if (kbd_queue_write != kbd_queue_read) {
        *key = (int)kbd_queue[kbd_queue_read].key;
        *mod = (int)kbd_queue[kbd_queue_read].mod;
        *pressed = kbd_queue[kbd_queue_read].pressed;
        kbd_queue_read = (kbd_queue_read + 1) & (KBD_QUEUE_MAX - 1);
        return 1;
    }
    return 0;
}

/******************************************************************************/

/* copy the latched keyarr (or what comes from the network) to the real keyarr */
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

/* update keyboard latch, returns 0 on success, -1 on error */
static int keyboard_set_latch_keyarr(int row, int col, int pressed)
{
    if (row < 0 || col < 0) {
        return -1;
    }
    DBGMTX(("keyboard_set_latch_keyarr     row:%d col:%d pressed:%d", row, col, pressed));
    if (pressed) {
        latch_keyarr[row] |= 1 << col;
        latch_rev_keyarr[col] |= 1 << row;
    } else {
        latch_keyarr[row] &= ~(1 << col);
        latch_rev_keyarr[col] &= ~(1 << row);
    }
#if 0
    {
        int r, c;
        for (r = 7; r >= 0; r--) {
            for (c = 7; c >= 0; c--) {
                printf("%c", latch_keyarr[r] & (1 << c) ? '*' : '.');
            }
            printf("\n");
        }
        printf("\n");
    }
#endif
    return 0;
}


/* used for event playback, triggering keys from Joystick events, virtual keyboard */
void keyboard_set_keyarr(int row, int col, int value)
{
    DBGMTX(("keyboard_set_keyarr       val:%d row:%d col:%d \n", value, row, col));
    if (keyboard_set_latch_keyarr(row, col, value) < 0) {
        return;
    }

    keyboard_latch_matrix(kbd_make_event_timestap(maincpu_clk, 2));
    /*keyboard_event_record();*/    /* ? */
    /* FIXME: perhaps trigger alarm here? */
}

/*-----------------------------------------------------------------------*/

static void clear_modifier_flags(void)
{
    left_cbm_down = left_ctrl_down =
    virtual_cbm_down = virtual_shift_down = virtual_deshift =
        left_shift_down = right_shift_down = keyboard_shiftlock = 0;
}

/*-----------------------------------------------------------------------*/

static void clear_virtual_modifier_flags(void) {
    int row, column;
    for (row = 0; row < KBD_ROWS; row++) {
        for (column = 0; column < KBD_COLS; column++) {
            virtual_modifier_flags[row][column] = 0;
        }
    }
}

/* combine virtual modifier flags from all keys */
static inline void update_virtual_modifier_flags(void) {
    int row, column, shift;
    virtual_shift_down = 0;
    virtual_cbm_down = 0;
    virtual_ctrl_down = 0;
    virtual_deshift = 0;
    for (row = 0; row < KBD_ROWS; row++) {
        for (column = 0; column < KBD_COLS; column++) {
            shift = virtual_modifier_flags[row][column];
            virtual_shift_down |= shift & VIRTUAL_SHIFT;
            virtual_cbm_down |= shift & VIRTUAL_CBM;
            virtual_ctrl_down |= shift & VIRTUAL_CTRL;
            virtual_deshift |= shift & DESHIFT_SHIFT;
        }
    }
    if (virtual_deshift && virtual_shift_down) {
        log_warning(keyboard_log, "using deshift + virtual shift at the same time");
    }
    if (virtual_deshift) {
        virtual_shift_down = 0;
    } else if (virtual_shift_down) {
        virtual_deshift = 0;
    }
    DBGMOD(("update_virtual_modifier_flags   vshift:%d vcbm:%d vctrl:%d lshift:%d rshift:%d cbm:%d ctrl:%d deshift:%d",
           virtual_shift_down, virtual_cbm_down, virtual_ctrl_down,
           left_shift_down, right_shift_down, left_cbm_down, left_ctrl_down,
            virtual_deshift
            ));
#if 0
    printf("update_virtual_modifier_flags:\n");
    for (row = KBD_ROWS-1; row >= 0; row--) {
        for (column = KBD_COLS-1; column >= 0; column--) {
            printf("%04x ", virtual_modifier_flags[row][column]);
        }
        printf("\n");
    }
    printf("\n");
#endif
}

/* press virtual modifier per key */
static inline void keyboard_key_pressed_modifier(int row, int column, int shift) {
    virtual_modifier_flags[row][column] |= IS_PRESSED | (shift & (VIRTUAL_SHIFT | VIRTUAL_CTRL | VIRTUAL_CBM | DESHIFT_SHIFT));
    update_virtual_modifier_flags();
    DBGMOD(("keyboard_key_pressed_modifier   vshift:%d vcbm:%d vctrl:%d                                deshift:%d shiftlock:%d",
           virtual_shift_down, virtual_cbm_down, virtual_ctrl_down,
            virtual_deshift,
            keyboard_shiftlock));
}

/* release virtual modifier per key */
static inline void keyboard_key_released_modifier(int row, int column, int shift) {
    virtual_modifier_flags[row][column] &= ~(shift | IS_PRESSED | VIRTUAL_SHIFT | VIRTUAL_CTRL | VIRTUAL_CBM | DESHIFT_SHIFT);
    update_virtual_modifier_flags();
    DBGMOD(("keyboard_key_released_modifier  vshift:%d vcbm:%d vctrl:%d                                deshift:%d shiftlock:%d",
           virtual_shift_down, virtual_cbm_down, virtual_ctrl_down,
            virtual_deshift,
            keyboard_shiftlock));
}

/*-----------------------------------------------------------------------*/

/* combine virtual and "real" modifier flags and latch them into the latch_keyarr matrix */
static void keyboard_latch_modifier_states(void)
{
    int physical_right = (rshift_defined() && (right_shift_down > 0));
    int physical_left = (lshift_defined() && (left_shift_down > 0));

    DBGMOD(("keyboard_latch_modifier_states  vshift:%d vcbm:%d vctrl:%d lshift:%d rshift:%d cbm:%d ctrl:%d shiftlock:%d",
            virtual_shift_down, virtual_cbm_down, virtual_ctrl_down,
            left_shift_down, right_shift_down,
            left_cbm_down, left_ctrl_down,
            keyboard_shiftlock));

    if (lshift_defined()) {
        if (((left_shift_down > 0) && !virtual_deshift) ||
            ((virtual_shift_down > 0) && (key_ctrl_vshift == KEY_LSHIFT) && !physical_right) ||
            ((keyboard_shiftlock > 0) && (key_ctrl_shiftl == KEY_LSHIFT))) {
            keyboard_set_latch_keyarr(kbd_lshiftrow, kbd_lshiftcol, 1);
        } else {
            keyboard_set_latch_keyarr(kbd_lshiftrow, kbd_lshiftcol, 0);
        }
    }

    /* keymaps that only have one shift key use RSHIFT for both, check it last
       so it takes precedence */
    if (rshift_defined()) {
        if (((right_shift_down > 0) && !virtual_deshift) ||
            ((virtual_shift_down > 0) && (key_ctrl_vshift == KEY_RSHIFT) && !physical_left) ||
            ((keyboard_shiftlock > 0) && (key_ctrl_shiftl == KEY_RSHIFT))) {
            keyboard_set_latch_keyarr(kbd_rshiftrow, kbd_rshiftcol, 1);
        } else {
            keyboard_set_latch_keyarr(kbd_rshiftrow, kbd_rshiftcol, 0);
        }
    }

    if (lcbm_defined()) {
        if ((left_cbm_down > 0) ||
            ((virtual_cbm_down > 0) && (key_ctrl_vcbm == KEY_LCBM))) {
            keyboard_set_latch_keyarr(kbd_lcbmrow, kbd_lcbmcol, 1);
        } else {
            keyboard_set_latch_keyarr(kbd_lcbmrow, kbd_lcbmcol, 0);
        }
    }

    if (lctrl_defined()) {
        if ((left_ctrl_down > 0) ||
            ((virtual_ctrl_down > 0) && (key_ctrl_vctrl == KEY_LCTRL))) {
            keyboard_set_latch_keyarr(kbd_lctrlrow, kbd_lctrlcol, 1);
        } else {
            keyboard_set_latch_keyarr(kbd_lctrlrow, kbd_lctrlcol, 0);
        }
    }
}

/******************************************************************************/

static int keyboard_key_pressed_matrix(int row, int column, int shift)
{
    if ((row < 0) || (column < 0)) {
        return 0;
    }

    /* press SHIFT keys key */
    if (shift & LEFT_SHIFT) {
        left_shift_down = 1;
    }
    if (shift & RIGHT_SHIFT) {
        right_shift_down = 1;
    }
    if (shift & SHIFT_LOCK) {
        keyboard_shiftlock ^= 1;
    }
    /* press CBM key */
    if (lcbm_defined()) {
        if (shift & LEFT_CBM) {
            left_cbm_down = 1;
        }
    }
    /* press CTRL key */
    if (lctrl_defined()) {
        if (shift & LEFT_CTRL) {
            left_ctrl_down = 1;
        }
    }

    keyboard_key_pressed_modifier(row, column, shift);

    DBGMOD(("keyboard_key_pressed_matrix   row:%d col:%d [pressed] shift:0x%04x      vshift:%d lshift:%d rshift:%d shiftlock:%d",
        row, column, (unsigned int)shift,
            virtual_shift_down, left_shift_down, right_shift_down,
            keyboard_shiftlock));

    keyboard_latch_modifier_states();
    return 1;
}

static int keyboard_key_released_matrix(int row, int column, int shift)
{
    int skip_release = 0;

    if ((row < 0) || (column < 0)) {
        return 0;
    }

    if (shift & LEFT_SHIFT) {
        left_shift_down = 0;
        if (keyboard_shiftlock && (key_ctrl_shiftl == KEY_LSHIFT)) {
            skip_release = 1;
        }
    }
    if (shift & RIGHT_SHIFT) {
        right_shift_down = 0;
        if (keyboard_shiftlock && (key_ctrl_shiftl == KEY_RSHIFT)) {
            skip_release = 1;
        }
    }

    /* when shift lock is released and shift lock is "locked", then exit
        early and do nothing */
    if (shift & SHIFT_LOCK) {
        if (keyboard_shiftlock) {
            return 0;
        }
    }

    /* release CBM key */
    if (lcbm_defined()) {
        if (shift & LEFT_CBM) {
            left_cbm_down = 0;
        }
    }
    /* release CTRL key */
    if (lctrl_defined()) {
        if (shift & LEFT_CTRL) {
            left_ctrl_down = 0;
        }
    }

    /* update the virtual modifier flags */
    keyboard_key_released_modifier(row, column, shift);

    DBGMOD(("keyboard_key_released_matrix  row:%d col:%d [release] shift:0x%04x      vshift:%d lshift:%d rshift:%d shiftlock:%d",
            row, column, (unsigned int)shift,
            virtual_shift_down, left_shift_down, right_shift_down,
            keyboard_shiftlock));

    keyboard_latch_modifier_states();

    return !skip_release;
}

static int keyboard_key_matrix_pressed(int row, int column, int shift, int pressed)
{
    DBGMTX(("keyboard_key_matrix_pressed   row:%d col:%d pressed:%d shift:0x%04x",
           row, column, pressed, (unsigned int)shift));
    if (pressed) {
        return keyboard_key_pressed_matrix(row, column, shift);
    }
    return keyboard_key_released_matrix(row, column, shift);
}

/******************************************************************************/

static int keyb_find_in_keyconvtab(int sym, int mod)
{
    int found = -1;
    int i;

    for (i = 0; i < keyconvmap_num_keys; ++i) {
        if (sym == keyconvmap[i].sym) {
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

            found = i;

            /* if the "allow more than one mapping" flag was not found, stop here */
            if (!(keyconvmap[i].shift & ALLOW_OTHER)) {
                break;
            }
        }
    }
    return found;
}

/* press or release a key in the keyboard matrix
 * key: host key
 * mod: host key modifier
 * pressed: press (=1) or release (=0)
 */
static void kbd_key_pressed(signed long key, int mod, int pressed)
{
    int keynum;

    if (keyconvmap == NULL) {
        return;
    }

    keynum = keyb_find_in_keyconvtab((int)key, mod);
    if (keynum == -1) {
        DBGKEY(("kbd_key_pressed [unmapped]  %s key:%3ld pressed:%d",
                pressed ? "D" : "U",
                key, pressed));
    } else {
        DBGKEY(("kbd_key_pressed [mapped]    %s row:%d col:%d pressed:%d shift:0x%04x key:%3ld mod:0x%04x",
                pressed ? "D" : "U",
                keyconvmap[keynum].row, keyconvmap[keynum].column,
                pressed,
                keyconvmap[keynum].shift,
                key, (unsigned)mod
                ));
        if (keyboard_key_matrix_pressed(keyconvmap[keynum].row,
                                        keyconvmap[keynum].column,
                                        keyconvmap[keynum].shift,
                                        pressed)) {
            /* modifier latching is handled in keyboard_latch_modifier_states()
               via keyboard_key_matrix_pressed() */
            if (!key_is_modifier(keyconvmap[keynum].row, keyconvmap[keynum].column)) {
                keyboard_set_latch_keyarr(keyconvmap[keynum].row, keyconvmap[keynum].column, pressed);
            }
            if (network_connected()) {
#if 0
                /* FIXME: does this delay make sense at all? */
                CLOCK delay = KEYBOARD_RAND();
                network_event_record(EVENT_KEYBOARD_DELAY, (void *)&delay, sizeof(delay));
#endif
                network_event_record(EVENT_KEYBOARD_MATRIX, (void *)latch_keyarr, sizeof(latch_keyarr));
            }
        }
    }
}

void keyboard_key_pressed_direct(signed long key, int mod, int pressed)
{
	kbd_key_pressed(key, mod, pressed);
	keyboard_latch_matrix(0);
}

/* keyboard alarm handler, this consumes the host keyboard queue */
static void keyboard_alarm_handler(CLOCK offset, void *data)
{
    int key, mod, pressed;
    int queuepos;

    alarm_unset(keyboard_alarm);
    alarm_context_update_next_pending(keyboard_alarm->context);

    if(kbd_queue_popkey(&key, &mod, &pressed) == 0) {
        DBGKEY(("keyboard_alarm_handler: [no more keys] maincpu_clk: %12lu offset: %12lu",
                maincpu_clk, offset));
        /*kbd_retrigger_alarm();*/
    } else {
        DBGKEY(("keyboard_alarm_handler: [left:%d]       maincpu_clk: %12lu offset: %12lu key:%3d mod:0x%04x pressed:%d",
               1 + ((kbd_queue_read > kbd_queue_write) ? (kbd_queue_read - kbd_queue_write) : (kbd_queue_write - kbd_queue_read)),
               maincpu_clk, offset, key, (unsigned)mod, pressed));
        /* HACK: For each host key event, look back in the keyboard queue if we
           find another key-press event for this host key. if so, unpress the
           earlier host key first. This fixes the situation where we for some
           reason get multiple "key-press" events for the same host key, but
           with different modifiers, and the resulting mapped key is a totally
           different row/col in the emulation. This could get some bits stuck
           in the matrix (tm), which this hack tries to avoid. */
        queuepos = (kbd_queue_read - 1) & (KBD_QUEUE_MAX - 1);
        while (queuepos != kbd_queue_write) {
            queuepos--;
            queuepos &= (KBD_QUEUE_MAX - 1);
            if (queuepos == kbd_queue_write) {
                break;
            }
            /* if there is a key-press event for the same key, then unpress that key+mods */
            if ((kbd_queue[queuepos].pressed == 1) &&
                (kbd_queue[queuepos].key == key)) {
                DBGKEY(("keyboard_alarm_handler: [release older]                          key:%3d mod:0x%04x",
                        key, (unsigned)mod));
                kbd_key_pressed(kbd_queue[queuepos].key, kbd_queue[queuepos].mod & ~mod, 0);
                break;
            }
        }
        DBGKEY(("keyboard_alarm_handler: [%s]                                key:%3d mod:0x%04x",
               pressed ? "pressed" : "release", key, (unsigned)mod));
        /* press or release the new key */
        kbd_key_pressed(key, mod, pressed);

        keyboard_latch_matrix(offset);
        keyboard_event_record();

        kbd_retrigger_alarm();
    }
}

/*-----------------------------------------------------------------------*/

/* press a key, this is called by the UI. this fills the host keyboard queue. */
void keyboard_key_pressed(signed long key, int mod)
{
    int i, j;

    DBGKEY(("keyboard_key_pressed:   [PRESSED]      maincpu_clk: %12lu key:%3ld mod:0x%04x",
            maincpu_clk, key, (unsigned)mod));

    if (event_playback_active()) {
        return;
    }

    /* RESTORE, extra custom keys */
    if (keyboard_custom_key_func_by_keysym((int)key, 1)) {
        return;
    }

    /* keypad at the joystick port */
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
    for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
        if (joystick_port_map[i] == JOYDEV_NUMPAD
            || joystick_port_map[i] == JOYDEV_KEYSET1
            || joystick_port_map[i] == JOYDEV_KEYSET2) {
            if (joystick_check_set(key, joystick_port_map[i] - JOYDEV_NUMPAD, i)) {
                return;
            }
        }
    }
#endif

    /* push key to the keyboard emulation queue */
    if (kbd_queue_pushkey((int)key, mod, 1) != 0) {
        /* generate an alarm at a random time in the future */
        kbd_retrigger_alarm();
    }
}

/* release a key, this is called by the UI. this fills the host keyboard queue. */
void keyboard_key_released(signed long key, int mod)
{
    int i, j;

#ifdef DBGKBD_KEYS
    int idx = keyb_find_in_keyconvtab(key, 0 /* modifier */);
#endif
    DBGKEY(("keyboard_key_released:  [RELEASED]     maincpu_clk: %12lu key:%3ld mod:0x%04x idx:%d",
            maincpu_clk, key, (unsigned)mod, idx));

    if (event_playback_active()) {
        return;
    }

    /* RESTORE, extra custom keys */
    if (keyboard_custom_key_func_by_keysym((int)key, 0)) {
        return;
    }

    /* keypad at the joystick port */
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
    for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
        if (joystick_port_map[i] == JOYDEV_NUMPAD
            || joystick_port_map[i] == JOYDEV_KEYSET1
            || joystick_port_map[i] == JOYDEV_KEYSET2) {
            if (joystick_check_clr(key, joystick_port_map[i] - JOYDEV_NUMPAD, i)) {
                return;
            }
        }
    }
#endif
    /* push key to processing queue */
    if (kbd_queue_pushkey((int)key, mod, 0) != 0) {
        /* generate an alarm at a random time in the future */
        kbd_retrigger_alarm();
    }
}

/*-----------------------------------------------------------------------*/

static void keyboard_key_clear_internal(void)
{
    keyboard_clear_keymatrix();
    clear_virtual_modifier_flags();
    joystick_clear_all();
    clear_modifier_flags();
#ifdef COMMON_JOYKEYS
    joystick_joypad_clear();
#endif
}

/* called by the ui */
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

/* used for event playback, triggering keys from Joystick events, virtual keyboard */
void keyboard_set_keyarr_any(int row, int col, int value)
{
    signed long sym;

    DBGKEY(("keyboard_set_keyarr_any val:%3d row:%d col:%d\n", value, row, col));

    if (row < 0) {
        /* handle special negative row values */
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

/*
    restore key handling. restore key presses are distributed randomly
    across a frame.

    FIXME: when network play is active this is not the case yet
*/

static void restore_alarm_triggered(CLOCK offset, void *data)
{
    static uint32_t event_data;
    alarm_unset(restore_alarm);
    event_data = (uint32_t)restore_delayed;

    DBGKEY(("restore_alarm_triggered pressed:%d maincpu_clk:%12lu", restore_delayed, maincpu_clk));

    machine_set_restore_key(restore_delayed);
    event_record(EVENT_KEYBOARD_RESTORE, (void*)&event_data, sizeof(uint32_t));

    if (restore_delayed && !restore_quick_release) {
        /* we pressed restore */
        restore_delayed = 0;
        /* next alarm will release */
        alarm_set(restore_alarm, kbd_make_event_timestap(maincpu_clk, 0));
    } else {
        /* we released restore */
        if (restore_quick_release) {
            restore_delayed = 0;
            /* next alarm will release (quick) */
            alarm_set(restore_alarm, kbd_make_event_timestap(maincpu_clk, 16));
        }
        restore_quick_release = 0;
    }
}

static int keyboard_restore_pressed(void)
{
    static uint32_t event_data;
    event_data = (uint32_t)1;

    DBGKEY(("keyboard_restore_pressed"));

    if (network_connected()) {
        network_event_record(EVENT_KEYBOARD_RESTORE, (void*)&event_data, sizeof(uint32_t));
    } else {
        if (restore_raw == 0) {
            /* on a 0->1 transition start an alarm */
            restore_delayed = 1;
            restore_quick_release = 0;
            /* next alarm will press */
            alarm_set(restore_alarm, kbd_make_event_timestap(maincpu_clk, 0));
        }
    }
    restore_raw = 1;    /* set = 1 */
    return 1;
}

static int keyboard_restore_released(void)
{
    static uint32_t event_data;
    event_data = (uint32_t)0;

    DBGKEY(("keyboard_restore_released"));

    if (network_connected()) {
        network_event_record(EVENT_KEYBOARD_RESTORE, (void*)&event_data, sizeof(uint32_t));
    } else {
        if (restore_raw == 1) {
            /* on a 1->0 transition start an alarm */
            if (restore_delayed) {
                restore_quick_release = 1;
                restore_delayed = 0;
                /* next alarm will release (quick) */
                alarm_set(restore_alarm, kbd_make_event_timestap(maincpu_clk, 16));
            } else {
                /* next alarm will release */
                alarm_set(restore_alarm, kbd_make_event_timestap(maincpu_clk, 0));
            }
        }
    }
    restore_raw = 0;    /* set = 0 */
    return 0;
}

static int key_restore1_pressed(int pressed)
{
    static int oldpressed = -1;
    DBGKEY(("key_restore1_pressed: %d", pressed));
    if (machine_has_restore_key()) {
        if (pressed != oldpressed) {
            oldpressed = pressed;
            if (pressed) {
                return keyboard_restore_pressed();
            }
            return keyboard_restore_released();
        }
    }
    return pressed;
}

static int key_restore2_pressed(int pressed)
{
    static int oldpressed = -1;
    DBGKEY(("key_restore2_pressed: %d", pressed));
    if (machine_has_restore_key()) {
        if (pressed != oldpressed) {
            oldpressed = pressed;
            if (pressed) {
                return keyboard_restore_pressed();
            }
            return keyboard_restore_released();
        }
    }
    return pressed;
}

/*-----------------------------------------------------------------------*/

/* joyport attached keypad. */
void keyboard_register_joy_keypad(key_joy_keypad_func_t func)
{
    key_joy_keypad_func = func;
}

/*-----------------------------------------------------------------------*/

/* return emulated shift lock state to the UI */
int keyboard_get_shiftlock(void)
{
    return keyboard_shiftlock;
}

/* called by the UI to sync the state of the actual caps lock key with the
   emulated shift lock key. this can be tricky since the host OS and/or keyboard
   might handle "locking" by itself and the state can change when the VICE
   application does not have focus and receives no keyboard events. */
void keyboard_set_shiftlock(int state)
{
    /* only alter the shift lock state when a caps lock key was defined in the
       host keymap. if shift lock is mapped to a regular key we don't have to
       do anything and leave the state alone */
    if (keyconvmap_has_caps_lock) {
        keyboard_shiftlock = state;
        keyboard_latch_modifier_states();
        keyboard_latch_matrix(maincpu_clk);
    }
}

/*****************************************************************************/

static key_custom_info_t key_custom_info[KBD_CUSTOM_NUM] = {
    { "NONE"    , NULL                , KBD_CUSTOM_NONE    , 0, 0, NULL              , NULL                },
    { "RESTORE1", key_restore1_pressed, KBD_CUSTOM_RESTORE1, 0, 0, &key_ctrl_restore1, &key_flags_restore1 },
    { "RESTORE2", key_restore2_pressed, KBD_CUSTOM_RESTORE2, 0, 0, &key_ctrl_restore2, &key_flags_restore2 },
};

/* CAUTION: the registered function MUST NOT call keyboard_custom_key_set() */
void keyboard_register_custom_key(int id, key_custom_func_t func, char *name, int *keysym, int *keyflags)
{
    key_custom_info[id].id = id;
    key_custom_info[id].func = func;
    key_custom_info[id].name = name;
    key_custom_info[id].keysym = keysym;
    key_custom_info[id].keyflags = keyflags;
}

/* called when custom key was pressed or released */
static int keyboard_custom_key_func_by_keysym(int keysym, int pressed)
{
    int n, newstate;
    DBGKEY(("keyboard_custom_key_func_by_keysym %d pressed: %d", keysym, pressed));
    for (n = 0; n < KBD_CUSTOM_NUM; n++) {
        if ((key_custom_info[n].keysym != NULL) &&
            (key_custom_info[n].keyflags != NULL) &&
            (*key_custom_info[n].keysym == keysym) &&
            (key_custom_info[n].func != NULL)) {
            int lockedkey = ((*key_custom_info[n].keyflags & KEYFLG_NO_LOCK) == 0) ? 1 : 0;
            DBGKEY(("keyboard_custom_key_func_by_keysym lockedkey:%d shift:%04x",
                    lockedkey, keyconvmap[n].shift));
            if (lockedkey) {
                /* "toggle" logic, the button is a locked switch */
                newstate = key_custom_info[n].state;
                if (pressed != key_custom_info[n].pressed) {
                    if (pressed) {
                        /* 0->1 */
                        newstate ^= 1;
                    }
                }
            } else {
                /* the key is a regular button */
                newstate = pressed;
            }

            /* remember new pressed state */
            key_custom_info[n].pressed = pressed;

            /* call the custom function only if the state changed */
            if (newstate != key_custom_info[n].state) {
                key_custom_info[n].state = newstate;
                log_message(keyboard_log, "%s %s: now %s",
                            key_custom_info[n].name,
                            pressed ? "down" : " up ",
                            key_custom_info[n].state ? "locked" : "released");
                key_custom_info[n].state = key_custom_info[n].func(key_custom_info[n].state);
            }
            return 1;
        }
    }
    return 0;
}

int keyboard_custom_key_get(int id)
{
    return key_custom_info[id].state;
}

int keyboard_custom_key_set(int id, int state)
{
    if (key_custom_info[id].state != state) {
        if (key_custom_info[id].func != NULL) {
            key_custom_info[id].state = key_custom_info[id].func(state);
        } else {
            key_custom_info[id].state = state;
        }
        return 0;
    }
    return -1;
}

int keyboard_custom_key_toggle(int id)
{
    key_custom_info[id].state = key_custom_info[id].state ? 0 : 1;
    if (key_custom_info[id].func != NULL) {
        key_custom_info[id].state = key_custom_info[id].func(key_custom_info[id].state);
    }
    return 0;
}

/*-----------------------------------------------------------------------*/

void keyboard_alternative_set(int alternative)
{
    key_alternative = alternative;
}


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

/** \brief  Get "KbdStatusbar" directly
 *
 * For UIs: get "KbdStatusbar" resource value without going through the
 * resource system.
 *
 * \return  current value of "KbdStatusbar" resource
 */
int keyboard_statusbar_enabled(void)
{
    return kbd_statusbar_enabled;
}

/*-----------------------------------------------------------------------*/

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

/* alarm_set(keyboard_alarm, maincpu_clk + keyboard_delay); */   /* FIXME */
    keyboard_latch_matrix(maincpu_clk);
}

/*--------------------------------------------------------------------------*/

#define SNAP_MAJOR 1
#define SNAP_MINOR 1
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

/*--------------------------------------------------------------------------*/
static const resource_int_t resources_int[] = {
    { "KbdStatusbar", 0, RES_EVENT_NO, NULL,
      &kbd_statusbar_enabled, keyboard_set_keyboard_statusbar, NULL },
    RESOURCE_INT_LIST_END
};

int keyboard_resources_init(void)
{
    /* VSID doesn't have a keyboard */
    if (machine_class == VICE_MACHINE_VSID) {
        return 0;
    }

    if (resources_register_int(resources_int) < 0) {
        return -1;
    }
    return keymap_resources_init();
}
/*--------------------------------------------------------------------------*/


static cmdline_option_t const cmdline_options[] =
{
    /* enable keyboard debugging display in the statusbar */
    { "-kbdstatusbar", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "KbdStatusbar", (resource_value_t)1,
        NULL, "Enable keyboard-status bar" },

    /* disable keyboard debugging display in the statusbar */
    { "+kbdstatusbar", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "KbdStatusbar", (resource_value_t)0,
        NULL, "Disable keyboard-status bar" },

    CMDLINE_LIST_END
};

int keyboard_cmdline_options_init(void)
{
    if (keymap_cmdline_options_init() < 0) {
        return -1;
    }
    if (machine_class != VICE_MACHINE_VSID) {
        return cmdline_register_options(cmdline_options);
    }
    return 0;
}

/*--------------------------------------------------------------------------*/

void keyboard_init(void)
{
    keyboard_log = log_open("Keyboard");

    keyboard_alarm = alarm_new(maincpu_alarm_context, "Keyboard", keyboard_alarm_handler, NULL);
    restore_alarm = alarm_new(maincpu_alarm_context, "Restore", restore_alarm_triggered, NULL);

    kbd_arch_init();

    keymap_init();
}

void keyboard_shutdown(void)
{
    keymap_shutdown();
}
