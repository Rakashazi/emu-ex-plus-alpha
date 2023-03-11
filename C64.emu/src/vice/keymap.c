/** \file   keymap.c
 * \brief   Keymap handling
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
/* #define DBGKBD_MODIFIERS */

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
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#ifdef DBGKBD_MODIFIERS
#define DBGMOD(x)  printf x
#else
#define DBGMOD(x)
#endif

static log_t keyboard_log = LOG_DEFAULT;

keyboard_conv_t *keyconvmap = NULL; /* FIXME: getter */

/* Memory size of array in sizeof(keyconv_t), 0 = static.  */
static int keyc_mem = 0;

/* FIXME: all of the following should probably not be simply globals, but
          go into a struct of some sort */

/* Number of convs used in sizeof(keyconv_t).  */
int keyconvmap_num_keys = 0;

/* flag that indicates if a key with SHIFT_LOCK flag exists in the keymap */
int keyconvmap_has_caps_lock = 0;

/* matrix locations for the modifier keys */
int kbd_lshiftrow = -1;
int kbd_lshiftcol = -1;
int kbd_rshiftrow = -1;
int kbd_rshiftcol = -1;
int kbd_lcbmrow   = -1;
int kbd_lcbmcol   = -1;
int kbd_lctrlrow  = -1;
int kbd_lctrlcol  = -1;

/* host keycodes for the modifiers */
int key_ctrl_vshift = KEY_NONE;   /* virtual shift */
int key_ctrl_vcbm   = KEY_NONE;   /* virtual cbm */
int key_ctrl_vctrl  = KEY_NONE;   /* virtual ctrl */
int key_ctrl_shiftl = KEY_NONE;   /* shift-lock */

/* Two possible restore keys.  */
int key_ctrl_restore1 = -1;
int key_flags_restore1 = KEYFLG_NO_LOCK;
int key_ctrl_restore2 = -1;
int key_flags_restore2 = KEYFLG_NO_LOCK;

/* 40/80 column key.  */
int key_ctrl_column4080 = -1;
int key_flags_column4080 = 0;   /* default is locked! */
/* CAPS (ASCII/DIN) key.  */
int key_ctrl_caps = -1;
int key_flags_caps = 0;   /* default is locked! */

/*-----------------------------------------------------------------------*/

/* Is the resource code ready to load the keymap?  */
static int load_keymap_ok = 0;

static int machine_keyboard_mapping = 0;
static int machine_keyboard_type = 0;

static int try_set_keymap_file(int atidx, int idx, int mapping, int type);

#define KBD_SWITCH_DEFAULT      0
#define KBD_SWITCH_MAPPING      1
#define KBD_SWITCH_INDEX        2
#define KBD_SWITCH_TYPE         3
static int switch_keymap_file(int sw, int *idxp, int *mapp, int *typep);

static int keyboard_parse_keymap(const char *filename, int child);

/*-----------------------------------------------------------------------*/

static void keyboard_keyconvmap_alloc(void)
{
#define KEYCONVMAP_SIZE_MIN 150

    keyconvmap = lib_malloc(KEYCONVMAP_SIZE_MIN * sizeof(keyboard_conv_t));
    keyconvmap_num_keys = 0;
    keyc_mem = KEYCONVMAP_SIZE_MIN - 1;
    keyconvmap[0].sym = ARCHDEP_KEYBOARD_SYM_NONE;
    keyconvmap_has_caps_lock = 0;
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
    key_ctrl_vshift = ret;
    return 0;
}

static int keyboard_keyword_shiftl(void)
{
    int ret = keyboard_keyword_vshiftl();
    if (ret < 0) {
        return -1;
    }
    key_ctrl_shiftl = ret;
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
    key_ctrl_vcbm = ret;
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
    key_ctrl_vctrl = ret;
    return 0;
}

static void keyboard_keyword_clear(void)
{
    int i, j;

    keyconvmap_num_keys = 0;
    keyconvmap[0].sym = ARCHDEP_KEYBOARD_SYM_NONE;

    key_ctrl_restore1 = -1;
    key_ctrl_restore2 = -1;
    key_ctrl_caps = -1;
    key_ctrl_column4080 = -1;

    key_flags_restore1 = KEYFLG_NO_LOCK;
    key_flags_restore2 = KEYFLG_NO_LOCK;
    key_flags_caps = 0;
    key_flags_column4080 = 0;

    key_ctrl_shiftl = KEY_NONE;
    key_ctrl_vshift = KEY_NONE;
    key_ctrl_vcbm = KEY_NONE;
    key_ctrl_vctrl = KEY_NONE;

    kbd_lshiftrow = -1;
    kbd_lshiftcol = -1;
    kbd_rshiftrow = -1;
    kbd_rshiftcol = -1;
    kbd_lcbmrow   = -1;
    kbd_lcbmcol   = -1;
    kbd_lctrlrow  = -1;
    kbd_lctrlcol  = -1;
    keyconvmap_has_caps_lock = 0;

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
        for (i = 0; i < keyconvmap_num_keys; i++) {
            if (keyconvmap[i].sym == sym) {
                if (keyconvmap_num_keys) {
                    keyconvmap[i] = keyconvmap[--keyconvmap_num_keys];
                }
                keyconvmap[keyconvmap_num_keys].sym = ARCHDEP_KEYBOARD_SYM_NONE;
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

static int keyboard_parse_set_pos_row(signed long sym, int row, int col,
                                       int shift)
{
    int i;

    /* FIXME: we should check against the actual size of the emulated
              keyboard here */
    if ((row >= KBD_ROWS) || (col >= KBD_COLS)) {
        return -1;
    }

    for (i = 0; i < keyconvmap_num_keys; i++) {
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
    if (i >= keyconvmap_num_keys) {
        /* Table too small -> realloc.  */
        if (keyconvmap_num_keys >= keyc_mem) {
            keyboard_keyconvmap_realloc();
        }

        if (keyconvmap_num_keys < keyc_mem) {
            keyconvmap[keyconvmap_num_keys].sym = sym;
            keyconvmap[keyconvmap_num_keys].row = row;
            keyconvmap[keyconvmap_num_keys].column = col;
            keyconvmap[keyconvmap_num_keys].shift = shift;
            keyconvmap[++keyconvmap_num_keys].sym = ARCHDEP_KEYBOARD_SYM_NONE;
        }
    }
    return 0;
}

static int keyboard_parse_set_neg_row(signed long sym, int row, int col, int shift)
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
        key_ctrl_restore1 = (int)sym;
        key_flags_restore1 = shift | KEYFLG_NO_LOCK;
    } else if ((row == KBD_ROW_RESTORE_2) && (col == KBD_COL_RESTORE_2)) {
        key_ctrl_restore2 = (int)sym;
        key_flags_restore2 = shift | KEYFLG_NO_LOCK;
    } else if ((row == KBD_ROW_4080COLUMN) && (col == KBD_COL_4080COLUMN)) {
        key_ctrl_column4080 = (int)sym;
        key_flags_column4080 = shift;
    } else if ((row == KBD_ROW_CAPSLOCK) && (col == KBD_COL_CAPSLOCK)) {
        key_ctrl_caps = (int)sym;
        key_flags_caps = shift;
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
        row = strtol(p, NULL, 0);
        p = strtok(NULL, " \t,");
        if (p != NULL) {
            col = (int)strtol(p, NULL, 0);
            p = strtok(NULL, " \t");
            if (p != NULL || row < 0) {
                if (p != NULL) {
                    shift = (int)strtol(p, NULL, 0);
                }

                if (row >= 0) {
                    if (keyboard_parse_set_pos_row(sym, (int)row, col, shift) < 0) {
                        log_error(keyboard_log,
                                  "%s:%d: Bad row/column value (%ld/%d) for keysym `%s'.",
                                  filename, line, row, col, key);
                    }
                } else {
                    if (keyboard_parse_set_neg_row(sym, (int)row, col, shift) < 0) {
                        log_error(keyboard_log,
                                  "%s:%d: Bad row/column value (%ld/%d) for keysym `%s'.",
                                  filename, line, row, col, key);
                    }
                }

                /* printf("%s:%d: %s %d %d (%04x)\n", filename, line, key, row, col, shift); */

                if (shift & SHIFT_LOCK) {
                    keyconvmap_has_caps_lock = 1;
                }

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
                        if (key_ctrl_shiftl == KEY_RSHIFT) {
                            if ((row != kbd_rshiftrow) || (col != kbd_rshiftcol)) {
                                log_warning(keyboard_log, "%s:%d: SHIFT-lock flag used but row and/or col differs from !RSHIFT definition", filename, line);
                            }
                        } else if (key_ctrl_shiftl == KEY_LSHIFT) {
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
                        if (key_ctrl_shiftl == KEY_RSHIFT) {
                            if ((row == kbd_rshiftrow) && (col == kbd_rshiftcol)) {
                                if ((!(shift & SHIFT_LOCK)) && (!(shift & (RIGHT_SHIFT | LEFT_SHIFT)))) {
                                    log_warning(keyboard_log, "%s:%d: !SHIFTL defined but key does not use SHIFT-lock flag", filename, line);
                                }
                            }
                        } else if (key_ctrl_shiftl == KEY_LSHIFT) {
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
    static const char * const ms[8] = {
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
    fp = sysfile_open(filename, machine_name, &complete_path, "rb");

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
        keyboard_parse_set_neg_row(sym, row, col, shift);
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
            "# - normal lines have 'keysym/scancode row column shiftflag'\n"
            "#\n"
            "# Keywords and their lines are:\n"
            "# '!CLEAR'               clear whole table\n"
            "# '!INCLUDE filename'    read file as mapping file\n"
            "# '!LSHIFT row col'      left shift keyboard row/column\n"
            "# '!RSHIFT row col'      right shift keyboard row/column\n"
            "# '!VSHIFT shiftkey'     virtual shift key (RSHIFT or LSHIFT)\n"
            "# '!SHIFTL shiftkey'     shift lock key (RSHIFT or LSHIFT)\n"
            "#  for emulated keyboards that have only one shift key, set both LSHIFT\n"
            "#  and RSHIFT to the same row/col and use RSHIFT for VSHIFT and SHIFTL.\n"
            "# '!LCTRL row col'       left control keyboard row/column\n"
            "# '!VCTRL ctrlkey'       virtual control key (LCTRL)\n"
            "# '!LCBM row col'        left CBM keyboard row/column\n"
            "# '!VCBM cbmkey'         virtual CBM key (LCBM)\n"
            "# '!UNDEF keysym'        remove keysym from table\n"
            "#\n"
            "# Shiftflag can have these values, flags can be ORed to combine them:\n"
            "# 0x0000      0  key is not shifted for this keysym/scancode\n"
            "# 0x0001      1  key is combined with shift for this keysym/scancode\n"
            "# 0x0002      2  key is left shift on emulated machine\n"
            "# 0x0004      4  key is right shift on emulated machine (use only this one\n"
            "#                for emulated keyboards that have only one shift key)\n"
            "# 0x0008      8  key can be shifted or not with this keysym/scancode\n"
            "# 0x0010     16  deshift key for this keysym/scancode\n"
            "# 0x0020     32  another definition for this keysym/scancode follows\n"
            "# 0x0040     64  key is shift-lock on emulated machine\n"
            "# 0x0080    128  shift modifier required on host\n"
            "# 0x0100    256  key is used for an alternative keyboard mapping, e.g. C64 mode in x128\n"
            "# 0x0200    512  alt-r (alt-gr) modifier required on host\n"
            "# 0x0400   1024  ctrl modifier required on host\n"
            "# 0x0800   2048  key is combined with cbm for this keysym/scancode\n"
            "# 0x1000   4096  key is combined with ctrl for this keysym/scancode\n"
            "# 0x2000   8192  key is (left) cbm on emulated machine\n"
            "# 0x4000  16384  key is (left) ctrl on emulated machine\n"
            "# 0x8000  32768  do NOT emulate toggle switch for this key\n"
            "#\n"
            "# Negative row values:\n"
            "# 'keysym -1 n' joystick keymap A, direction n\n"
            "# 'keysym -2 n' joystick keymap B, direction n\n"
            "# 'keysym -3 0' first RESTORE key\n"
            "# 'keysym -3 1' second RESTORE key\n"
            "# 'keysym -4 0 <flags>' 40/80 column key (x128)\n"
            "# 'keysym -4 1 <flags>' CAPS (ASCII/DIN) key (x128)\n"
            "# 'keysym -5 n' joyport keypad, key n (not supported in x128)\n"
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
        fprintf(fp, "!VSHIFT %s\n", (key_ctrl_vshift == KEY_RSHIFT) ? "RSHIFT" : "LSHIFT");
    }
    if (shiftlock_defined()) {
        fprintf(fp, "!SHIFTL %s\n", (key_ctrl_shiftl == KEY_RSHIFT) ? "RSHIFT" : "LSHIFT");
    }
    if (lctrl_defined()) {
        fprintf(fp, "!LCTRL %d %d\n", kbd_lctrlrow, kbd_lctrlcol);
    }
    if (vctrl_defined()) {
        fprintf(fp, "!VCTRL %s\n", (key_ctrl_vctrl == KEY_LCTRL) ? "LCTRL" : "?");
    }
    if (lcbm_defined()) {
        fprintf(fp, "!LCBM %d %d\n", kbd_lcbmrow, kbd_lcbmcol);
    }
    if (vcbm_defined()) {
        fprintf(fp, "!VCBM %s\n", (key_ctrl_vcbm == KEY_LCBM) ? "LCBM" : "?");
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
        fprintf(fp, "%s -4 0 0x%04x\n",
                kbd_arch_keynum_to_keyname(key_ctrl_column4080), (unsigned)key_flags_column4080);
        fprintf(fp, "\n");
    }

    if (key_ctrl_caps != -1) {
        fprintf(fp, "#\n"
                "# CAPS (ASCII/DIN) key mapping\n"
                "#\n");
        fprintf(fp, "%s -4 1 0x%04x\n",
                kbd_arch_keynum_to_keyname(key_ctrl_caps), (unsigned)key_flags_caps);
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

/*****************************************************************************/

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

/* CAUTION: keep in sync with constants in keyboard.h and code in
            arch/shared/archdep_kbd_get_host_mapping.c and also with the
            description of the KeyboardMapping resource in vice.texi */
static mapping_info_t kbdinfo[KBD_MAPPING_NUM + 1] = {
    { "American (us)", KBD_MAPPING_US, "" },    /* this must be first (=0) always */
    { "Belgian (dutch) (be)", KBD_MAPPING_BE, "be" },
    { "British (uk)", KBD_MAPPING_UK, "uk" },
    { "Danish (da)", KBD_MAPPING_DA, "da" },
    { "Dutch (nl)", KBD_MAPPING_NL, "nl" },
    { "Finnish (fi)", KBD_MAPPING_FI, "fi" },
    { "French (fr)", KBD_MAPPING_FR, "fr" },
    { "German (de)", KBD_MAPPING_DE, "de" },
    { "Italian (it)", KBD_MAPPING_IT, "it" },
    { "Norwegian (no)", KBD_MAPPING_NO, "no" },
    { "Spanish (es)", KBD_MAPPING_ES, "es" },
    { "Swedish (se)", KBD_MAPPING_SE, "se" },
    { "Swiss (ch)", KBD_MAPPING_CH, "ch" },
    { "Turkish (tr)", KBD_MAPPING_TR, "tr" },
    { NULL, 0, 0 }
};

mapping_info_t *keyboard_get_info_list(void)
{
    return &kbdinfo[0];
}

static char *keyboard_get_mapping_name(int mapping)
{
    int n;
    for (n = 0; n < KBD_MAPPING_NUM; n++) {
        if (kbdinfo[n].mapping == mapping) {
            return kbdinfo[n].mapping_name;
        }
    }
    return kbdinfo[0].mapping_name;
}

static char *keyboard_get_keymap_name(int idx, int mapping, int type)
{
    static const char * const sympos[2] = { "sym", "pos"};
    const char *mapname;
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
    res = sysfile_locate(name, machine_name, &complete_path);

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
    if (sysfile_locate(name, machine_name, &complete_path) != 0) {
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
    /* CAUTION: first mapping, then index. (Host) Mapping can always be changed
       and may block/adjust the index depending on available keymaps */
    { "KeyboardMapping", 0, RES_EVENT_NO, NULL,
      &machine_keyboard_mapping, keyboard_set_keyboard_mapping, NULL },
    { "KeymapIndex", KBD_INDEX_SYM, RES_EVENT_NO, NULL,
      &machine_keymap_index, keyboard_set_keymap_index, NULL },
    { "KeyboardType", 0, RES_EVENT_NO, NULL,
      &machine_keyboard_type, keyboard_set_keyboard_type, NULL },
    RESOURCE_INT_LIST_END
};

/*--------------------------------------------------------------------------*/

int keymap_resources_init(void)
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
    CMDLINE_LIST_END
};

int keymap_cmdline_options_init(void)
{
    if (machine_class != VICE_MACHINE_VSID) {
        return cmdline_register_options(cmdline_options);
    }
    return 0;
}

/*--------------------------------------------------------------------------*/

void keymap_init(void)
{
    if (machine_class != VICE_MACHINE_VSID) {
        load_keymap_ok = 1;
        keyboard_set_keymap_index(machine_keymap_index, NULL);
    }
}

void keymap_shutdown(void)
{
    keyboard_keyconvmap_free();
    keyboard_resources_shutdown();      /* FIXME: perhaps call from elsewhere? */
}

