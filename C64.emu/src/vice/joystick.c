/*
 * joystick.c - Common joystick emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Based on old code by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  André Fachat <fachat@physik.tu-chemnitz.de>
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

#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "alarm.h"
#include "keyboard.h"
#include "joy.h"
#include "joystick.h"
#include "kbd.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "network.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "uiapi.h"
#include "userport_joystick.h"
#include "vice-event.h"

/* #define DEBUGJOY */

#ifdef DEBUGJOY
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#define JOYSTICK_RAND() (rand() % machine_get_cycles_per_frame())

#define JOYPAD_FIRE 0x10
#define JOYPAD_E    0x08
#define JOYPAD_W    0x04
#define JOYPAD_S    0x02
#define JOYPAD_N    0x01
#define JOYPAD_SW   (JOYPAD_S | JOYPAD_W)
#define JOYPAD_SE   (JOYPAD_S | JOYPAD_E)
#define JOYPAD_NW   (JOYPAD_N | JOYPAD_W)
#define JOYPAD_NE   (JOYPAD_N | JOYPAD_E)

/* Global joystick value.  */
/*! \todo SRT: document: what are these values joystick_value[0, 1, 2, ..., 5] used for? */
BYTE joystick_value[JOYSTICK_NUM + 1] = { 0 };
static BYTE network_joystick_value[JOYSTICK_NUM + 1] = { 0 };

/* Latched joystick status.  */
static BYTE latch_joystick_value[JOYSTICK_NUM + 1] = { 0 };

/* mapping of the joystick ports */
int joystick_port_map[JOYSTICK_NUM] = { 0 };

/* to prevent illegal direction combinations */
static int joystick_opposite_enable = 0;
static const BYTE joystick_opposite_direction[] = {
                                               /* E W S N */
    0,                                         /*         */
    JOYPAD_S,                                  /*       + */
    JOYPAD_N,                                  /*     +   */
    JOYPAD_S | JOYPAD_N,                       /*     + + */
    JOYPAD_E,                                  /*   +     */
    JOYPAD_E | JOYPAD_S,                       /*   +   + */
    JOYPAD_E | JOYPAD_N,                       /*   + +   */
    JOYPAD_E | JOYPAD_S | JOYPAD_N,            /*   + + + */
    JOYPAD_W,                                  /* +       */
    JOYPAD_W | JOYPAD_S,                       /* +     + */
    JOYPAD_W | JOYPAD_N,                       /* +   +   */
    JOYPAD_W | JOYPAD_S | JOYPAD_N,            /* +   + + */
    JOYPAD_E | JOYPAD_W,                       /* + +     */
    JOYPAD_E | JOYPAD_W | JOYPAD_S,            /* + +   + */
    JOYPAD_E | JOYPAD_W | JOYPAD_N,            /* + + +   */
    JOYPAD_E | JOYPAD_W | JOYPAD_S | JOYPAD_N  /* + + + + */
};

/* Callback to machine specific joystick routines, needed for lightpen triggering */
static joystick_machine_func_t joystick_machine_func = NULL;

static alarm_t *joystick_alarm = NULL;

static CLOCK joystick_delay;

#ifdef COMMON_KBD
#define KEYSET_NUM 3
#define KEYSET_NUM_KEYS 9
#define KEYSET_IDX_NUMBLOCK 0
#define KEYSET_IDX_A 1
#define KEYSET_IDX_B 2
static int joykeys[KEYSET_NUM][KEYSET_NUM_KEYS];
#endif

/*! \todo SRT: offset is unused! */

static void joystick_latch_matrix(CLOCK offset)
{
    BYTE idx;

    if (network_connected()) {
        idx = network_joystick_value[0];
        if (idx > 0) {
            joystick_value[idx] = network_joystick_value[idx];
        } else {
            memcpy(joystick_value, network_joystick_value, sizeof(joystick_value));
        }
    } else {
        memcpy(joystick_value, latch_joystick_value, sizeof(joystick_value));
    }

    if (joystick_machine_func != NULL) {
        joystick_machine_func();
    }

    ui_display_joyport(joystick_value);
}

/*-----------------------------------------------------------------------*/

static void joystick_event_record(void)
{
    event_record(EVENT_JOYSTICK_VALUE, (void *)joystick_value, sizeof(joystick_value));
}

void joystick_event_playback(CLOCK offset, void *data)
{
    memcpy(latch_joystick_value, data, sizeof(joystick_value));

    joystick_latch_matrix(offset);
}

static void joystick_latch_handler(CLOCK offset, void *data)
{
    alarm_unset(joystick_alarm);
    alarm_context_update_next_pending(joystick_alarm->context);

    joystick_latch_matrix(offset);

    joystick_event_record();
}

void joystick_event_delayed_playback(void *data)
{
    /*! \todo SRT: why network_joystick_value?
     * and why sizeof latch_joystick_value,
     * if the target is network_joystick_value?
     */
    memcpy(network_joystick_value, data, sizeof(latch_joystick_value));
    alarm_set(joystick_alarm, maincpu_clk + joystick_delay);
}

void joystick_register_machine(joystick_machine_func_t func)
{
    joystick_machine_func = func;
}

void joystick_register_delay(unsigned int delay)
{
    joystick_delay = delay;
}
/*-----------------------------------------------------------------------*/
static void joystick_process_latch(void)
{
    if (network_connected()) {
        CLOCK joystick_delay = JOYSTICK_RAND();
        network_event_record(EVENT_JOYSTICK_DELAY, (void *)&joystick_delay, sizeof(joystick_delay));
        network_event_record(EVENT_JOYSTICK_VALUE, (void *)latch_joystick_value, sizeof(latch_joystick_value));
    } else {
        alarm_set(joystick_alarm, maincpu_clk + JOYSTICK_RAND());
    }
}

void joystick_set_value_absolute(unsigned int joyport, BYTE value)
{
    if (event_playback_active()) {
        return;
    }

    if (latch_joystick_value[joyport] != value) {
        latch_joystick_value[joyport] = value;
        latch_joystick_value[0] = (BYTE)joyport;
        joystick_process_latch();
    }
}

/* set joystick bits */
void joystick_set_value_or(unsigned int joyport, BYTE value)
{
    if (event_playback_active()) {
        return;
    }

    latch_joystick_value[joyport] |= value;

    if (!joystick_opposite_enable) {
        latch_joystick_value[joyport] &= ~joystick_opposite_direction[value & 0xf];
    }

    latch_joystick_value[0] = (BYTE)joyport;
    joystick_process_latch();
}

/* release joystick bits */
void joystick_set_value_and(unsigned int joyport, BYTE value)
{
    if (event_playback_active()) {
        return;
    }

    latch_joystick_value[joyport] &= value;
    latch_joystick_value[0] = (BYTE)joyport;
    joystick_process_latch();
}

void joystick_clear(unsigned int joyport)
{
    latch_joystick_value[joyport] = 0;
    latch_joystick_value[0] = (BYTE)joyport;
    joystick_latch_matrix(0);
}

void joystick_clear_all(void)
{
    memset(latch_joystick_value, 0, sizeof latch_joystick_value);
    joystick_latch_matrix(0);
}

BYTE get_joystick_value(int index)
{
    return joystick_value[index];
}

/*-----------------------------------------------------------------------*/

static int set_joystick_opposite_enable(int val, void *param)
{
    joystick_opposite_enable = val;
    return 0;
}

/*--------------------------------------------------------------------------*/

#ifdef COMMON_KBD

/* the order of values in joypad_bits is the same as in joystick_direction_t */
static int joypad_bits[KEYSET_NUM_KEYS] = {
    JOYPAD_FIRE,
    JOYPAD_SW,
    JOYPAD_S,
    JOYPAD_SE,
    JOYPAD_W,
    JOYPAD_E,
    JOYPAD_NW,
    JOYPAD_N,
    JOYPAD_NE
};

static int joypad_status[KEYSET_NUM][KEYSET_NUM_KEYS];
static int joypad_vmask[KEYSET_NUM];
static int joypad_hmask[KEYSET_NUM];

typedef enum {
    KEYSET_FIRE,
    KEYSET_SW,
    KEYSET_S,
    KEYSET_SE,
    KEYSET_W,
    KEYSET_E,
    KEYSET_NW,
    KEYSET_N,
    KEYSET_NE
} joystick_direction_t;

/* convert the given keyset status array into the corrosponding bits for the
 * joystick
 */
static int getjoyvalue(int *status)
{
    int val = 0;
    int column;

    for (column = 0; column < KEYSET_NUM_KEYS; column++) {
        if (status[column]) {
            val |= joypad_bits[column];
        }
    }
    return val;
}

/* toggle keyset joystick.
   this disables any active key-based joystick and is useful for typing. */
static int joykeys_enable = 0;

static int set_joykeys_enable(int val, void *param)
{
    joykeys_enable = val;
    return 0;
}

#define DEFINE_SET_KEYSET(num)                       \
    static int set_keyset##num(int val, void *param) \
    {                                                \
        joykeys[num][vice_ptr_to_int(param)] = val;  \
                                                     \
        return 0;                                    \
    }

DEFINE_SET_KEYSET(1)
DEFINE_SET_KEYSET(2)

static const resource_int_t resources_int[] = {
    { "KeySet1NorthWest", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_A][KEYSET_NW], set_keyset1, (void *)KEYSET_NW },
    { "KeySet1North", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_A][KEYSET_N], set_keyset1, (void *)KEYSET_N },
    { "KeySet1NorthEast", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_A][KEYSET_NE], set_keyset1, (void *)KEYSET_NE },
    { "KeySet1East", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_A][KEYSET_E], set_keyset1, (void *)KEYSET_E },
    { "KeySet1SouthEast", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_A][KEYSET_SE], set_keyset1, (void *)KEYSET_SE },
    { "KeySet1South", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_A][KEYSET_S], set_keyset1, (void *)KEYSET_S },
    { "KeySet1SouthWest", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_A][KEYSET_SW], set_keyset1, (void *)KEYSET_SW },
    { "KeySet1West", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_A][KEYSET_W], set_keyset1, (void *)KEYSET_W },
    { "KeySet1Fire", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_A][KEYSET_FIRE], set_keyset1, (void *)KEYSET_FIRE },
    { "KeySet2NorthWest", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_B][KEYSET_NW], set_keyset2, (void *)KEYSET_NW },
    { "KeySet2North", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_B][KEYSET_N], set_keyset2, (void *)KEYSET_N },
    { "KeySet2NorthEast", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_B][KEYSET_NE], set_keyset2, (void *)KEYSET_NE },
    { "KeySet2East", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_B][KEYSET_E], set_keyset2, (void *)KEYSET_E },
    { "KeySet2SouthEast", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_B][KEYSET_SE], set_keyset2, (void *)KEYSET_SE },
    { "KeySet2South", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_B][KEYSET_S], set_keyset2, (void *)KEYSET_S },
    { "KeySet2SouthWest", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_B][KEYSET_SW], set_keyset2, (void *)KEYSET_SW },
    { "KeySet2West", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_B][KEYSET_W], set_keyset2, (void *)KEYSET_W },
    { "KeySet2Fire", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[KEYSET_IDX_B][KEYSET_FIRE], set_keyset2, (void *)KEYSET_FIRE },
    { "KeySetEnable", 1, RES_EVENT_NO, NULL,
      &joykeys_enable, set_joykeys_enable, NULL },
    { "JoyOpposite", 0, RES_EVENT_NO, NULL,
      &joystick_opposite_enable, set_joystick_opposite_enable, NULL },
    { NULL }
};

#ifdef DEBUGJOY
static void DBGSTATUS(int keysetnum, int value, int joyport, int key, int flg)
{
    int column;
    char *flags[3] = { "set", "unset", "ignored" };

    DBG((" key:%02x |", key));
    for (column = 0; column < KEYSET_NUM_KEYS; column++) {
        DBG((joypad_status[keysetnum][column] ? "*" : "."));
    }
    DBG(("|"));
    for (column = 5; column >= 0; column--) {
        DBG((((value >> column) & 1) ? "*" : "."));
    }
    DBG(("|"));
    for (column = 5; column >= 0; column--) {
        DBG((((joypad_vmask[keysetnum] >> column) & 1) ? "*" : "."));
    }
    DBG(("|"));
    for (column = 5; column >= 0; column--) {
        DBG((((joypad_hmask[keysetnum] >> column) & 1) ? "*" : "."));
    }
    DBG(("|"));
    for (column = 5; column >= 0; column--) {
        DBG((((latch_joystick_value[joyport] >> column) & 1) ? "*" : "."));
    }
    DBG((" (%s)\n", flags[flg]));
}
#else
#define DBGSTATUS(a, b, c, d, e)
#endif

/* called on key-down event */
int joystick_check_set(signed long key, int keysetnum, unsigned int joyport)
{
    int column, value;

    /* if joykeys are disabled then ignore key sets */
    if (!joykeys_enable) {
        return 0;
    }

    for (column = 0; column < KEYSET_NUM_KEYS; column++) {
        if (key == joykeys[keysetnum][column]) {
            DBG(("joystick_check_set:"));

            joypad_status[keysetnum][column] = 1;
            value = getjoyvalue(joypad_status[keysetnum]);

            if (!joystick_opposite_enable) {
                /* setup the mask for the opposite side of the pressed key */
                if ((column == KEYSET_N) || (column == KEYSET_NW) || (column == KEYSET_NE)) {
                    joypad_vmask[keysetnum] = ~JOYPAD_S;
                } else if ((column == KEYSET_S) || (column == KEYSET_SW) || (column == KEYSET_SE)) {
                    joypad_vmask[keysetnum] = ~JOYPAD_N;
                }
                if ((column == KEYSET_W) || (column == KEYSET_SW) || (column == KEYSET_NW)) {
                    joypad_hmask[keysetnum] = ~JOYPAD_E;
                } else if ((column == KEYSET_E) || (column == KEYSET_SE) || (column == KEYSET_NE)) {
                    joypad_hmask[keysetnum] = ~JOYPAD_W;
                }
                /* if two opposite directions are set, mask out the opposite side of
                 * the last pressed key */
                if ((value & joypad_bits[KEYSET_N]) && (value & joypad_bits[KEYSET_S])) {
                    value &= joypad_vmask[keysetnum];
                }
                if ((value & joypad_bits[KEYSET_E]) && (value & joypad_bits[KEYSET_W])) {
                    value &= joypad_hmask[keysetnum];
                }
            }

            joystick_set_value_absolute(joyport, (BYTE)value);

            DBGSTATUS(keysetnum, value, joyport, key, 0);
            return 1;
        }
    }
    return 0;
}

/* called on key-up event */
int joystick_check_clr(signed long key, int keysetnum, unsigned int joyport)
{
    int column, value;

    /* if joykeys are disabled then ignore key sets */
    if (!joykeys_enable) {
        return 0;
    }

    for (column = 0; column < KEYSET_NUM_KEYS; column++) {
        if (key == joykeys[keysetnum][column]) {
            joypad_status[keysetnum][column] = 0;
            value = getjoyvalue(joypad_status[keysetnum]);

            if (!joystick_opposite_enable) {
                /* if two opposite directions are set, mask out the opposite side of
                 * the last pressed key */
                if ((value & joypad_bits[KEYSET_N]) && (value & joypad_bits[KEYSET_S])) {
                    value &= joypad_vmask[keysetnum];
                }
                if ((value & joypad_bits[KEYSET_E]) && (value & joypad_bits[KEYSET_W])) {
                    value &= joypad_hmask[keysetnum];
                }
            }

            joystick_set_value_absolute(joyport, (BYTE)value);

            DBG(("joystick_check_clr:"));
            DBGSTATUS(keysetnum, value, joyport, key, 1);
            return 1;
        }
    }
    return 0;
}

void joystick_joypad_clear(void)
{
    memset(joypad_status, 0, sizeof(joypad_status));
}

/*-----------------------------------------------------------------------*/

int joystick_init_resources(void)
{
    resources_register_int(resources_int);

    return joystick_arch_init_resources();
}
#else
static const resource_int_t resources_int[] = {
    { "JoyOpposite", 0, RES_EVENT_NO, NULL,
      &joystick_opposite_enable, set_joystick_opposite_enable, NULL },
    { NULL }
};

int joystick_extra_init_resources(void)
{
    resources_register_int(resources_int);

    return 0;
}
#endif /* COMMON_KBD */

int joystick_init(void)
{
    joystick_alarm = alarm_new(maincpu_alarm_context, "Joystick",
                               joystick_latch_handler, NULL);

#ifdef COMMON_KBD
    kbd_initialize_numpad_joykeys(joykeys[0]);
#endif

    return joy_arch_init();
}

/*--------------------------------------------------------------------------*/
int joystick_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, "JOYSTICK", 1, 0);
    if (m == NULL) {
        return -1;
    }

    if (SMW_BA(m, joystick_value, (JOYSTICK_NUM + 1)) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (snapshot_module_close(m) < 0) {
        return -1;
    }

    return 0;
}

int joystick_snapshot_read_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, "JOYSTICK",
                             &major_version, &minor_version);
    if (m == NULL) {
        return 0;
    }

    if (SMR_BA(m, joystick_value, (JOYSTICK_NUM + 1)) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}
