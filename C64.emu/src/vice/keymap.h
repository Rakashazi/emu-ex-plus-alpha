/*
 * keymap.h - Keymap handling
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

#ifndef VICE_KEYMAP_H
#define VICE_KEYMAP_H

/* index to select the current keymap ("KeymapIndex") */
#define KBD_INDEX_SYM     0
#define KBD_INDEX_POS     1
#define KBD_INDEX_USERSYM 2
#define KBD_INDEX_USERPOS 3
#define KBD_INDEX_LAST    3
#define KBD_INDEX_NUM     4

/* the mapping of the host ("KeyboardMapping")

   CAUTION: keep in sync with code in keyboard.c and code in
            arch/shared/archdep_kbd_get_host_mapping.c and also with the
            description of the KeyboardMapping resource in vice.texi

   CAUTION: append new mappings to the end of the list, do not change the
            existing IDs, since these are referenced by number in config files
            and cmdline options.
*/
#define KBD_MAPPING_US    0     /* "" (us mapping) this must be first (=0) always */
#define KBD_MAPPING_UK    1     /* "uk" */
#define KBD_MAPPING_DA    2     /* "da" */
#define KBD_MAPPING_NL    3     /* "nl" */
#define KBD_MAPPING_FI    4     /* "fi" */
#define KBD_MAPPING_FR    5     /* "fr" */
#define KBD_MAPPING_DE    6     /* "de" */
#define KBD_MAPPING_IT    7     /* "it" */
#define KBD_MAPPING_NO    8     /* "no" */
#define KBD_MAPPING_ES    9     /* "es" */
#define KBD_MAPPING_SE    10    /* "se" */
#define KBD_MAPPING_CH    11    /* "ch" */
#define KBD_MAPPING_BE    12    /* "be" */
#define KBD_MAPPING_TR    13    /* "tr" */
#define KBD_MAPPING_LAST  13
#define KBD_MAPPING_NUM   14

int keyboard_get_num_mappings(void);

/* mapping info for GUIs */
typedef struct {
    char *name;
    int mapping;
    char *mapping_name;
} mapping_info_t;

mapping_info_t *keyboard_get_info_list(void);
int keyboard_is_keymap_valid(int sympos, int hosttype, int kbdtype);
int keyboard_is_hosttype_valid(int hosttype);

/* FIXME: rename the members of the enum, use some common prefix */
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
    LEFT_CTRL        = (1 << 14), /* Key is CTRL on the emulated machine */

    KEYFLG_NO_LOCK   = (1 << 15), /* do NOT emulate a "locked" key for this key */

    /* used in keyboard emulation code, not in keymaps */
    IS_PRESSED       = (1 << 30)  /* is pressed */
};

struct keyboard_conv_s {
    signed long sym;
    int row;
    int column;
    enum shift_type shift;
    char *comment;
};
typedef struct keyboard_conv_s keyboard_conv_t;

extern keyboard_conv_t *keyconvmap;

/*****************************************************************************/
/* FIXME: all of the following should probably not be simply globals, but
          go into a struct of some sort */

/* number of keys in the keymap */
extern int keyconvmap_num_keys;
extern int keyconvmap_has_caps_lock;

/* matrix locations for the modifier keys */
extern int kbd_lshiftrow;
extern int kbd_lshiftcol;
extern int kbd_rshiftrow;
extern int kbd_rshiftcol;
extern int kbd_lcbmrow;
extern int kbd_lcbmcol;
extern int kbd_lctrlrow;
extern int kbd_lctrlcol;

/* where the modifiers map to */
#define KEY_NONE   0
#define KEY_RSHIFT 1
#define KEY_LSHIFT 2
#define KEY_LCBM   3
#define KEY_LCTRL  4

extern int key_ctrl_vshift;   /* virtual shift */
extern int key_ctrl_vcbm;   /* virtual cbm */
extern int key_ctrl_vctrl;   /* virtual ctrl */
extern int key_ctrl_shiftl;   /* shift-lock */

/* Two possible restore keys.  */
extern int key_ctrl_restore1;
extern int key_flags_restore1;
extern int key_ctrl_restore2;
extern int key_flags_restore2;

/* 40/80 column key.  */
extern int key_ctrl_column4080;
extern int key_flags_column4080;
/* CAPS (ASCII/DIN) key.  */
extern int key_ctrl_caps;
extern int key_flags_caps;

/* joyport attached keypad. */
extern signed long key_joy_keypad[KBD_JOY_KEYPAD_ROWS][KBD_JOY_KEYPAD_COLS]; /*FIXME*/

/*****************************************************************************/

void keymap_init(void);
void keymap_shutdown(void);

int keymap_cmdline_options_init(void);
int keymap_resources_init(void);

extern int keyboard_set_keymap_index(int vak, void *param);
int keyboard_set_keymap_file(const char *val, void *param);
int keyboard_keymap_dump(const char *filename);

void keyboard_set_map_any(signed long sym, int row, int col, int shift);
void keyboard_set_unmap_any(signed long sym);

/*****************************************************************************/

/* some inline functions */

/* FIXME: better names? */

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
    return !(key_ctrl_vshift == KEY_NONE);
}

static inline int vctrl_defined(void) {
    return !(key_ctrl_vctrl == KEY_NONE);
}

static inline int vcbm_defined(void) {
    return !(key_ctrl_vcbm == KEY_NONE);
}

static inline int shiftlock_defined(void) {
    return !(key_ctrl_shiftl == KEY_NONE);
}

static inline int key_is_modifier(int row, int column) {
    if ((rshift_defined() && (row == kbd_rshiftrow) && (column == kbd_rshiftcol)) ||
        (lshift_defined() && (row == kbd_lshiftrow) && (column == kbd_lshiftcol)) ||
        (lcbm_defined() && (row == kbd_lcbmrow) && (column == kbd_lcbmcol)) ||
        (lctrl_defined() && (row == kbd_lctrlrow) && (column == kbd_lctrlcol))) {
        return 1;
    }
    return 0;
}

#endif
