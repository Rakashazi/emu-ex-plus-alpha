/*
 * joystick.c - Common joystick emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "alarm.h"
#include "cmdline.h"
#include "keyboard.h"
#include "joyport.h"
#include "joystick.h"
#include "kbd.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "network.h"
#include "resources.h"
#include "snapshot.h"
#include "sysfile.h"
#include "types.h"
#include "uiapi.h"
#include "userport_joystick.h"
#include "util.h"
#include "vice-event.h"

/* Control port <--> Joystick connections:

   cport | joystick | I/O
   ----------------------
     1   | up       |  I
     2   | down     |  I
     3   | left     |  I
     4   | right    |  I
     6   | button   |  I
         |          |
     9   | button 2 |  I
     5   | button 3 |  I

   Directions and fire button 1 works on all joystick ports and joystick adapters
   Button 2 and 3 works on:
   - Native joystick port(s) (x64/x64sc/xscpu64/x128/xcbm5x0/xvic)
   - sidcart joystick adapter port (xplus4)

 */

/* #define DEBUGJOY */

#ifdef DEBUGJOY
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#define JOYPAD_FIRE2 0x20
#define JOYPAD_FIRE3 0x40
#define JOYPAD_FIRE4 0x80
#define JOYPAD_FIRE5 0x100
#define JOYPAD_FIRE6 0x200
#define JOYPAD_FIRE7 0x400
#define JOYPAD_FIRE8 0x800
#define JOYPAD_FIRE 0x10
#define JOYPAD_E    0x08
#define JOYPAD_W    0x04
#define JOYPAD_S    0x02
#define JOYPAD_N    0x01
#define JOYPAD_SW   (JOYPAD_S | JOYPAD_W)
#define JOYPAD_SE   (JOYPAD_S | JOYPAD_E)
#define JOYPAD_NW   (JOYPAD_N | JOYPAD_W)
#define JOYPAD_NE   (JOYPAD_N | JOYPAD_E)

static int joyport_joystick[JOYPORT_MAX_PORTS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* Global joystick value.  */
/*! \todo SRT: document: what are these values joystick_value[0, 1, 2, ..., 5] used for? */
VICE_API uint16_t joystick_value[JOYPORT_MAX_PORTS] = { 0 };

typedef struct joystick_values_s {
    unsigned int last_used_joyport;
    uint16_t values[JOYPORT_MAX_PORTS];
} joystick_values_t;

static joystick_values_t network_joystick_value = { .last_used_joyport = JOYPORT_MAX_PORTS };

/* Latched joystick status.  */
static joystick_values_t latch_joystick_value = { .last_used_joyport = JOYPORT_MAX_PORTS };

/* mapping of the joystick ports */
int joystick_port_map[JOYPORT_MAX_PORTS] = { 0 };

/* to prevent illegal direction combinations */
static int joystick_opposite_enable = 0;
static const uint16_t joystick_opposite_direction[] = {
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

#ifdef COMMON_JOYKEYS
int joykeys[JOYSTICK_KEYSET_NUM][JOYSTICK_KEYSET_NUM_KEYS];
#endif

static int joystick_autofire_enable[JOYPORT_MAX_PORTS] = {0};
static int joystick_autofire_mode[JOYPORT_MAX_PORTS] = {0};
static int joystick_autofire_speed[JOYPORT_MAX_PORTS] = {0};

static uint8_t joystick_axis_value[2][2] = { {0x80, 0x80}, {0x80, 0x80} };

/*! \todo SRT: offset is unused! */

/* Joystick mapping filename */
static char *joymap_file = NULL;

/** \brief  Temporary copy of the default joymap file name
 *
 * Avoids silly casting away of const
 */
static char *joymap_factory = NULL;


static log_t joy_log = LOG_ERR;

static void joystick_latch_matrix(CLOCK offset)
{
    uint8_t idx;
    int port;

    if (network_connected()) {
        idx = network_joystick_value.last_used_joyport;
        if (idx < JOYPORT_MAX_PORTS) {
            joystick_value[idx] = network_joystick_value.values[idx];
        } else {
            memcpy(joystick_value, network_joystick_value.values, sizeof(joystick_value));
        }
    } else {
        memcpy(joystick_value, latch_joystick_value.values, sizeof(joystick_value));
    }

    if (joystick_machine_func != NULL) {
        joystick_machine_func();
    }

    for (port = 0; port < JOYPORT_MAX_PORTS; port++) {
        if (joyport_joystick[port]) {
            joyport_display_joyport(port, JOYPORT_ID_JOYSTICK, joystick_value[port]);
        }
    }
}

/*-----------------------------------------------------------------------*/

static void joystick_event_record(void)
{
    event_record(EVENT_JOYSTICK_VALUE, (void *)joystick_value, sizeof(joystick_value));
}

void joystick_event_playback(CLOCK offset, void *data)
{
    memcpy(latch_joystick_value.values, data, sizeof(latch_joystick_value.values));

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
    memcpy(&network_joystick_value, data, sizeof(latch_joystick_value));
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

static int joystick_hook[JOYPORT_MAX_PORTS] = {0};
static uint16_t joystick_hook_mask[JOYPORT_MAX_PORTS] = {0};
static uint16_t joystick_hook_state[JOYPORT_MAX_PORTS] = {0};

void joystick_set_hook(int port, int val, uint16_t mask)
{
    joystick_hook[port] = val;
    joystick_hook_mask[port] = mask;
}

static void joystick_handle_hooks(unsigned int joyport)
{
    uint16_t masked_new;
    uint16_t masked_old;

    if (joystick_hook[joyport]) {
        masked_old = joystick_hook_state[joyport] & joystick_hook_mask[joyport];
        masked_new = latch_joystick_value.values[joyport] & joystick_hook_mask[joyport];
        if (masked_old != masked_new) {
            joyport_handle_joystick_hook(joyport, masked_new);
            joystick_hook_state[joyport] = masked_new;
        }
    }
}

static void joystick_process_latch(void)
{
    CLOCK delay = lib_unsigned_rand(1, (unsigned int)machine_get_cycles_per_frame());

    if (network_connected()) {
        network_event_record(EVENT_JOYSTICK_DELAY, (void *)&delay, sizeof(delay));
        network_event_record(EVENT_JOYSTICK_VALUE, (void *)&latch_joystick_value, sizeof(latch_joystick_value));
    } else {
        alarm_set(joystick_alarm, maincpu_clk + delay);
    }
}

uint8_t joystick_get_axis_value(unsigned int port, unsigned int pot)
{
    return joystick_axis_value[port][pot];
}

void joystick_set_value_absolute(unsigned int joyport, uint16_t value)
{
    if (event_playback_active()) {
        return;
    }

    if (latch_joystick_value.values[joyport] != value) {
        latch_joystick_value.values[joyport] = value;
        latch_joystick_value.last_used_joyport = joyport;
        joystick_process_latch();
        joystick_handle_hooks(joyport);
    }
}

/* set joystick bits */
void joystick_set_value_or(unsigned int joyport, uint16_t value)
{
    if (event_playback_active()) {
        return;
    }

    latch_joystick_value.values[joyport] |= value;

    if (!joystick_opposite_enable) {
        latch_joystick_value.values[joyport] &= (uint16_t)(~joystick_opposite_direction[value & 0xf]);
    }

    latch_joystick_value.last_used_joyport = joyport;
    joystick_process_latch();
    joystick_handle_hooks(joyport);
}

/* release joystick bits */
void joystick_set_value_and(unsigned int joyport, uint16_t value)
{
    if (event_playback_active()) {
        return;
    }

    latch_joystick_value.values[joyport] &= value;
    latch_joystick_value.last_used_joyport = joyport;
    joystick_process_latch();
    joystick_handle_hooks(joyport);
}

void joystick_clear(unsigned int joyport)
{
    latch_joystick_value.values[joyport] = 0;
    latch_joystick_value.last_used_joyport = joyport;
    joystick_latch_matrix(0);
    joystick_handle_hooks(joyport);
}

void joystick_clear_all(void)
{
    int i;

    memset(latch_joystick_value.values, 0, sizeof latch_joystick_value.values);
    latch_joystick_value.last_used_joyport = JOYPORT_MAX_PORTS;
    joystick_latch_matrix(0);
    for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
        joystick_handle_hooks(i);
    }
}

static int get_joystick_autofire(int index)
{
    uint32_t second_cycles = (uint32_t)(maincpu_clk % machine_get_cycles_per_second());
    uint32_t cycles_per_flip = (uint32_t)(machine_get_cycles_per_second() / (joystick_autofire_speed[index] * 2));
    uint32_t flip_part = second_cycles / cycles_per_flip;

    if (flip_part & 1) {
        return 0;
    }
    return 1;
}

uint16_t get_joystick_value(int index)
{
    uint16_t retval = joystick_value[index] & 0xffef;
    int fire_button = JOYPORT_BIT_BOOL(joystick_value[index], JOYPORT_FIRE_BIT);

    /* check if autofire is enabled */
    if (joystick_autofire_enable[index]) {
        /* check if autofire mode is permanent, as in autofire when fire is not pressed */
        if (joystick_autofire_mode[index] == JOYSTICK_AUTOFIRE_MODE_PERMANENT) {
            /* check if fire button is not pressed */
            if (!fire_button) {
                /* get fire button from autofire code */
                fire_button = get_joystick_autofire(index);
            }
        } else {
            /* check if fire button is pressed */
            if (fire_button) {
                /* get fire button from autofire code */
                fire_button = get_joystick_autofire(index);
            }
        }
    }

    /* put fire button into the return value */
    retval |= (fire_button ? JOYPORT_FIRE : 0);

    return retval;
}

/*--------------------------------------------------------------------------*/

#ifdef COMMON_JOYKEYS

/* the order of values in joypad_bits is the same as in joystick_direction_t */
static const int joypad_bits[JOYSTICK_KEYSET_NUM_KEYS] = {
    JOYPAD_FIRE,
    JOYPAD_SW,
    JOYPAD_S,
    JOYPAD_SE,
    JOYPAD_W,
    JOYPAD_E,
    JOYPAD_NW,
    JOYPAD_N,
    JOYPAD_NE,
    JOYPAD_FIRE2,
    JOYPAD_FIRE3,
    JOYPAD_FIRE4,
    JOYPAD_FIRE5,
    JOYPAD_FIRE6,
    JOYPAD_FIRE7,
    JOYPAD_FIRE8
};

static int joypad_status[JOYSTICK_KEYSET_NUM][JOYSTICK_KEYSET_NUM_KEYS];
static int joypad_vmask[JOYSTICK_KEYSET_NUM];
static int joypad_hmask[JOYSTICK_KEYSET_NUM];

/* convert the given keyset status array into the corrosponding bits for the
 * joystick
 */
static int getjoyvalue(int *status)
{
    int val = 0;
    int column;

    for (column = 0; column < JOYSTICK_KEYSET_NUM_KEYS; column++) {
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
    joykeys_enable = val ? 1 : 0;

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

static const resource_int_t joykeys_resources_int[] = {
    { "KeySet1NorthWest", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_NW], set_keyset1, (void *)JOYSTICK_KEYSET_NW },
    { "KeySet1North", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_N], set_keyset1, (void *)JOYSTICK_KEYSET_N },
    { "KeySet1NorthEast", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_NE], set_keyset1, (void *)JOYSTICK_KEYSET_NE },
    { "KeySet1East", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_E], set_keyset1, (void *)JOYSTICK_KEYSET_E },
    { "KeySet1SouthEast", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_SE], set_keyset1, (void *)JOYSTICK_KEYSET_SE },
    { "KeySet1South", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_S], set_keyset1, (void *)JOYSTICK_KEYSET_S },
    { "KeySet1SouthWest", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_SW], set_keyset1, (void *)JOYSTICK_KEYSET_SW },
    { "KeySet1West", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_W], set_keyset1, (void *)JOYSTICK_KEYSET_W },
    { "KeySet1Fire", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_FIRE], set_keyset1, (void *)JOYSTICK_KEYSET_FIRE },
    { "KeySet1Fire2", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
        &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_FIRE2], set_keyset1, (void *)JOYSTICK_KEYSET_FIRE2 },
    { "KeySet1Fire3", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
        &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_FIRE3], set_keyset1, (void *)JOYSTICK_KEYSET_FIRE3 },
    { "KeySet1Fire4", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
        &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_FIRE4], set_keyset1, (void *)JOYSTICK_KEYSET_FIRE4 },
    { "KeySet1Fire5", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
        &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_FIRE5], set_keyset1, (void *)JOYSTICK_KEYSET_FIRE5 },
    { "KeySet1Fire6", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
        &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_FIRE6], set_keyset1, (void *)JOYSTICK_KEYSET_FIRE6 },
    { "KeySet1Fire7", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
        &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_FIRE7], set_keyset1, (void *)JOYSTICK_KEYSET_FIRE7 },
    { "KeySet1Fire8", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
        &joykeys[JOYSTICK_KEYSET_IDX_A][JOYSTICK_KEYSET_FIRE8], set_keyset1, (void *)JOYSTICK_KEYSET_FIRE8 },
    { "KeySet2NorthWest", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_NW], set_keyset2, (void *)JOYSTICK_KEYSET_NW },
    { "KeySet2North", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_N], set_keyset2, (void *)JOYSTICK_KEYSET_N },
    { "KeySet2NorthEast", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_NE], set_keyset2, (void *)JOYSTICK_KEYSET_NE },
    { "KeySet2East", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_E], set_keyset2, (void *)JOYSTICK_KEYSET_E },
    { "KeySet2SouthEast", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_SE], set_keyset2, (void *)JOYSTICK_KEYSET_SE },
    { "KeySet2South", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_S], set_keyset2, (void *)JOYSTICK_KEYSET_S },
    { "KeySet2SouthWest", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_SW], set_keyset2, (void *)JOYSTICK_KEYSET_SW },
    { "KeySet2West", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_W], set_keyset2, (void *)JOYSTICK_KEYSET_W },
    { "KeySet2Fire", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
      &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_FIRE], set_keyset2, (void *)JOYSTICK_KEYSET_FIRE },
    { "KeySet2Fire2", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
        &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_FIRE2], set_keyset2, (void *)JOYSTICK_KEYSET_FIRE2 },
    { "KeySet2Fire3", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
        &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_FIRE3], set_keyset2, (void *)JOYSTICK_KEYSET_FIRE3 },
    { "KeySet2Fire4", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
        &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_FIRE4], set_keyset2, (void *)JOYSTICK_KEYSET_FIRE4 },
    { "KeySet2Fire5", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
        &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_FIRE5], set_keyset2, (void *)JOYSTICK_KEYSET_FIRE5 },
    { "KeySet2Fire6", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
        &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_FIRE6], set_keyset2, (void *)JOYSTICK_KEYSET_FIRE6 },
    { "KeySet2Fire7", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
        &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_FIRE7], set_keyset2, (void *)JOYSTICK_KEYSET_FIRE7 },
    { "KeySet2Fire8", ARCHDEP_KEYBOARD_SYM_NONE, RES_EVENT_NO, NULL,
        &joykeys[JOYSTICK_KEYSET_IDX_B][JOYSTICK_KEYSET_FIRE8], set_keyset2, (void *)JOYSTICK_KEYSET_FIRE8 },
    { "KeySetEnable", 1, RES_EVENT_NO, NULL,
      &joykeys_enable, set_joykeys_enable, NULL },
    RESOURCE_INT_LIST_END
};

#ifdef DEBUGJOY
static void DBGSTATUS(int keysetnum, int value, int joyport, int key, int flg)
{
    int column;
    char *flags[3] = { "set", "unset", "ignored" };

    DBG((" key:%02x |", (unsigned int)key));
    for (column = 0; column < JOYSTICK_KEYSET_NUM_KEYS; column++) {
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
#if 0
    DBG(("|"));
    for (column = 5; column >= 0; column--) {
        DBG((((latch_joystick_value[joyport] >> column) & 1) ? "*" : "."));
    }
#endif
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

    for (column = 0; column < JOYSTICK_KEYSET_NUM_KEYS; column++) {
        if (key == joykeys[keysetnum][column]) {
            DBG(("joystick_check_set:"));

            joypad_status[keysetnum][column] = 1;
            value = getjoyvalue(joypad_status[keysetnum]);

            if (!joystick_opposite_enable) {
                /* setup the mask for the opposite side of the pressed key */
                if ((column == JOYSTICK_KEYSET_N) || (column == JOYSTICK_KEYSET_NW) || (column == JOYSTICK_KEYSET_NE)) {
                    joypad_vmask[keysetnum] = ~JOYPAD_S;
                } else if ((column == JOYSTICK_KEYSET_S) || (column == JOYSTICK_KEYSET_SW) || (column == JOYSTICK_KEYSET_SE)) {
                    joypad_vmask[keysetnum] = ~JOYPAD_N;
                }
                if ((column == JOYSTICK_KEYSET_W) || (column == JOYSTICK_KEYSET_SW) || (column == JOYSTICK_KEYSET_NW)) {
                    joypad_hmask[keysetnum] = ~JOYPAD_E;
                } else if ((column == JOYSTICK_KEYSET_E) || (column == JOYSTICK_KEYSET_SE) || (column == JOYSTICK_KEYSET_NE)) {
                    joypad_hmask[keysetnum] = ~JOYPAD_W;
                }
                /* if two opposite directions are set, mask out the opposite side of
                 * the last pressed key */
                if ((value & joypad_bits[JOYSTICK_KEYSET_N]) && (value & joypad_bits[JOYSTICK_KEYSET_S])) {
                    value &= joypad_vmask[keysetnum];
                }
                if ((value & joypad_bits[JOYSTICK_KEYSET_E]) && (value & joypad_bits[JOYSTICK_KEYSET_W])) {
                    value &= joypad_hmask[keysetnum];
                }
            }

            joystick_set_value_absolute(joyport, (uint16_t)value);

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

    for (column = 0; column < JOYSTICK_KEYSET_NUM_KEYS; column++) {
        if (key == joykeys[keysetnum][column]) {
            joypad_status[keysetnum][column] = 0;
            value = getjoyvalue(joypad_status[keysetnum]);

            if (!joystick_opposite_enable) {
                /* if two opposite directions are set, mask out the opposite side of
                 * the last pressed key */
                if ((value & joypad_bits[JOYSTICK_KEYSET_N]) && (value & joypad_bits[JOYSTICK_KEYSET_S])) {
                    value &= joypad_vmask[keysetnum];
                }
                if ((value & joypad_bits[JOYSTICK_KEYSET_E]) && (value & joypad_bits[JOYSTICK_KEYSET_W])) {
                    value &= joypad_hmask[keysetnum];
                }
            }

            joystick_set_value_absolute(joyport, (uint16_t)value);

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
#endif /* COMMON_JOYKEYS */

/*-----------------------------------------------------------------------*/

static joyport_mapping_t joystick_mapping = {
    "Joystick",   /* name of the device */
    "Up",         /* name for the mapping of pin 0 (UP) */
    "Down",       /* name for the mapping of pin 1 (DOWN) */
    "Left",       /* name for the mapping of pin 2 (LEFT) */
    "Right",      /* name for the mapping of pin 3 (RIGHT) */
    "Fire1",      /* name for the mapping of pin 4 (FIRE-1/SNES-A) */
    "Fire2",      /* name for the mapping of pin 5 (FIRE-2/SNES-B) */
    "Fire3",      /* name for the mapping of pin 6 (FIRE-3/SNES-X) */
    NULL,         /* NO mapping of pin 7 (SNES-Y) */
    NULL,         /* NO mapping of pin 8 (SNES-LB) */
    NULL,         /* NO mapping of pin 9 (SNES-RB) */
    NULL,         /* NO mapping of pin 10 (SNES-SELECT) */
    NULL,         /* NO mapping of pin 11 (SNES-START) */
    NULL,         /* NO mapping of pot 1 (POT-X) */
    NULL          /* NO mapping of pot 2 (POT-Y) */
};

static joyport_mapping_t joystick_no_pot_mapping = {
    "Joystick",   /* name of the device */
    "Up",         /* name for the mapping of pin 0 (UP) */
    "Down",       /* name for the mapping of pin 1 (DOWN) */
    "Left",       /* name for the mapping of pin 2 (LEFT) */
    "Right",      /* name for the mapping of pin 3 (RIGHT) */
    "Fire",       /* name for the mapping of pin 4 (FIRE-1/SNES-A) */
    NULL,         /* NO mapping of pin 5 (FIRE-2/SNES-B) */
    NULL,         /* NO mapping of pin 6 (FIRE-3/SNES-X) */
    NULL,         /* NO mapping of pin 7 (SNES-Y) */
    NULL,         /* NO mapping of pin 8 (SNES-LB) */
    NULL,         /* NO mapping of pin 9 (SNES-RB) */
    NULL,         /* NO mapping of pin 10 (SNES-SELECT) */
    NULL,         /* NO mapping of pin 11 (SNES-START) */
    NULL,         /* NO mapping of pot 1 (POT-X) */
    NULL          /* NO mapping of pot 2 (POT-Y) */
};

static joyport_mapping_t snes_mapping = {
    "SNES Pad",     /* name of the device */
    "D-Pad Up",     /* name for the mapping of pin 0 (UP) */
    "D-Pad Down",   /* name for the mapping of pin 1 (DOWN) */
    "D-Pad Left",   /* name for the mapping of pin 2 (LEFT) */
    "D-Pad Right",  /* name for the mapping of pin 3 (RIGHT) */
    "A Button",     /* name for the mapping of pin 4 (FIRE-1/SNES-A) */
    "B Button",     /* name for the mapping of pin 5 (FIRE-2/SNES-B) */
    "X Button",     /* name for the mapping of pin 6 (FIRE-3/SNES-X) */
    "Y Button",     /* name for the mapping of pin 7 (SNES-Y) */
    "Left Bumber",  /* name for the mapping of pin 8 (SNES-LB) */
    "Right Bumper", /* name for the mapping of pin 9 (SNES-RB) */
    "Select",       /* name for the mapping of pin 10 (SNES-SELECT) */
    "Start",        /* name for the mapping of pin 11 (SNES-START) */
    NULL,           /* NO mapping of pot 1 (POT-X) */
    NULL            /* NO mapping of pot 2 (POT-Y) */
};

void joystick_set_snes_mapping(int port)
{
    joyport_set_mapping(&snes_mapping, port);
}

static int joyport_enable_joystick(int port, int val)
{
    joyport_mapping_t *mapping = NULL;

    joyport_joystick[port] = (val) ? 1 : 0;
    if (val) {
        if (port == JOYPORT_1 || port == JOYPORT_2 || (port == JOYPORT_PLUS4_SIDCART && machine_class == VICE_MACHINE_PLUS4)) {
            if (joyport_port_has_pot(port)) {
                mapping = &joystick_mapping;
            } else {
                mapping = &joystick_no_pot_mapping;
            }
        } else {
            if (joystick_adapter_is_snes()) {
                mapping = &snes_mapping;
            } else {
                mapping = &joystick_no_pot_mapping;
            }
        }
        joyport_set_mapping(mapping, port);
    } else {
        joyport_clear_mapping(port);
    }

    return 0;
}

static uint8_t read_joystick(int port)
{
    return (uint8_t)(~(get_joystick_value(port) & 0x1f));
}

static uint8_t read_potx(int port)
{
    /* printf("read_potx %d %02x %02x %02x\n", port, joystick_value[port + 1]); */
    return joystick_value[port] & JOYPAD_FIRE2 ? 0x00 : 0xff;
}

static uint8_t read_poty(int port)
{
    /* printf("read_poty %d %02x %02x %02x\n", port, joystick_value[port + 1]); */
    return joystick_value[port] & JOYPAD_FIRE3 ? 0x00 : 0xff;
}


/* Some prototypes are needed */
static int joystick_snapshot_write_module(snapshot_t *s, int port);
static int joystick_snapshot_read_module(snapshot_t *s, int port);

static joyport_t joystick_device = {
    "Joystick",                     /* name of the device */
    JOYPORT_RES_ID_NONE,            /* device doesn't have a class, multiple devices of this kind can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,        /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,           /* device does NOT use the potentiometer lines */
    JOYPORT_5VDC_NOT_NEEDED,        /* device does NOT need +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,       /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_JOYSTICK,        /* device is a Joystick */
    0,                              /* NO output bits */
    joyport_enable_joystick,        /* device enable/disable function */
    read_joystick,                  /* digital line read function */
    NULL,                           /* NO digital line store function */
    read_potx,                      /* pot-x read function */
    read_poty,                      /* pot-y read function */
    NULL,                           /* NO powerup function */
    joystick_snapshot_write_module, /* device write snapshot function */
    joystick_snapshot_read_module,  /* device read snapshot function */
    NULL,                           /* NO device hook function */
    0                               /* NO device hook function mask */
};

int joystick_joyport_register(void)
{
    return joyport_device_register(JOYPORT_ID_JOYSTICK, &joystick_device);
}

/*--------------------------------------------------------------------------*/

static int set_joystick_opposite_enable(int val, void *param)
{
    joystick_opposite_enable = val ? 1 : 0;

    return 0;
}

typedef struct joystick_axis_mapping_s {
    /* Previous state of input */
    uint8_t prev;
    struct joystick_mapping_s positive_direction;
    struct joystick_mapping_s negative_direction;
    uint8_t pot;
} joystick_axis_mapping_t;

typedef struct joystick_button_mapping_s {
    /* Previous state of input */
    uint8_t prev;
    struct joystick_mapping_s mapping;
} joystick_button_mapping_t;

typedef struct joystick_hat_mapping_s {
    /* Previous state of input */
    uint8_t prev;
    struct joystick_mapping_s up;
    struct joystick_mapping_s down;
    struct joystick_mapping_s left;
    struct joystick_mapping_s right;
} joystick_hat_mapping_t;

static int num_joystick_devices = 0;

/** \brief  Joystick device name length (including 0)
 */
#define JOYDEV_NAME_SIZE    0x80

/* device structure */
typedef struct joystick_device_s {
    struct joystick_driver_s *driver;
    char jname[JOYDEV_NAME_SIZE];
    int joyport;
    void *priv;
    joystick_axis_mapping_t *axis_mapping;
    joystick_button_mapping_t *button_mapping;
    joystick_hat_mapping_t *hat_mapping;
    int num_axes;
    int num_hats;
    int num_buttons;
} joystick_device_t;


static struct joystick_device_s *joystick_devices = NULL;

static int set_joystick_device(int val, void *param)
{
    int port_idx = vice_ptr_to_int(param);

    if (joystick_port_map[port_idx] >= JOYDEV_REALJOYSTICK_MIN) {
        int olddev = joystick_port_map[port_idx] - JOYDEV_REALJOYSTICK_MIN;
        if (olddev < num_joystick_devices) {
            joystick_devices[olddev].joyport = -1;
        }
    }

    joystick_port_map[port_idx] = val;

    if (joystick_port_map[port_idx] >= JOYDEV_REALJOYSTICK_MIN) {
        int newdev = joystick_port_map[port_idx] - JOYDEV_REALJOYSTICK_MIN;
        if (newdev < num_joystick_devices) {
            joystick_devices[newdev].joyport = port_idx;
            int i;
            for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
                if (i != port_idx && joystick_port_map[port_idx] == joystick_port_map[i]) {
                    joystick_port_map[i] = JOYDEV_NONE;
                }
            }
        }
    }

    return 0;
}

void joystick_set_axis_value(unsigned int joynum, unsigned int axis, uint8_t value)
{
    if (joynum < num_joystick_devices
    && (joystick_devices[joynum].joyport == 0 || joystick_devices[joynum].joyport == 1)
    && joystick_devices[joynum].axis_mapping[axis].pot > 0) {
        joystick_axis_value[joystick_devices[joynum].joyport][joystick_devices[joynum].axis_mapping[axis].pot - 1] = value;
    }
}

static char mapping_retval[50];

char *get_joy_pot_mapping_string(int joystick_device_num, int pot)
{
    int j;
    char *retval = NULL;

    if (joystick_device_num >= 0 && joystick_device_num < num_joystick_devices) {
        for (j = 0; j < joystick_devices[joystick_device_num].num_axes; j++) {
            if (joystick_devices[joystick_device_num].axis_mapping[j].pot - 1 == pot) {
                snprintf(mapping_retval, 50, "Ax%d", j);
                retval = mapping_retval;
            }
        }
    }
    return retval;
}

char *get_joy_pin_mapping_string(int joystick_device_num, int pin)
{
    int j;
    joystick_action_t t;
    int valid = 0;
    int index = 0;
    int sub_index = 0;
    char *retval = NULL;
    char *type_string = NULL;
    char *index_string = NULL;

    if (joystick_device_num >= 0 && joystick_device_num < num_joystick_devices) {
        for (j = 0; j < joystick_devices[joystick_device_num].num_axes; j++) {
            t = joystick_devices[joystick_device_num].axis_mapping[j].positive_direction.action;
            if (t == JOY_ACTION_JOYSTICK && joystick_devices[joystick_device_num].axis_mapping[j].positive_direction.value.joy_pin == pin) {
                valid++;
                type_string = "Ax";
                index_string = "I";
                index = j;
                sub_index = 0;
            }
            t = joystick_devices[joystick_device_num].axis_mapping[j].negative_direction.action;
            if (t == JOY_ACTION_JOYSTICK && joystick_devices[joystick_device_num].axis_mapping[j].negative_direction.value.joy_pin == pin) {
                valid++;
                type_string = "Ax";
                index_string = "I";
                index = j;
                sub_index = 1;
            }
        }
        for (j = 0; j < joystick_devices[joystick_device_num].num_buttons; j++) {
            t = joystick_devices[joystick_device_num].button_mapping[j].mapping.action;
            if (t == JOY_ACTION_JOYSTICK) {
                if (joystick_devices[joystick_device_num].button_mapping[j].mapping.value.joy_pin == pin) {
                    valid++;
                    type_string = "Bt";
                    index_string = NULL;
                    index = j;
                    sub_index = 0;
                }
            }
        }
        for (j = 0; j < joystick_devices[joystick_device_num].num_hats; j++) {
            t = joystick_devices[joystick_device_num].hat_mapping[j].up.action;

            if (t == JOY_ACTION_JOYSTICK) {
                if (joystick_devices[joystick_device_num].hat_mapping[j].up.value.joy_pin == pin) {
                    valid++;
                    type_string = "Ht";
                    index_string = "I";
                    index = j;
                    sub_index = 0;
                }
            }
            t = joystick_devices[joystick_device_num].hat_mapping[j].down.action;

            if (t == JOY_ACTION_JOYSTICK) {
                if (joystick_devices[joystick_device_num].hat_mapping[j].down.value.joy_pin == pin) {
                    valid++;
                    type_string = "Ht";
                    index_string = "I";
                    index = j;
                    sub_index = 1;
                }
            }
            t = joystick_devices[joystick_device_num].hat_mapping[j].left.action;

            if (t == JOY_ACTION_JOYSTICK) {
                if (joystick_devices[joystick_device_num].hat_mapping[j].left.value.joy_pin == pin) {
                    valid++;
                    type_string = "Ht";
                    index_string = "I";
                    index = j;
                    sub_index = 2;
                }
            }
            t = joystick_devices[joystick_device_num].hat_mapping[j].right.action;

            if (t == JOY_ACTION_JOYSTICK) {
                if (joystick_devices[joystick_device_num].hat_mapping[j].right.value.joy_pin == pin) {
                    valid++;
                    type_string = "Ht";
                    index_string = "I";
                    index = j;
                    sub_index = 3;
                }
            }
        }
    }
    if (valid > 1) {
        retval = "Multiple";
    }
    if (valid == 1) {
        if (index_string != NULL ) {
            snprintf(mapping_retval, 50, "%s%d, %s%d", type_string, index, index_string, sub_index);
        } else {
            snprintf(mapping_retval, 50, "%s%d", type_string, index);
        }
        retval = mapping_retval;
    }

    return retval;
}

char *get_joy_extra_mapping_string(int which)
{
    int i, j;
    joystick_action_t t;
    int valid = 0;
    int joy = 0;
    int index = 0;
    int sub_index = 0;
    char *retval = NULL;
    char *type_string = NULL;
    char *index_string = NULL;

    for (i = 0; i < num_joystick_devices; i++) {
        for (j = 0; j < joystick_devices[i].num_axes; j++) {
            t = joystick_devices[i].axis_mapping[j].positive_direction.action;
            if (t == (which ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE)) {
                valid++;
                joy = i;
                type_string = "Ax";
                index_string = "I";
                index = j;
                sub_index = 0;
            }
            t = joystick_devices[i].axis_mapping[j].negative_direction.action;
            if (t == (which ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE)) {
                valid++;
                joy = i;
                type_string = "Ax";
                index_string = "I";
                index = j;
                sub_index = 1;
            }
        }
        for (j = 0; j < joystick_devices[i].num_buttons; j++) {
            t = joystick_devices[i].button_mapping[j].mapping.action;
            if (t == (which ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE)) {
                valid++;
                joy = i;
                type_string = "Bt";
                index_string = NULL;
                index = j;
                sub_index = 0;
            }
        }
        for (j = 0; j < joystick_devices[i].num_hats; j++) {
            t = joystick_devices[i].hat_mapping[j].up.action;

            if (t == (which ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE)) {
                valid++;
                joy = i;
                type_string = "Ht";
                index_string = "I";
                index = j;
                sub_index = 0;
            }
            t = joystick_devices[i].hat_mapping[j].down.action;

            if (t == (which ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE)) {
                valid++;
                joy = i;
                type_string = "Ht";
                index_string = "I";
                index = j;
                sub_index = 1;
            }
            t = joystick_devices[i].hat_mapping[j].left.action;

            if (t == (which ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE)) {
                valid++;
                joy = i;
                type_string = "Ht";
                index_string = "I";
                index = j;
                sub_index = 2;
            }
            t = joystick_devices[i].hat_mapping[j].right.action;

            if (t == (which ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE)) {
                valid++;
                joy = i;
                type_string = "Ht";
                index_string = "I";
                index = j;
                sub_index = 3;
            }
        }
    }
    if (valid > 1) {
        retval = "Multiple";
    }
    if (valid == 1) {
        if (index_string != NULL ) {
            snprintf(mapping_retval, 50, "J%d, %s%d, %s%d", joy, type_string, index, index_string, sub_index);
        } else {
            snprintf(mapping_retval, 50, "J%d, %s%d", joy, type_string, index);
        }
        retval = mapping_retval;
    }

    return retval;
}

void joy_set_pot_mapping(int joystick_device_num, int axis, int pot)
{
    joystick_devices[joystick_device_num].axis_mapping[axis].pot = pot + 1;
}

void joy_delete_pot_mapping(int joystick_device_num, int pot)
{
    int j;

    if (joystick_device_num >= 0 && joystick_device_num < num_joystick_devices) {
        for (j = 0; j < joystick_devices[joystick_device_num].num_axes; j++) {
            if (joystick_devices[joystick_device_num].axis_mapping[j].pot - 1 == pot) {
                joystick_devices[joystick_device_num].axis_mapping[j].pot = 0;
            }
        }
    }
}

void joy_delete_pin_mapping(int joystick_device_num, int pin)
{
    int j;
    joystick_action_t t;

    if (joystick_device_num >= 0 && joystick_device_num < num_joystick_devices) {
        for (j = 0; j < joystick_devices[joystick_device_num].num_axes; j++) {
            t = joystick_devices[joystick_device_num].axis_mapping[j].positive_direction.action;
            if (t == JOY_ACTION_JOYSTICK) {
                if (joystick_devices[joystick_device_num].axis_mapping[j].positive_direction.value.joy_pin == pin) {
                    joystick_devices[joystick_device_num].axis_mapping[j].positive_direction.action = JOY_ACTION_NONE;
                    joystick_devices[joystick_device_num].axis_mapping[j].positive_direction.value.joy_pin = 0;
                }
            }
            t = joystick_devices[joystick_device_num].axis_mapping[j].negative_direction.action;
            if (t == JOY_ACTION_JOYSTICK) {
                if (joystick_devices[joystick_device_num].axis_mapping[j].negative_direction.value.joy_pin == pin) {
                    joystick_devices[joystick_device_num].axis_mapping[j].negative_direction.action = JOY_ACTION_NONE;
                    joystick_devices[joystick_device_num].axis_mapping[j].negative_direction.value.joy_pin = 0;
                }
            }
        }
        for (j = 0; j < joystick_devices[joystick_device_num].num_buttons; j++) {
            t = joystick_devices[joystick_device_num].button_mapping[j].mapping.action;
            if (t == JOY_ACTION_JOYSTICK) {
                if (joystick_devices[joystick_device_num].button_mapping[j].mapping.value.joy_pin == pin) {
                    joystick_devices[joystick_device_num].button_mapping[j].mapping.action = JOY_ACTION_NONE;
                    joystick_devices[joystick_device_num].button_mapping[j].mapping.value.joy_pin = 0;
                }
            }
        }
        for (j = 0; j < joystick_devices[joystick_device_num].num_hats; j++) {
            t = joystick_devices[joystick_device_num].hat_mapping[j].up.action;
            if (t == JOY_ACTION_JOYSTICK) {
                if (joystick_devices[joystick_device_num].hat_mapping[j].up.value.joy_pin == pin) {
                    joystick_devices[joystick_device_num].hat_mapping[j].up.action = JOY_ACTION_NONE;
                    joystick_devices[joystick_device_num].hat_mapping[j].up.value.joy_pin = 0;
                }
            }
            t = joystick_devices[joystick_device_num].hat_mapping[j].down.action;
            if (t == JOY_ACTION_JOYSTICK) {
                if (joystick_devices[joystick_device_num].hat_mapping[j].down.value.joy_pin == pin) {
                    joystick_devices[joystick_device_num].hat_mapping[j].down.action = JOY_ACTION_NONE;
                    joystick_devices[joystick_device_num].hat_mapping[j].down.value.joy_pin = 0;
                }
            }
            t = joystick_devices[joystick_device_num].hat_mapping[j].left.action;
            if (t == JOY_ACTION_JOYSTICK) {
                if (joystick_devices[joystick_device_num].hat_mapping[j].left.value.joy_pin == pin) {
                    joystick_devices[joystick_device_num].hat_mapping[j].left.action = JOY_ACTION_NONE;
                    joystick_devices[joystick_device_num].hat_mapping[j].left.value.joy_pin = 0;
                }
            }
            t = joystick_devices[joystick_device_num].hat_mapping[j].right.action;
            if (t == JOY_ACTION_JOYSTICK) {
                if (joystick_devices[joystick_device_num].hat_mapping[j].right.value.joy_pin == pin) {
                    joystick_devices[joystick_device_num].hat_mapping[j].right.action = JOY_ACTION_NONE;
                    joystick_devices[joystick_device_num].hat_mapping[j].right.value.joy_pin = 0;
                }
            }
        }
    }
}

#if (defined USE_SDLUI ||defined USE_SDL2UI)
void joy_delete_extra_mapping(int type)
{
    int i, j;
    joystick_action_t t;

    for (i = 0; i < num_joystick_devices; i++) {
        for (j = 0; j < joystick_devices[i].num_axes; j++) {
            t = joystick_devices[i].axis_mapping[j].positive_direction.action;
            if (t == (type ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE)) {
                joystick_devices[i].axis_mapping[j].positive_direction.action = JOY_ACTION_NONE;
                joystick_devices[i].axis_mapping[j].positive_direction.value.ui_action = ACTION_NONE;
            }
            t = joystick_devices[i].axis_mapping[j].negative_direction.action;
            if (t == (type ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE)) {
                joystick_devices[i].axis_mapping[j].negative_direction.action = JOY_ACTION_NONE;
                joystick_devices[i].axis_mapping[j].negative_direction.value.ui_action = ACTION_NONE;
            }
        }
        for (j = 0; j < joystick_devices[i].num_buttons; j++) {
            t = joystick_devices[i].button_mapping[j].mapping.action;
            if (t == (type ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE)) {
                joystick_devices[i].button_mapping[j].mapping.action = JOY_ACTION_NONE;
                joystick_devices[i].button_mapping[j].mapping.value.ui_action = ACTION_NONE;
            }
        }
        for (j = 0; j < joystick_devices[i].num_hats; j++) {
            t = joystick_devices[i].hat_mapping[j].up.action;
            if (t == (type ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE)) {
                joystick_devices[i].hat_mapping[j].up.action = JOY_ACTION_NONE;
                joystick_devices[i].hat_mapping[j].up.value.ui_action = ACTION_NONE;
            }
            t = joystick_devices[i].hat_mapping[j].down.action;
            if (t == (type ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE)) {
                joystick_devices[i].hat_mapping[j].down.action = JOY_ACTION_NONE;
                joystick_devices[i].hat_mapping[j].down.value.ui_action = ACTION_NONE;
            }
            t = joystick_devices[i].hat_mapping[j].left.action;
            if (t == (type ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE)) {
                joystick_devices[i].hat_mapping[j].left.action = JOY_ACTION_NONE;
                joystick_devices[i].hat_mapping[j].left.value.ui_action = ACTION_NONE;
            }
            t = joystick_devices[i].hat_mapping[j].right.action;
            if (t == (type ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE)) {
                joystick_devices[i].hat_mapping[j].right.action = JOY_ACTION_NONE;
                joystick_devices[i].hat_mapping[j].right.value.ui_action = ACTION_NONE;
            }
        }
    }
}
#endif


static void mapping_dump_header(FILE *fp)
{
    fprintf(fp, "# VICE joystick mapping file\n"
            "#\n"
            "# A joystick map is read in as patch to the current map.\n"
            "#\n"
            "# File format:\n"
            "# - comment lines start with '#'\n"
            "# - keyword lines start with '!keyword'\n"
            "# - normal line has 'joynum inputtype inputindex action'\n"
            "#\n"
            "# Keywords and their lines are:\n"
            "# '!CLEAR'    clear all mappings\n"
            "#\n"
            );

    fprintf(fp, "# inputtype:\n"
            "# 0      axis\n"
            "# 1      button\n"
            "# 2      hat\n"
            "#\n"
            "# For buttons, inputindex is the zero-based index of the button.\n"
            "# For hats: hat 0 has inputindex 0,1,2,3 respectively for up, down, left and right. Hat 1 has 5,6,7,8 etc.\n"
            "# For axes, and action 1 (joystick) and 2 (keyboard): axis 0 has inputindex 0,1 respectively for positive and negative, axis 1 has 2,3 etc.\n"
            "# For axes, and action 6 (pot axis): inputindex is the zero-based index of the axis.\n"
            "#\n"
            "# action [action_parameters]:\n"
            "# 0               none\n"
            "# 1 pin           joystick (pin: 1/2/4/8/16/32/64 = u/d/l/r/fire/fire2/fire3)\n"
            "# 2 row col       keyboard\n"
            "# 3               map\n"
            "# 4               UI activate\n"
            "# 5 action-name   UI function\n"
            "# 6 pot           potentiometer (1=pot x, 2=pot y)\n"
            "#\n\n"
            );
}

/** \brief Dump mapping of host controller input to emulator input
 *
 * Dump mapping of a host axis, button or hat to an emulator key, joystick
 * button or UI action.
 *
 * \param[in]   fp              file pointer of the .vjm file
 * \param[in]   device_index    host device index
 * \param[in]   input_type      input type (axis, button, hat)
 * \param[in]   map_index       sub index of mapping of \a input_type
 * \param[in]   map             mapping to dump
 */
static void mapping_dump_map(FILE               *fp,
                             int                 device_index,
                             joystick_input_t    input_type,
                             int                 map_index,
                             joystick_mapping_t *map)
{
    fprintf(fp, "%i %u %i %u", device_index, input_type, map_index, map->action);
    switch (map->action) {
        case JOY_ACTION_JOYSTICK:
            fprintf(fp, " %i", map->value.joy_pin);
            break;
        case JOY_ACTION_KEYBOARD:
            fprintf(fp, " %i %i", map->value.key[0], map->value.key[1]);
            break;
        case JOY_ACTION_UI_FUNCTION:
            fprintf(fp, " %s", ui_action_get_name(map->value.ui_action));
            break;
        default:
            break;
    }
    fprintf(fp, "\n");
}


int joy_arch_mapping_dump(const char *filename)
{
    FILE *fp;
    int   dev_idx;
    int   map_idx;

#ifdef SDL_DEBUG
    fprintf(stderr, "%s\n", __func__);
#endif

    if (filename == NULL) {
        return -1;
    }

    fp = fopen(filename, MODE_WRITE_TEXT);
    if (fp == NULL) {
        return -1;
    }

    mapping_dump_header(fp);

    fprintf(fp, "!CLEAR\n\n");

    for (dev_idx = 0; dev_idx < num_joystick_devices; dev_idx++) {
        joystick_device_t *device = &joystick_devices[dev_idx];
        int                row    = 0;

        fprintf(fp, "# %s\n", device->jname);

        /* dump axis mappings */
        for (map_idx = 0; map_idx < device->num_axes; map_idx++) {
            joystick_axis_mapping_t *axis = &(device->axis_mapping[map_idx]);

            if (axis->pot > 0) {
                fprintf(fp, "%i %i %i %i %u\n",
                        dev_idx, JOY_INPUT_AXIS, map_idx, JOY_ACTION_POT_AXIS, axis->pot);
            } else {
                mapping_dump_map(fp, dev_idx, JOY_INPUT_AXIS, row + 0, &(axis->positive_direction));
                mapping_dump_map(fp, dev_idx, JOY_INPUT_AXIS, row + 1, &(axis->negative_direction));
            }
            fprintf(fp, "\n");
            row += 2;
        }

        /* dump button mappings */
        for (map_idx = 0; map_idx < device->num_buttons; map_idx++) {
            joystick_button_mapping_t *button = &(device->button_mapping[map_idx]);

            mapping_dump_map(fp, dev_idx, JOY_INPUT_BUTTON, map_idx, &button->mapping);
        }
        fprintf(fp, "\n");

        /* dump hat mappings */
        row = 0;
        for (map_idx = 0; map_idx < device->num_hats; map_idx++) {
            joystick_hat_mapping_t *hat = &(device->hat_mapping[map_idx]);

            /* indexes 0-3 are hardcoded to up, down, left and right */
            mapping_dump_map(fp, dev_idx, JOY_INPUT_HAT, row + 0, &(hat->up));
            mapping_dump_map(fp, dev_idx, JOY_INPUT_HAT, row + 1, &(hat->down));
            mapping_dump_map(fp, dev_idx, JOY_INPUT_HAT, row + 2, &(hat->left));
            mapping_dump_map(fp, dev_idx, JOY_INPUT_HAT, row + 3, &(hat->right));
            row += 4;
        }

        /* avoid printing newlines at end of dump */
        if (dev_idx < num_joystick_devices - 1) {
            fprintf(fp, "\n\n");
        }
    }

    fclose(fp);
    return 0;
}


static void joy_arch_keyword_clear(void)
{
    int i, k;

    for (i = 0; i < num_joystick_devices; ++i) {
        for (k = 0; k < joystick_devices[i].num_axes; ++k) {
            joystick_devices[i].axis_mapping[k].positive_direction.action = JOY_ACTION_NONE;
            joystick_devices[i].axis_mapping[k].negative_direction.action = JOY_ACTION_NONE;
        }
        for (k = 0; k < joystick_devices[i].num_buttons; ++k) {
            joystick_devices[i].button_mapping[k].mapping.action = JOY_ACTION_NONE;
        }
        for (k = 0; k < joystick_devices[i].num_hats; ++k) {
            joystick_devices[i].hat_mapping[k].up.action = JOY_ACTION_NONE;
            joystick_devices[i].hat_mapping[k].down.action = JOY_ACTION_NONE;
            joystick_devices[i].hat_mapping[k].left.action = JOY_ACTION_NONE;
            joystick_devices[i].hat_mapping[k].right.action = JOY_ACTION_NONE;
        }
    }
}

static void joy_arch_parse_keyword(char *buffer)
{
    char *key;

    key = strtok(buffer + 1, " \t:");

    if (!strcmp(key, "CLEAR")) {
        joy_arch_keyword_clear();
    }
}


/* Comment out to revert to old parser */
#define NEW_ENTRY_PARSER

#ifndef NEW_ENTRY_PARSER
/* Old parser, kept for reference and bug checking */
static void joy_arch_parse_entry(char *buffer)
{
    char *p;
    int joynum, inputindex, data1 = 0, data2 = 0;
    int inputtype;
    joystick_action_t action;
    int axis_or_hat_index, direction;
    char valid = 0;
    int action_id;

    p = strtok(buffer, " \t:");

    joynum = atoi(p);

    if (joynum >= num_joystick_devices) {
        log_error(joy_log, "Could not find joystick %i!", joynum);
        return;
    }

    p = strtok(NULL, " \t,");
    if (p != NULL) {
        inputtype = atoi(p);
        p = strtok(NULL, " \t,");
        if (p != NULL) {
            inputindex = atoi(p);
            p = strtok(NULL, " \t");
            if (p != NULL) {
                action = (joystick_action_t)atoi(p);

                switch (action) {
                    case JOY_ACTION_UI_FUNCTION:
                        p = strtok(NULL, "\t\r\n");
                        if (p != NULL) {
                            valid = 1;
                        }
                        break;
                    case JOY_ACTION_JOYSTICK:
                    case JOY_ACTION_POT_AXIS:
                        p = strtok(NULL, " \t");
                        if (p != NULL) {
                            data1 = atoi(p);
                            valid = 1;
                        }
                        break;
                    case JOY_ACTION_KEYBOARD:
                        p = strtok(NULL, " \t");
                        if (p != NULL) {
                            data1 = atoi(p);
                            p = strtok(NULL, " \t");
                            if (p != NULL) {
                                data2 = atoi(p);
                                valid = 1;
                            }
                        }
                        break;
                    case JOY_ACTION_UI_ACTIVATE:
                        valid = 1;
                        break;
                    default:
                        break;
                }

                if (valid) {
                    switch (inputtype) {
                        case 0:
                            if (action == JOY_ACTION_POT_AXIS) {
                                if (inputindex < joystick_devices[joynum].num_axes) {
                                    joystick_devices[joynum].axis_mapping[inputindex].pot = data1;
                                } else {
                                    log_warning(joy_log, "inputindex %i too large for inputtype axis (pot), joynum %i!", inputindex, joynum);
                                }
                            } else {
                                axis_or_hat_index = inputindex / 2;
                                direction = inputindex % 2;
                                if (axis_or_hat_index < joystick_devices[joynum].num_axes) {
                                    if (direction == 0) {
                                        joystick_devices[joynum].axis_mapping[axis_or_hat_index].positive_direction.action = action;
                                        switch (action) {
                                            case JOY_ACTION_JOYSTICK:
                                                joystick_devices[joynum].axis_mapping[axis_or_hat_index].positive_direction.value.joy_pin = data1;
                                                break;
                                            case JOY_ACTION_KEYBOARD:
                                                joystick_devices[joynum].axis_mapping[axis_or_hat_index].positive_direction.value.key[0] = data1;
                                                joystick_devices[joynum].axis_mapping[axis_or_hat_index].positive_direction.value.key[1] = data2;
                                                break;
                                            case JOY_ACTION_UI_FUNCTION:
                                                joystick_devices[joynum].axis_mapping[axis_or_hat_index].positive_direction.value.ui_action = ui_action_get_id(p);
                                               break;
                                            default:
                                                break;
                                        }
                                    } else {
                                        joystick_devices[joynum].axis_mapping[axis_or_hat_index].negative_direction.action = action;
                                        switch (action) {
                                            case JOY_ACTION_JOYSTICK:
                                                joystick_devices[joynum].axis_mapping[axis_or_hat_index].negative_direction.value.joy_pin = data1;
                                                break;
                                            case JOY_ACTION_KEYBOARD:
                                                joystick_devices[joynum].axis_mapping[axis_or_hat_index].negative_direction.value.key[0] = data1;
                                                joystick_devices[joynum].axis_mapping[axis_or_hat_index].negative_direction.value.key[1] = data2;
                                                break;
                                            case JOY_ACTION_UI_FUNCTION:
                                                joystick_devices[joynum].axis_mapping[axis_or_hat_index].negative_direction.value.ui_action = ui_action_get_id(p);
                                               break;
                                            default:
                                                break;
                                        }
                                    }
                                } else {
                                    log_warning(joy_log, "inputindex %i too large for inputtype axis, joynum %i!", inputindex, joynum);
                                }
                            }
                            break;
                        case 1:
                            if (inputindex < joystick_devices[joynum].num_buttons) {
                                joystick_devices[joynum].button_mapping[inputindex].mapping.action = action;
                                switch (action) {
                                    case JOY_ACTION_JOYSTICK:
                                        joystick_devices[joynum].button_mapping[inputindex].mapping.value.joy_pin = data1;
                                        break;
                                    case JOY_ACTION_KEYBOARD:
                                        joystick_devices[joynum].button_mapping[inputindex].mapping.value.key[0] = data1;
                                        joystick_devices[joynum].button_mapping[inputindex].mapping.value.key[1] = data2;
                                        break;
                                    case JOY_ACTION_UI_FUNCTION:
                                        action_id = ui_action_get_id(p);
                                        joystick_devices[joynum].button_mapping[inputindex].mapping.value.ui_action = action_id;
                                        break;
                                    default:
                                        break;
                                }
                            } else {
                                log_warning(joy_log, "inputindex %i too large for inputtype button, joynum %i!", inputindex, joynum);
                            }
                            break;
                        case 2:
                            axis_or_hat_index = inputindex / 4;
                            direction = inputindex % 4;
                            if (axis_or_hat_index < joystick_devices[joynum].num_hats) {
                                if (direction == 0) {
                                    joystick_devices[joynum].hat_mapping[axis_or_hat_index].up.action = action;
                                    switch (action) {
                                        case JOY_ACTION_JOYSTICK:
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].up.value.joy_pin = data1;
                                            break;
                                        case JOY_ACTION_KEYBOARD:
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].up.value.key[0] = data1;
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].up.value.key[1] = data2;
                                            break;
                                        case JOY_ACTION_UI_FUNCTION:
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].up.value.ui_action = ui_action_get_id(p);
                                            break;
                                        default:
                                            break;
                                    }
                                } else if (direction == 1) {
                                    joystick_devices[joynum].hat_mapping[axis_or_hat_index].down.action = action;
                                    switch (action) {
                                        case JOY_ACTION_JOYSTICK:
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].down.value.joy_pin = data1;
                                            break;
                                        case JOY_ACTION_KEYBOARD:
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].down.value.key[0] = data1;
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].down.value.key[1] = data2;
                                            break;
                                        case JOY_ACTION_UI_FUNCTION:
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].down.value.ui_action = ui_action_get_id(p);
                                            break;
                                        default:
                                            break;
                                    }
                                } else if (direction == 2) {
                                    joystick_devices[joynum].hat_mapping[axis_or_hat_index].left.action = action;
                                    switch (action) {
                                        case JOY_ACTION_JOYSTICK:
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].left.value.joy_pin = data1;
                                            break;
                                        case JOY_ACTION_KEYBOARD:
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].left.value.key[0] = data1;
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].left.value.key[1] = data2;
                                            break;
                                        case JOY_ACTION_UI_FUNCTION:
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].left.value.ui_action = ui_action_get_id(p);
                                            break;
                                        default:
                                            break;
                                    }
                                } else if (direction == 3) {
                                    joystick_devices[joynum].hat_mapping[axis_or_hat_index].right.action = action;
                                    switch (action) {
                                        case JOY_ACTION_JOYSTICK:
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].right.value.joy_pin = data1;
                                            break;
                                        case JOY_ACTION_KEYBOARD:
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].right.value.key[0] = data1;
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].right.value.key[1] = data2;
                                            break;
                                        case JOY_ACTION_UI_FUNCTION:
                                            joystick_devices[joynum].hat_mapping[axis_or_hat_index].right.value.ui_action = ui_action_get_id(p);
                                            break;
                                        default:
                                            break;
                                    }
                                }
                            } else {
                                log_warning(joy_log, "inputindex %i too large for inputtype hat, joynum %i!", inputindex, joynum);
                            }
                            break;
                    }
                }
            }
        }
    }
}
#endif

#ifdef NEW_ENTRY_PARSER

/*
 * Parser reimplementation: get rid of the nested switch statements insanity,
 * split massive function into smaller functions and add proper input checking
 * and error reporting.
 */

/** \brief  Parser state object
 *
 * This object is used to pass parser state between functions used by the
 * parser.
 */
typedef struct parser_state_s {
    const char        *filename;    /**< file being parsed */
    int                lineno;      /**< line number in file */
    const char        *buffer;      /**< buffer being parsed */
    const char        *bufptr;      /**< current position in buffer */

    /* mandatory columns */
    int                joy_index;   /**< joystick index */
    joystick_input_t   input_type;  /**< input type */
    int                input_index; /**< input index */
    joystick_action_t  action;      /**< action number */

    union {
        uint16_t pin;           /**< joystick pin number */
        int      pot;           /**< potentiometer number */
        int      ui_action_id;  /**< UI action ID */
        struct {
            int row;            /**< keyboard matrix row */
            int column;         /**< keyboard matrix column */
            int flags;          /**< key event flags */
        } key;
    } args; /**< action-specific arguments */
} parser_state_t;


/** \brief  Parse text for integer literal
 *
 * \param[in]   text    text possibly containing an integer literal
 * \param[out]  endptr  positionion of first non-digit character in \a text
 * \param[out]  value   integer result
 *
 * \return  \c true on success
 * \note    Does not check for overflow/underflow.
 */
static bool parse_int(const char *text, char **endptr, int *value)
{
    const char *s = text;
    long        v;

    v = strtol(text, endptr, 10);
    if (*endptr == s) {
        return false;
    }
    *value = (int)v;
    return true;
}

/** \brief  Parse a number of whitespace separated integer literals
 *
 * Parse \a text for at most \a num integer literals and store them in \a values.
 *
 * \param[in]   text    text to parse for integer literals
 * \param[out]  endptr  pointer to first non-digit in \a text
 * \param[in]   num     maximum number of integer literals to parse
 * \param[out]  values  destination for values
 *
 * \return  number of integer literals succesfully parsed, can be less than \a num
 */
static int parse_int_args(const char *text, char **endptr, size_t num, int *values)
{
    const char *s = text;
    size_t      n = 0;

    while (n < num) {
        int val = 0;

        s = util_skip_whitespace(s);
        if (!parse_int(s, endptr, &val)) {
            break;
        }

        values[n] = val;
        s = *endptr;
        n++;
    }

    return (int)n;
}

/** \brief  Log parser error using printf-style formatting
 *
 * \param[in]   state   parser state
 * \param[in]   fmt     format string
 * \param[in]   ...     optional arguments to \a fmt
 */
static void parser_log_error(const parser_state_t *state, const char *fmt, ...)
{
    char    message[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(message, sizeof message, fmt, args);
    log_error(joy_log, "%s:%d: %s", state->filename, state->lineno, message);
    va_end(args);
}

/** \brief  Set a host controller to joystick mapping
 *
 * \param[in]   state   parser state
 * \param[in]   mapping mapping to set
 */
static void parser_set_mapping(const parser_state_t *state,
                               joystick_mapping_t   *mapping)

{
    mapping->action = state->action;

    switch (state->action) {
        case JOY_ACTION_NONE:           /* fall through */
        case JOY_ACTION_MAP:            /* fall through */
        case JOY_ACTION_UI_ACTIVATE:
            /* no arguments required */
            break;
        case JOY_ACTION_JOYSTICK:
            mapping->value.joy_pin = state->args.pin;
            break;
        case JOY_ACTION_KEYBOARD:
            mapping->value.key[0] = state->args.key.row;
            mapping->value.key[1] = state->args.key.column;
            mapping->value.key[2] = state->args.key.flags;
            break;
        case JOY_ACTION_UI_FUNCTION:
            mapping->value.ui_action = state->args.ui_action_id;
            break;
        case JOY_ACTION_POT_AXIS:
            /* already handled in axis code, there's no joystick_mapping_t
             * instace used for POT mappings */
            break;
        default:
            parser_log_error(state, "unknown action %d.", state->action);
            break;
    }
}


/** \brief  Set an axis mapping
 *
 * \param[in]   state   parser state
 *
 * \return  \c true on success
 */
static bool parser_set_axis(const parser_state_t *state)
{
    joystick_device_t  *dev = &(joystick_devices[state->joy_index]);
    bool                result = true;

    if (state->action == JOY_ACTION_POT_AXIS) {
        /* map to potentiometer */
        if (state->input_index < dev->num_axes) {
            dev->axis_mapping[state->input_index].pot = state->args.pot;
        } else {
            result = false;
        }
    } else {
        /* map to axis direction */
        int index     = state->input_index / 2;
        int direction = state->input_index % 2;

#if 0
        printf("%s(): AXIS: index = %d, direction = %s\n",
               __func__, index, direction == 0 ? "positive" : "negative");
#endif
        if (index < dev->num_axes) {
            joystick_axis_mapping_t *axis = &(dev->axis_mapping[index]);
            joystick_mapping_t      *mapping;

            /* select directional mapping */
            if (direction == 0) {
                mapping = &(axis->positive_direction);
            } else {
                mapping = &(axis->negative_direction);
            }
            parser_set_mapping(state, mapping);
        } else {
            result = false;
        }
    }

    if (!result) {
        parser_log_error(state,
                         "input index %d too large for input type axis (pot), joystick %d.",
                         state->input_index, state->joy_index);
    }
    return result;
}

/** \brief  Set a button mapping
 *
 * \param[in]   state   parser state
 *
 * \return  \c true on success
 */
static bool parser_set_button(const parser_state_t *state)
{
    joystick_device_t *dev    = &(joystick_devices[state->joy_index]);
    int                button = state->input_index;
    bool               result = true;

    if (button < dev->num_buttons) {
        joystick_mapping_t *mapping = &(dev->button_mapping[button].mapping);

        parser_set_mapping(state, mapping);
    } else {
        parser_log_error(state,
                         "invalid button index %d for joystick %d.",
                         button, state->joy_index);
        result = false;
    }
    return result;
}

/** \brief  Set a hat mapping
 *
 * \param[in]   state   parser state
 *
 * \return  \c true on success
 */
static bool parser_set_hat(const parser_state_t *state)
{
    int index              = state->input_index / 4;
    int direction          = state->input_index % 4;
    joystick_device_t *dev = &(joystick_devices[state->joy_index]);

    if (index < dev->num_hats) {
        joystick_hat_mapping_t *hat = &(dev->hat_mapping[index]);
        joystick_mapping_t     *mapping;

        switch (direction) {
            case JOY_HAT_UP:
                mapping = &(hat->up);
                break;
            case JOY_HAT_DOWN:
                mapping = &(hat->down);
                break;
            case JOY_HAT_LEFT:
                mapping = &(hat->left);
                break;
            case JOY_HAT_RIGHT:
                mapping = &(hat->right);
                break;
            default:
                /* never reached, to satisfy gcc */
                parser_log_error(state, "invalid direction %d for hat.", direction);
                return false;
        }
        parser_set_mapping(state, mapping);
    } else {
        parser_log_error(state, "invalid hat index %d.", index);
        return false;
    }

    return true;
}

/** \brief  Set a ball mapping
 *
 * \param[in]   state   parser state
 *
 * \return  \c true on success
 * \note    Currently unimplemented, logs error and returns \c false.
 */
static bool parser_set_ball(const parser_state_t *state)
{
    parser_log_error(state, "balls are currently not supported.");
    return false;
}

/** \brief  Parse a single line of a joymap file
 *
 * \param[in]   buffer      line of text from file \a filename
 * \param[in]   filename    current joymap file being parsed (for error messages)
 * \param[in]   lineno      current line number in \a filename (for error messages)
 *
 * \return  \c true on success
 */
static bool joy_arch_parse_entry_new(const char *buffer, const char *filename, int lineno)
{
    parser_state_t  state;
    char            action_name[256];
    char           *endptr;
    bool            result = true;
    int             args[4];
    int             key_args[3];
    int             nargs;
    int             itmp;
    size_t          a;


    /* TODO: split parsing and checking mandatory args into separate function */
#if 0
    printf("%s(): parsing '%s'\n", __func__, buffer);
#endif
    nargs = parse_int_args(buffer, &endptr, sizeof args / sizeof args[0], args);

    state.buffer      = buffer;
    state.filename    = filename;
    state.lineno      = lineno;
    state.joy_index   = args[0];
    state.input_type  = (joystick_input_t)args[1];
    state.input_index = args[2];
    state.action      = (joystick_action_t)args[3];
    state.bufptr      = endptr;

    /* check input so far */

    /* joystick index (leading whitespace is already trimmed here) */
    if (nargs < 1) {
        parser_log_error(&state, "missing joystick number.");
        return false;
    }
    if (state.joy_index < 0 || state.joy_index >= num_joystick_devices) {
        parser_log_error(&state, "could not find joystick %d.", state.joy_index);
        return false;
    }

    /* input type */
    if (nargs < 2) {
        parser_log_error(&state, "missing input type.");
        return false;
    }
    if (state.input_type < 0 || state.input_type > JOY_INPUT_MAX) {
        parser_log_error(&state, "invalid input type %d.", (int)state.input_type);
        return false;
    }

    /* input index */
    if (nargs < 3) {
        parser_log_error(&state, "missing input type.");
        return false;
    }
    if (state.input_index < 0) {
        parser_log_error(&state, "input index cannot be negative.");
        return false;
    }

    /* joystick action number */
    if (nargs < 4) {
        parser_log_error(&state, "missing action number.");
        return false;
    }
    if (state.action < JOY_ACTION_NONE || state.action > JOY_ACTION_MAX) {
        parser_log_error(&state, "invalid action number %d.", (int)state.action);
        return false;
    }

    /* four mandatory columns done: */
#if 0
    printf("%s(): joynum = %d, input type = %d, input index = %d, action = %d\n",
           __func__, state.joy_index, (int)state.input_type,
           state.input_index, (int)state.action);
#endif

    /* get additional colums, depending on action */
    /* TODO: split into separate function */
    state.bufptr = util_skip_whitespace(state.bufptr);
    switch (state.action) {
        case JOY_ACTION_JOYSTICK:
            /* joystick pin */
            if (!parse_int(state.bufptr, &endptr, &itmp)) {
                parser_log_error(&state, "missing joystick pin number.");
                return false;
            }
            if (itmp < 0 || itmp > UINT16_MAX) {
                parser_log_error(&state, "pin number %d out of bounds.", itmp);
                return false;
            }
            state.args.pin = (uint16_t)itmp;
            break;

        case JOY_ACTION_KEYBOARD:
            /* emulated keyboard press */
            nargs = parse_int_args(state.bufptr, &endptr, 3, key_args);
            if (nargs < 2) {
                parser_log_error(&state,
                                 "incomplete argument list for key press,"
                                 " got %d argument(s), expected 2 or 3.",
                                 nargs);
                return false;
            }
            state.args.key.row    = key_args[0];
            state.args.key.column = key_args[1];
            state.args.key.flags  = nargs == 3 ? key_args[2] : 0;
            break;

        case JOY_ACTION_MAP:
            /* map controller button to UI action (UNIMPLEMENTED) */
            break;

        case JOY_ACTION_UI_ACTIVATE:
            /* activate the menu (SDL) or show the settings dialog (Gtk3) */
            break;

        case JOY_ACTION_UI_FUNCTION:
            /* trigger UI action with controller */
            a = 0;
            if (state.bufptr[a] == '\0') {
                parser_log_error(&state, "missing UI action name");
                return false;
            }
            if (!isalpha((unsigned char)state.bufptr[a])) {
                parser_log_error(&state, "invalid UI action name: %s", state.bufptr);
                return false;
            }

            action_name[a] = state.bufptr[a];
            a++;
            while (a < sizeof action_name &&
                   state.bufptr[a] != '\0' &&
                   (isalnum((unsigned char)(state.bufptr[a])) ||
                    state.bufptr[a] == '_' ||
                    state.bufptr[a] == '-' ||
                    state.bufptr[a] == ':')) {
                action_name[a] = state.bufptr[a];
                a++;
            }
            action_name[a] = '\0';
            state.args.ui_action_id = ui_action_get_id(action_name);
            if (state.args.ui_action_id <= ACTION_NONE) {
                parser_log_error(&state, "invalid action '%s'", action_name);
                return false;
            }
            break;

        case JOY_ACTION_POT_AXIS:
            /* map axis to potentiometer */
            if (!parse_int(state.bufptr, &endptr, &state.args.pot)) {
                parser_log_error(&state, "missing potentiometer number.");
                return false;
            }
            break;

        default:
            break;
    }

    /* handle axis, button or hat */
    switch (state.input_type) {
        case JOY_INPUT_AXIS:
            result = parser_set_axis(&state);
            break;
        case JOY_INPUT_BUTTON:
            result = parser_set_button(&state);
            break;
        case JOY_INPUT_HAT:
            result = parser_set_hat(&state);
            break;
        case JOY_INPUT_BALL:
            result = parser_set_ball(&state);
            break;
        default:
            parser_log_error(&state,
                             "unsupported input type %u.",
                             (unsigned int)state.input_type);
            result = false;
            break;
    }

    return result;
}
#endif


int joy_arch_mapping_load(const char *filename)
{
    FILE *fp;
    char *complete_path;
    char buffer[1000];
    int lineno = 1;

    /* Silently ignore keymap load on resource & cmdline init */
    if (joystick_devices == NULL) {
        return 0;
    }

    if (filename == NULL) {
        return -1;
    }

    fp = sysfile_open(filename, NULL, &complete_path, MODE_READ_TEXT);

    if (fp == NULL) {
        log_warning(joy_log, "Failed to open `%s'.", filename);
        return -1;
    }

    log_message(joy_log, "Loading joystick map `%s'.", complete_path);

    lib_free(complete_path);

    do {
        buffer[0] = 0;
        if (fgets(buffer, sizeof buffer - 1u, fp)) {
            size_t  len;
            char   *p;

            len = strlen(buffer);
            if (len == 0) {
                break;
            }

            buffer[len - 1u] = 0; /* remove newline */

            /* remove comments */
            /* What if a comment contains '#' (e.g. `# map button to "#"`)? */
            if ((p = strchr(buffer, '#'))) {
                *p = 0;
            }

            /* remove whitespace at the beginning of the line */
            p = buffer;
            while (((*p == ' ') || (*p == '\t')) && (*p != 0)) {
                ++p;
            }

            switch (*p) {
                case 0:
                    break;
                case '!':
                    /* keyword handling */
                    joy_arch_parse_keyword(p);
                    break;
                default:
                    /* table entry handling */
                    /* new func goes first since old one clobbers its argument */
#ifdef NEW_ENTRY_PARSER
                    joy_arch_parse_entry_new(p, filename, lineno);
#else
                    joy_arch_parse_entry(p);
#endif
                    break;
            }

            lineno++;
        }
        /* Should we get an I/O error during fgets(), this will loop forever:
         * fgets() will return NULL but feof(fp) will be false. */
    } while (!feof(fp));
    fclose(fp);

    return 0;
}

static int set_joystick_autofire(int val, void *param)
{
    int port_idx = vice_ptr_to_int(param);

    joystick_autofire_enable[port_idx] = val ? 1 : 0;

    return 0;
}

static int set_joystick_autofire_mode(int val, void *param)
{
    int port_idx = vice_ptr_to_int(param);

    joystick_autofire_mode[port_idx] = val ? JOYSTICK_AUTOFIRE_MODE_PERMANENT : JOYSTICK_AUTOFIRE_MODE_PRESS;

    return 0;
}

static int set_joystick_autofire_speed(int val, void *param)
{
    int port_idx = vice_ptr_to_int(param);

    if ((val < JOYSTICK_AUTOFIRE_SPEED_MIN) || (val > JOYSTICK_AUTOFIRE_SPEED_MAX)) {
        return -1;
    }

    joystick_autofire_speed[port_idx] = val;

    return 0;
}

static int joymap_file_set(const char *val, void *param)
{
    if (util_string_set(&joymap_file, val)) {
        return 0;
    }

    return joy_arch_mapping_load(joymap_file);
}

static const resource_int_t joyopposite_resources_int[] = {
    { "JoyOpposite", 0, RES_EVENT_NO, NULL,
      &joystick_opposite_enable, set_joystick_opposite_enable, NULL },
    RESOURCE_INT_LIST_END
};

static resource_int_t joy1_resources_int[] = {
    { "JoyDevice1", JOYDEV_NONE, RES_EVENT_NO, NULL,
      &joystick_port_map[JOYPORT_1], set_joystick_device, (void *)JOYPORT_1 },
    { "JoyStick1AutoFire", JOYSTICK_AUTOFIRE_OFF, RES_EVENT_NO, NULL,
      &joystick_autofire_enable[JOYPORT_1], set_joystick_autofire, (void *)JOYPORT_1 },
    { "JoyStick1AutoFireMode", JOYSTICK_AUTOFIRE_MODE_PRESS, RES_EVENT_NO, NULL,
      &joystick_autofire_mode[JOYPORT_1], set_joystick_autofire_mode, (void *)JOYPORT_1 },
    { "JoyStick1AutoFireSpeed", JOYSTICK_AUTOFIRE_SPEED_DEFAULT, RES_EVENT_NO, NULL,
      &joystick_autofire_speed[JOYPORT_1], set_joystick_autofire_speed, (void *)JOYPORT_1 },
    RESOURCE_INT_LIST_END
};

static resource_int_t joy2_resources_int[] = {
    { "JoyDevice2", JOYDEV_NONE, RES_EVENT_NO, NULL,
      &joystick_port_map[JOYPORT_2], set_joystick_device, (void *)JOYPORT_2 },
    { "JoyStick2AutoFire", JOYSTICK_AUTOFIRE_OFF, RES_EVENT_NO, NULL,
      &joystick_autofire_enable[JOYPORT_2], set_joystick_autofire, (void *)JOYPORT_2 },
    { "JoyStick2AutoFireMode", JOYSTICK_AUTOFIRE_MODE_PRESS, RES_EVENT_NO, NULL,
      &joystick_autofire_mode[JOYPORT_2], set_joystick_autofire_mode, (void *)JOYPORT_2 },
    { "JoyStick2AutoFireSpeed", JOYSTICK_AUTOFIRE_SPEED_DEFAULT, RES_EVENT_NO, NULL,
      &joystick_autofire_speed[JOYPORT_2], set_joystick_autofire_speed, (void *)JOYPORT_2 },
    RESOURCE_INT_LIST_END
};

static resource_int_t joy3_resources_int[] = {
    { "JoyDevice3", JOYDEV_NONE, RES_EVENT_NO, NULL,
      &joystick_port_map[JOYPORT_3], set_joystick_device, (void *)JOYPORT_3 },
    { "JoyStick3AutoFire", JOYSTICK_AUTOFIRE_OFF, RES_EVENT_NO, NULL,
      &joystick_autofire_enable[JOYPORT_3], set_joystick_autofire, (void *)JOYPORT_3 },
    { "JoyStick3AutoFireMode", JOYSTICK_AUTOFIRE_MODE_PRESS, RES_EVENT_NO, NULL,
      &joystick_autofire_mode[JOYPORT_3], set_joystick_autofire_mode, (void *)JOYPORT_3 },
    { "JoyStick3AutoFireSpeed", JOYSTICK_AUTOFIRE_SPEED_DEFAULT, RES_EVENT_NO, NULL,
      &joystick_autofire_speed[JOYPORT_3], set_joystick_autofire_speed, (void *)JOYPORT_3 },
    RESOURCE_INT_LIST_END
};

static resource_int_t joy4_resources_int[] = {
    { "JoyDevice4", JOYDEV_NONE, RES_EVENT_NO, NULL,
      &joystick_port_map[JOYPORT_4], set_joystick_device, (void *)JOYPORT_4 },
    { "JoyStick4AutoFire", JOYSTICK_AUTOFIRE_OFF, RES_EVENT_NO, NULL,
      &joystick_autofire_enable[JOYPORT_4], set_joystick_autofire, (void *)JOYPORT_4 },
    { "JoyStick4AutoFireMode", JOYSTICK_AUTOFIRE_MODE_PRESS, RES_EVENT_NO, NULL,
      &joystick_autofire_mode[JOYPORT_4], set_joystick_autofire_mode, (void *)JOYPORT_4 },
    { "JoyStick4AutoFireSpeed", JOYSTICK_AUTOFIRE_SPEED_DEFAULT, RES_EVENT_NO, NULL,
      &joystick_autofire_speed[JOYPORT_4], set_joystick_autofire_speed, (void *)JOYPORT_4 },
    RESOURCE_INT_LIST_END
};

static resource_int_t joy5_resources_int[] = {
    { "JoyDevice5", JOYDEV_NONE, RES_EVENT_NO, NULL,
      &joystick_port_map[JOYPORT_5], set_joystick_device, (void *)JOYPORT_5 },
    { "JoyStick5AutoFire", JOYSTICK_AUTOFIRE_OFF, RES_EVENT_NO, NULL,
      &joystick_autofire_enable[JOYPORT_5], set_joystick_autofire, (void *)JOYPORT_5 },
    { "JoyStick5AutoFireMode", JOYSTICK_AUTOFIRE_MODE_PRESS, RES_EVENT_NO, NULL,
      &joystick_autofire_mode[JOYPORT_5], set_joystick_autofire_mode, (void *)JOYPORT_5 },
    { "JoyStick5AutoFireSpeed", JOYSTICK_AUTOFIRE_SPEED_DEFAULT, RES_EVENT_NO, NULL,
      &joystick_autofire_speed[JOYPORT_5], set_joystick_autofire_speed, (void *)JOYPORT_5 },
    RESOURCE_INT_LIST_END
};

static resource_int_t joy6_resources_int[] = {
    { "JoyDevice6", JOYDEV_NONE, RES_EVENT_NO, NULL,
      &joystick_port_map[JOYPORT_6], set_joystick_device, (void *)JOYPORT_6 },
    { "JoyStick6AutoFire", JOYSTICK_AUTOFIRE_OFF, RES_EVENT_NO, NULL,
      &joystick_autofire_enable[JOYPORT_6], set_joystick_autofire, (void *)JOYPORT_6 },
    { "JoyStick6AutoFireMode", JOYSTICK_AUTOFIRE_MODE_PRESS, RES_EVENT_NO, NULL,
      &joystick_autofire_mode[JOYPORT_6], set_joystick_autofire_mode, (void *)JOYPORT_6 },
    { "JoyStick6AutoFireSpeed", JOYSTICK_AUTOFIRE_SPEED_DEFAULT, RES_EVENT_NO, NULL,
      &joystick_autofire_speed[JOYPORT_6], set_joystick_autofire_speed, (void *)JOYPORT_6 },
    RESOURCE_INT_LIST_END
};

static resource_int_t joy7_resources_int[] = {
    { "JoyDevice7", JOYDEV_NONE, RES_EVENT_NO, NULL,
      &joystick_port_map[JOYPORT_7], set_joystick_device, (void *)JOYPORT_7 },
    { "JoyStick7AutoFire", JOYSTICK_AUTOFIRE_OFF, RES_EVENT_NO, NULL,
      &joystick_autofire_enable[JOYPORT_7], set_joystick_autofire, (void *)JOYPORT_7 },
    { "JoyStick7AutoFireMode", JOYSTICK_AUTOFIRE_MODE_PRESS, RES_EVENT_NO, NULL,
      &joystick_autofire_mode[JOYPORT_7], set_joystick_autofire_mode, (void *)JOYPORT_7 },
    { "JoyStick7AutoFireSpeed", JOYSTICK_AUTOFIRE_SPEED_DEFAULT, RES_EVENT_NO, NULL,
      &joystick_autofire_speed[JOYPORT_7], set_joystick_autofire_speed, (void *)JOYPORT_7 },
    RESOURCE_INT_LIST_END
};

static resource_int_t joy8_resources_int[] = {
    { "JoyDevice8", JOYDEV_NONE, RES_EVENT_NO, NULL,
      &joystick_port_map[JOYPORT_8], set_joystick_device, (void *)JOYPORT_8 },
    { "JoyStick8AutoFire", JOYSTICK_AUTOFIRE_OFF, RES_EVENT_NO, NULL,
      &joystick_autofire_enable[JOYPORT_8], set_joystick_autofire, (void *)JOYPORT_8 },
    { "JoyStick8AutoFireMode", JOYSTICK_AUTOFIRE_MODE_PRESS, RES_EVENT_NO, NULL,
      &joystick_autofire_mode[JOYPORT_8], set_joystick_autofire_mode, (void *)JOYPORT_8 },
    { "JoyStick8AutoFireSpeed", JOYSTICK_AUTOFIRE_SPEED_DEFAULT, RES_EVENT_NO, NULL,
      &joystick_autofire_speed[JOYPORT_8], set_joystick_autofire_speed, (void *)JOYPORT_8 },
    RESOURCE_INT_LIST_END
};

static resource_int_t joy9_resources_int[] = {
    { "JoyDevice9", JOYDEV_NONE, RES_EVENT_NO, NULL,
      &joystick_port_map[JOYPORT_9], set_joystick_device, (void *)JOYPORT_9 },
    { "JoyStick9AutoFire", JOYSTICK_AUTOFIRE_OFF, RES_EVENT_NO, NULL,
      &joystick_autofire_enable[JOYPORT_9], set_joystick_autofire, (void *)JOYPORT_9 },
    { "JoyStick9AutoFireMode", JOYSTICK_AUTOFIRE_MODE_PRESS, RES_EVENT_NO, NULL,
      &joystick_autofire_mode[JOYPORT_9], set_joystick_autofire_mode, (void *)JOYPORT_9 },
    { "JoyStick9AutoFireSpeed", JOYSTICK_AUTOFIRE_SPEED_DEFAULT, RES_EVENT_NO, NULL,
      &joystick_autofire_speed[JOYPORT_9], set_joystick_autofire_speed, (void *)JOYPORT_9 },
    RESOURCE_INT_LIST_END
};

static resource_int_t joy10_resources_int[] = {
    { "JoyDevice10", JOYDEV_NONE, RES_EVENT_NO, NULL,
      &joystick_port_map[JOYPORT_10], set_joystick_device, (void *)JOYPORT_10 },
    { "JoyStick10AutoFire", JOYSTICK_AUTOFIRE_OFF, RES_EVENT_NO, NULL,
      &joystick_autofire_enable[JOYPORT_10], set_joystick_autofire, (void *)JOYPORT_10 },
    { "JoyStick10AutoFireMode", JOYSTICK_AUTOFIRE_MODE_PRESS, RES_EVENT_NO, NULL,
      &joystick_autofire_mode[JOYPORT_10], set_joystick_autofire_mode, (void *)JOYPORT_10 },
    { "JoyStick10AutoFireSpeed", JOYSTICK_AUTOFIRE_SPEED_DEFAULT, RES_EVENT_NO, NULL,
      &joystick_autofire_speed[JOYPORT_10], set_joystick_autofire_speed, (void *)JOYPORT_10 },
    RESOURCE_INT_LIST_END
};

static resource_int_t joy11_resources_int[] = {
    { "JoyDevice11", JOYDEV_NONE, RES_EVENT_NO, NULL,
      &joystick_port_map[JOYPORT_11], set_joystick_device, (void *)JOYPORT_11 },
    { "JoyStick11AutoFire", JOYSTICK_AUTOFIRE_OFF, RES_EVENT_NO, NULL,
      &joystick_autofire_enable[JOYPORT_11], set_joystick_autofire, (void *)JOYPORT_11 },
    { "JoyStick11AutoFireMode", JOYSTICK_AUTOFIRE_MODE_PRESS, RES_EVENT_NO, NULL,
      &joystick_autofire_mode[JOYPORT_11], set_joystick_autofire_mode, (void *)JOYPORT_11 },
    { "JoyStick11AutoFireSpeed", JOYSTICK_AUTOFIRE_SPEED_DEFAULT, RES_EVENT_NO, NULL,
      &joystick_autofire_speed[JOYPORT_11], set_joystick_autofire_speed, (void *)JOYPORT_11 },
    RESOURCE_INT_LIST_END
};

static resource_string_t resources_string[] = {
    { "JoyMapFile", NULL, RES_EVENT_NO, NULL,
      &joymap_file, joymap_file_set, (void *)0 },
    RESOURCE_STRING_LIST_END
};


/** \brief  Initialize joystick resources
 *
 * \return  0 on success, -1 on failure
 */
int joystick_resources_init(void)
{
#ifdef COMMON_JOYKEYS
    if (resources_register_int(joykeys_resources_int) < 0) {
        return -1;
    }
#endif

    if (resources_register_int(joyopposite_resources_int) < 0) {
        return -1;
    }

#ifdef JOYDEV_DEFAULT
    switch (machine_class) {
        case VICE_MACHINE_C64:
        case VICE_MACHINE_C64SC:
        case VICE_MACHINE_C128:
        case VICE_MACHINE_C64DTV:
        case VICE_MACHINE_PLUS4:
        case VICE_MACHINE_SCPU64:
            joy2_resources_int[0].factory_value = JOYDEV_DEFAULT;
            break;
        case VICE_MACHINE_VIC20:
        case VICE_MACHINE_CBM5x0:
            joy1_resources_int[0].factory_value = JOYDEV_DEFAULT;
            break;
        case VICE_MACHINE_PET:
        case VICE_MACHINE_CBM6x0:
            break;
        default:
            break;
    }
#endif

    if (joyport_get_port_name(JOYPORT_1)) {
        if (resources_register_int(joy1_resources_int) < 0) {
            return -1;
        }
    }
    if (joyport_get_port_name(JOYPORT_2)) {
        if (resources_register_int(joy2_resources_int) < 0) {
            return -1;
        }
    }
    if (joyport_get_port_name(JOYPORT_3)) {
        if (resources_register_int(joy3_resources_int) < 0) {
            return -1;
        }
    }
    if (joyport_get_port_name(JOYPORT_4)) {
        if (resources_register_int(joy4_resources_int) < 0) {
            return -1;
        }
    }

    if (joyport_get_port_name(JOYPORT_5)) {
        if (resources_register_int(joy5_resources_int) < 0) {
            return -1;
        }
    }

    if (joyport_get_port_name(JOYPORT_6)) {
        if (resources_register_int(joy6_resources_int) < 0) {
            return -1;
        }
    }

    if (joyport_get_port_name(JOYPORT_7)) {
        if (resources_register_int(joy7_resources_int) < 0) {
            return -1;
        }
    }

    if (joyport_get_port_name(JOYPORT_8)) {
        if (resources_register_int(joy8_resources_int) < 0) {
            return -1;
        }
    }

    if (joyport_get_port_name(JOYPORT_9)) {
        if (resources_register_int(joy9_resources_int) < 0) {
            return -1;
        }
    }

    if (joyport_get_port_name(JOYPORT_10)) {
        if (resources_register_int(joy10_resources_int) < 0) {
            return -1;
        }
    }

    if (machine_class == VICE_MACHINE_PLUS4) {
        if (joyport_get_port_name(JOYPORT_11)) {
            if (resources_register_int(joy11_resources_int) < 0) {
                return -1;
            }
        }
    }

    joymap_factory = archdep_default_joymap_file_name();
    resources_string[0].factory_value = joymap_factory;

    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

#ifdef HAVE_SDL_NUMJOYSTICKS
    return joy_sdl_resources_init();
#else
    return 1;
#endif
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] = {
    { "-joyopposite", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyOpposite", (resource_value_t)1,
      NULL, "Enable opposite joystick directions" },
    { "+joyopposite", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyOpposite", (resource_value_t)0,
      NULL, "Disable opposite joystick directions" },
#ifdef COMMON_JOYKEYS
    { "-keyset", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "KeySetEnable", (resource_value_t)1,
      NULL, "Enable keyset" },
    { "+keyset", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "KeySetEnable", (resource_value_t)0,
      NULL, "Disable keyset" },
#endif
    CMDLINE_LIST_END
};

/* Per-joystick command-line options.  */

static const cmdline_option_t joydev1cmdline_options[] = {
    { "-joydev1", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyDevice1", NULL,
#ifdef HAS_USB_JOYSTICK
    "<0-13>", "Set device for native joystick port 1 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5, 10: Digital joystick 0, 11: Digital joystick 1, 12: USB joystick 0, 13: USB joystick 1)" },
#else
    "<0-9>", "Set device for native joystick port 1 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5)" },
#endif
    { "-joystick1autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick1AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_ON,
      NULL, "Enable autofire for joystick/joypad in native joystick port 1" },
    { "+joystick1autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick1AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_OFF,
      NULL, "Disable autofire for joystick/joypad in native joystick port 1" },
    { "-joystick1autofiremode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick1AutoFireMode", NULL,
      "<0-1>", "Set autofire mode for joystick/joypad in native joystick port 1 (0: Autofire when fire button is pressed, 1: Permanently autofire (pressing fire overrides autofire)" },
    { "-joystick1autofirespeed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick1AutoFireSpeed", NULL,
      "<1-255>", "Set autofire speed for joystick/joypad in native joystick port 1 (amount of fire button presses per second)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t joydev1cmdline_vic20_options[] = {
    { "-joydev1", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyDevice1", NULL,
#ifdef HAS_USB_JOYSTICK
    "<0-13>", "Set device for native joystick port (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5, 10: Digital joystick 0, 11: Digital joystick 1, 12: USB joystick 0, 13: USB joystick 1)" },
#else
    "<0-9>", "Set device for native joystick port (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5)" },
#endif
    { "-joystick1autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick1AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_ON,
      NULL, "Enable autofire for joystick/joypad in native joystick port" },
    { "+joystick1autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick1AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_OFF,
      NULL, "Disable autofire for joystick/joypad in native joystick port" },
    { "-joystick1autofiremode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick1AutoFireMode", NULL,
      "<0-1>", "Set autofire mode for joystick/joypad in native joystick port (0: Autofire when fire button is pressed, 1: Permanently autofire (pressing fire overrides autofire)" },
    { "-joystick1autofirespeed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick1AutoFireSpeed", NULL,
      "<1-255>", "Set autofire speed for joystick/joypad in native joystick port (amount of fire button presses per second)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t joydev2cmdline_options[] = {
    { "-joydev2", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyDevice2", NULL,
#ifdef HAS_USB_JOYSTICK
    "<0-13>", "Set device for native joystick port 2 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5, 10: Digital joystick 0, 11: Digital joystick 1, 12: USB joystick 0, 13: USB joystick 1)" },
#else
    "<0-9>", "Set device for native joystick port 2 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5)" },
#endif
    { "-joystick2autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick2AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_ON,
      NULL, "Enable autofire for joystick/joypad in native joystick port 2" },
    { "+joystick2autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick2AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_OFF,
      NULL, "Disable autofire for joystick/joypad in native joystick port 2" },
    { "-joystick2autofiremode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick2AutoFireMode", NULL,
      "<0-1>", "Set autofire mode for joystick/joypad in native joystick port 2 (0: Autofire when fire button is pressed, 1: Permanently autofire (pressing fire overrides autofire)" },
    { "-joystick2autofirespeed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick2AutoFireSpeed", NULL,
      "<1-255>", "Set autofire speed for joystick/joypad in native joystick port 2 (amount of fire button presses per second)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t joydev3cmdline_options[] = {
    { "-extrajoydev1", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyDevice3", NULL,

#ifdef HAS_USB_JOYSTICK
    "<0-13>", "Set device for joystick adapter port 1 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5, 10: Digital joystick 0, 11: Digital joystick 1, 12: USB joystick 0, 13: USB joystick 1)" },
#else
    "<0-9>", "Set device for joystick adapter port 1 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5)" },
#endif
    { "-extrajoystick1autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick3AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_ON,
      NULL, "Enable autofire for joystick/joypad in joystick adapter port 1" },
    { "+extrajoystick1autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick3AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_OFF,
      NULL, "Disable autofire for joystick/joypad in joystick adapter port 1" },
    { "-extrajoystick1autofiremode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick3AutoFireMode", NULL,
      "<0-1>", "Set autofire mode for joystick/joypad in joystick adapter port 1 (0: Autofire when fire button is pressed, 1: Permanently autofire (pressing fire overrides autofire)" },
    { "-extrajoystick1autofirespeed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick3AutoFireSpeed", NULL,
      "<1-255>", "Set autofire speed for joystick/joypad in joystick adapter port 1 (amount of fire button presses per second)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t joydev4cmdline_options[] = {
    { "-extrajoydev2", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyDevice4", NULL,
#ifdef HAS_USB_JOYSTICK
    "<0-13>", "Set device for joystick adapter port 2 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5, 10: Digital joystick 0, 11: Digital joystick 1, 12: USB joystick 0, 13: USB joystick 1)" },
#else
    "<0-9>", "Set device for joystick adapter port 2 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5)" },
#endif
    { "-extrajoystick2autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick4AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_ON,
      NULL, "Enable autofire for joystick/joypad in joystick adapter port 2" },
    { "+extrajoystick2autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick4AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_OFF,
      NULL, "Disable autofire for joystick/joypad in joystick adapter port 2" },
    { "-extrajoystick2autofiremode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick4AutoFireMode", NULL,
      "<0-1>", "Set autofire mode for joystick/joypad in joystick adapter port 2 (0: Autofire when fire button is pressed, 1: Permanently autofire (pressing fire overrides autofire)" },
    { "-extrajoystick2autofirespeed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick4AutoFireSpeed", NULL,
      "<1-255>", "Set autofire speed for joystick/joypad in joystick adapter port 2 (amount of fire button presses per second)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t joydev5cmdline_options[] = {
    { "-extrajoydev3", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyDevice5", NULL,
#ifdef HAS_USB_JOYSTICK
    "<0-13>", "Set device for joystick adapter port 3 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5, 10: Digital joystick 0, 11: Digital joystick 1, 12: USB joystick 0, 13: USB joystick 1)" },
#else
    "<0-9>", "Set device for joystick adapter port 3 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5)" },
#endif
    { "-extrajoystick3autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick5AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_ON,
      NULL, "Enable autofire for joystick/joypad in joystick adapter port 3" },
    { "+extrajoystick3autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick5AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_OFF,
      NULL, "Disable autofire for joystick/joypad in joystick adapter port 3" },
    { "-extrajoystick3autofiremode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick5AutoFireMode", NULL,
      "<0-1>", "Set autofire mode for joystick/joypad in joystick adapter port 3 (0: Autofire when fire button is pressed, 1: Permanently autofire (pressing fire overrides autofire)" },
    { "-extrajoystick3autofirespeed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick5AutoFireSpeed", NULL,
      "<1-255>", "Set autofire speed for joystick/joypad in joystick adapter port 3 (amount of fire button presses per second)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t joydev5cmdline_plus4_options[] = {
    { "-extrajoydev3", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyDevice5", NULL,
#ifdef HAS_USB_JOYSTICK
    "<0-13>", "Set device for sidcart joystick port (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5, 10: Digital joystick 0, 11: Digital joystick 1, 12: USB joystick 0, 13: USB joystick 1)" },
#else
    "<0-9>", "Set device for sidcart joystick port (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5)" },
#endif
    { "-extrajoystick3autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick5AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_ON,
      NULL, "Enable autofire for joystick/joypad in sidcart joystick port" },
    { "+extrajoystick3autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick5AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_OFF,
      NULL, "Disable autofire for joystick/joypad in sidcart joystick port" },
    { "-extrajoystick3autofiremode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick5AutoFireMode", NULL,
      "<0-1>", "Set autofire mode for joystick/joypad in sidcart joystick port (0: Autofire when fire button is pressed, 1: Permanently autofire (pressing fire overrides autofire)" },
    { "-extrajoystick3autofirespeed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick5AutoFireSpeed", NULL,
      "<1-255>", "Set autofire speed for joystick/joypad in sidcart joystick port (amount of fire button presses per second)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t joydev6cmdline_options[] = {
    { "-extrajoydev4", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyDevice6", NULL,
#ifdef HAS_USB_JOYSTICK
    "<0-13>", "Set device for joystick adapter port 4 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5, 10: Digital joystick 0, 11: Digital joystick 1, 12: USB joystick 0, 13: USB joystick 1)" },
#else
    "<0-9>", "Set device for joystick adapter port 4 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5)" },
#endif
    { "-extrajoystick4autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick6AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_ON,
      NULL, "Enable autofire for joystick/joypad in joystick adapter port 4" },
    { "+extrajoystick4autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick6AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_OFF,
      NULL, "Disable autofire for joystick/joypad in joystick adapter port 4" },
    { "-extrajoystick4autofiremode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick6AutoFireMode", NULL,
      "<0-1>", "Set autofire mode for joystick/joypad in joystick adapter port 4 (0: Autofire when fire button is pressed, 1: Permanently autofire (pressing fire overrides autofire)" },
    { "-extrajoystick4autofirespeed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick6AutoFireSpeed", NULL,
      "<1-255>", "Set autofire speed for joystick/joypad in joystick adapter port 4 (amount of fire button presses per second)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t joydev7cmdline_options[] = {
    { "-extrajoydev5", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyDevice7", NULL,
#ifdef HAS_USB_JOYSTICK
    "<0-13>", "Set device for joystick adapter port 5 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5, 10: Digital joystick 0, 11: Digital joystick 1, 12: USB joystick 0, 13: USB joystick 1)" },
#else
    "<0-9>", "Set device for joystick adapter port 5 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5)" },
#endif
    { "-extrajoystick5autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick7AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_ON,
      NULL, "Enable autofire for joystick/joypad in joystick adapter port 5" },
    { "+extrajoystick5autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick7AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_OFF,
      NULL, "Disable autofire for joystick/joypad in joystick adapter port 5" },
    { "-extrajoystick5autofiremode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick7AutoFireMode", NULL,
      "<0-1>", "Set autofire mode for joystick/joypad in joystick adapter port 5 (0: Autofire when fire button is pressed, 1: Permanently autofire (pressing fire overrides autofire)" },
    { "-extrajoystick5autofirespeed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick7AutoFireSpeed", NULL,
      "<1-255>", "Set autofire speed for joystick/joypad in joystick adapter port 5 (amount of fire button presses per second)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t joydev8cmdline_options[] = {
    { "-extrajoydev6", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyDevice8", NULL,
#ifdef HAS_USB_JOYSTICK
    "<0-13>", "Set device for joystick adapter port 6 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5, 10: Digital joystick 0, 11: Digital joystick 1, 12: USB joystick 0, 13: USB joystick 1)" },
#else
    "<0-9>", "Set device for joystick adapter port 6 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5)" },
#endif
    { "-extrajoystick6autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick8AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_ON,
      NULL, "Enable autofire for joystick/joypad in joystick adapter port 6" },
    { "+extrajoystick6autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick8AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_OFF,
      NULL, "Disable autofire for joystick/joypad in joystick adapter port 6" },
    { "-extrajoystick6autofiremode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick8AutoFireMode", NULL,
      "<0-1>", "Set autofire mode for joystick/joypad in joystick adapter port 6 (0: Autofire when fire button is pressed, 1: Permanently autofire (pressing fire overrides autofire)" },
    { "-extrajoystick6autofirespeed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick8AutoFireSpeed", NULL,
      "<1-255>", "Set autofire speed for joystick/joypad in joystick adapter port 6 (amount of fire button presses per second)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t joydev9cmdline_options[] = {
    { "-extrajoydev7", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyDevice9", NULL,
#ifdef HAS_USB_JOYSTICK
    "<0-13>", "Set device for joystick adapter port 7 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5, 10: Digital joystick 0, 11: Digital joystick 1, 12: USB joystick 0, 13: USB joystick 1)" },
#else
    "<0-9>", "Set device for joystick adapter port 7 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5)" },
#endif
    { "-extrajoystick7autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick9AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_ON,
      NULL, "Enable autofire for joystick/joypad in joystick adapter port 7" },
    { "+extrajoystick7autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick9AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_OFF,
      NULL, "Disable autofire for joystick/joypad in joystick adapter port 7" },
    { "-extrajoystick7autofiremode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick9AutoFireMode", NULL,
      "<0-1>", "Set autofire mode for joystick/joypad in joystick adapter port 7 (0: Autofire when fire button is pressed, 1: Permanently autofire (pressing fire overrides autofire)" },
    { "-extrajoystick7autofirespeed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick9AutoFireSpeed", NULL,
      "<1-255>", "Set autofire speed for joystick/joypad in joystick adapter port 7 (amount of fire button presses per second)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t joydev10cmdline_options[] = {
    { "-extrajoydev8", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyDevice10", NULL,
#ifdef HAS_USB_JOYSTICK
    "<0-13>", "Set device for joystick adapter port 8 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5, 10: Digital joystick 0, 11: Digital joystick 1, 12: USB joystick 0, 13: USB joystick 1)" },
#else
    "<0-9>", "Set device for joystick adapter port 8 (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5)" },
#endif
    { "-extrajoystick8autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick10AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_ON,
      NULL, "Enable autofire for joystick/joypad in joystick adapter port 8" },
    { "+extrajoystick8autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick10AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_OFF,
      NULL, "Disable autofire for joystick/joypad in joystick adapter port 8" },
    { "-extrajoystick8autofiremode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick10AutoFireMode", NULL,
      "<0-1>", "Set autofire mode for joystick/joypad in joystick adapter port 8 (0: Autofire when fire button is pressed, 1: Permanently autofire (pressing fire overrides autofire)" },
    { "-extrajoystick8autofirespeed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick10AutoFireSpeed", NULL,
      "<1-255>", "Set autofire speed for joystick/joypad in joystick adapter port 8 (amount of fire button presses per second)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t joydev11cmdline_options[] = {
    { "-extrajoydev9", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyDevice11", NULL,
#ifdef HAS_USB_JOYSTICK
    "<0-13>", "Set device for sidcart joystick port (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5, 10: Digital joystick 0, 11: Digital joystick 1, 12: USB joystick 0, 13: USB joystick 1)" },
#else
    "<0-9>", "Set device for sidcart joystick port (0: None, 1: Numpad, 2: Keyset 1, 3: Keyset 2, 4: Analog joystick 0, 5: Analog joystick 1, 6: Analog joystick 2, 7: Analog joystick 3, 8: Analog joystick 4, 9: Analog joystick 5)" },
#endif
    { "-extrajoystick9autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick11AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_ON,
      NULL, "Enable autofire for joystick/joypad in sidcart joystick port" },
    { "+extrajoystick9autofire", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyStick11AutoFire", (resource_value_t)JOYSTICK_AUTOFIRE_OFF,
      NULL, "Disable autofire for joystick/joypad in sidcart joystick port" },
    { "-extrajoystick9autofiremode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick11AutoFireMode", NULL,
      "<0-1>", "Set autofire mode for joystick/joypad in sidcart joystick port (0: Autofire when fire button is pressed, 1: Permanently autofire (pressing fire overrides autofire)" },
    { "-extrajoystick9autofirespeed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyStick11AutoFireSpeed", NULL,
      "<1-255>", "Set autofire speed for joystick/joypad in sidcart joystick port (amount of fire button presses per second)" },
    CMDLINE_LIST_END
};

/** \brief  Initialize joystick command line options
 *
 * \return  0 on success, -1 on failure
 */
int joystick_cmdline_options_init(void)
{
    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }
    if (joyport_get_port_name(JOYPORT_1)) {
        if (machine_class == VICE_MACHINE_VIC20) {
            if (cmdline_register_options(joydev1cmdline_vic20_options) < 0) {
                return -1;
            }
        } else {
            if (cmdline_register_options(joydev1cmdline_options) < 0) {
                return -1;
            }
        }
    }
    if (joyport_get_port_name(JOYPORT_2)) {
        if (cmdline_register_options(joydev2cmdline_options) < 0) {
            return -1;
        }
    }
    if (joyport_get_port_name(JOYPORT_3)) {
        if (cmdline_register_options(joydev3cmdline_options) < 0) {
            return -1;
        }
    }
    if (joyport_get_port_name(JOYPORT_4)) {
        if (cmdline_register_options(joydev4cmdline_options) < 0) {
            return -1;
        }
    }
    if (joyport_get_port_name(JOYPORT_5)) {
        if (machine_class == VICE_MACHINE_PLUS4) {
            if (cmdline_register_options(joydev5cmdline_plus4_options) < 0) {
                return -1;
            }
        } else {
            if (cmdline_register_options(joydev5cmdline_options) < 0) {
                return -1;
            }
        }
    }
    if (joyport_get_port_name(JOYPORT_6)) {
        if (cmdline_register_options(joydev6cmdline_options) < 0) {
            return -1;
        }
    }
    if (joyport_get_port_name(JOYPORT_7)) {
        if (cmdline_register_options(joydev7cmdline_options) < 0) {
            return -1;
        }
    }
    if (joyport_get_port_name(JOYPORT_8)) {
        if (cmdline_register_options(joydev8cmdline_options) < 0) {
            return -1;
        }
    }
    if (joyport_get_port_name(JOYPORT_9)) {
        if (cmdline_register_options(joydev9cmdline_options) < 0) {
            return -1;
        }
    }
    if (joyport_get_port_name(JOYPORT_10)) {
        if (cmdline_register_options(joydev10cmdline_options) < 0) {
            return -1;
        }
    }

    if (machine_class == VICE_MACHINE_PLUS4) {
        if (joyport_get_port_name(JOYPORT_11)) {
            if (cmdline_register_options(joydev11cmdline_options) < 0) {
                return -1;
            }
        }
    }

#ifdef HAVE_SDL_NUMJOYSTICKS
    return joy_sdl_cmdline_options_init();
#else
    return 1;
#endif
}

/*--------------------------------------------------------------------------*/

int joystick_init(void)
{
    joystick_alarm = alarm_new(maincpu_alarm_context, "Joystick",
                               joystick_latch_handler, NULL);

#ifdef COMMON_JOYKEYS
    kbd_initialize_numpad_joykeys(joykeys[0]);
#endif

#ifdef LINUX_JOYSTICK
    linux_joystick_init();
#elif defined HAS_USB_JOYSTICK
    usb_joystick_init();
#elif defined MAC_JOYSTICK
    joy_hidlib_init();
#elif defined HAVE_DINPUT
    if (win32_directinput_joystick_init()) {
    }
#elif defined HAVE_SDL_NUMJOYSTICKS
    joy_sdl_init();
#endif

    int i;
    for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
        if (joystick_port_map[i] >= JOYDEV_REALJOYSTICK_MIN) {
            if (joystick_port_map[i] - JOYDEV_REALJOYSTICK_MIN < num_joystick_devices) {
                joystick_devices[joystick_port_map[i] - JOYDEV_REALJOYSTICK_MIN].joyport = i;
            } else {
                joystick_port_map[i] = JOYDEV_NONE;
            }
        }
    }

    /* do not load joymap file when -default was passed on the command line */
    if (!default_settings_requested) {
        joy_arch_mapping_load(joymap_file);
    }

    return 1;
}

/*--------------------------------------------------------------------------*/

#define DUMP_VER_MAJOR   1
#define DUMP_VER_MINOR   2

static int joystick_snapshot_write_module(snapshot_t *s, int port)
{
    snapshot_module_t *m;
    char snapshot_name[16];

    sprintf(snapshot_name, "JOYSTICK%d", port);

    m = snapshot_module_create(s, snapshot_name, DUMP_VER_MAJOR, DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (SMW_W(m, joystick_value[port]) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

static int joystick_snapshot_read_module(snapshot_t *s, int port)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;
    char snapshot_name[16];

    sprintf(snapshot_name, "JOYSTICK%d", port);

    m = snapshot_module_open(s, snapshot_name, &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (!snapshot_version_is_equal(major_version, minor_version, DUMP_VER_MAJOR, DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (SMR_W(m, &joystick_value[port + 1]) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

/* ------------------------------------------------------------------------- */


/* Array used to avoid "stuck" pins and pins being released too soon when two
 * or more physical (host) buttons are mapped to the same emulated pins.
 *
 * FIXME: The name of this array and the functions handling it are a misnomer:
 * this code is always used, not just for Gtk.
 * A better solution would be to handle many-to-one mappings in the host side
 * code: activate pin when *any* of the host buttons are pressed, deactive pin
 * when *all* of the host buttons are released.
 */
static int gtkjoy_pins[JOYPORT_MAX_PORTS][JOYPORT_MAX_PINS];

void register_joystick_driver(
    struct joystick_driver_s *driver,
    const char *jname,
    void *priv,
    int num_axes,
    int num_buttons,
    int num_hats)
{
    struct joystick_device_s *new_joystick_device;
    int n;

    joystick_devices = lib_realloc(joystick_devices,
            sizeof(struct joystick_device_s) * (num_joystick_devices + 1));
    new_joystick_device = &joystick_devices[num_joystick_devices++];
    new_joystick_device->driver = driver;
    strncpy(new_joystick_device->jname, jname, JOYDEV_NAME_SIZE - 1);
    new_joystick_device->jname[JOYDEV_NAME_SIZE - 1] = '\0';
    new_joystick_device->num_axes = num_axes;
    new_joystick_device->num_hats = num_hats;
    new_joystick_device->num_buttons = num_buttons;

    log_message(LOG_DEFAULT, "registered controller '%s' with %d axes, %d hats, %d buttons",
                new_joystick_device->jname, num_axes, num_hats, num_buttons);

    new_joystick_device->axis_mapping = lib_calloc(num_axes, sizeof *(new_joystick_device->axis_mapping));
    new_joystick_device->button_mapping = lib_calloc(num_buttons, sizeof *(new_joystick_device->button_mapping));
    new_joystick_device->hat_mapping = lib_calloc(num_hats, sizeof *(new_joystick_device->hat_mapping));

    new_joystick_device->joyport = -1;
    new_joystick_device->priv = priv;

    /* now create a default mapping, using the following logic:
     * - map all hats to joystick directions
     * - if the controller has at least two axes, map first two axis to joystick directions
     * - if the controller has exactly four axes, map the next two to joystick directions too
     * - if the controller has six or more axes, map axis 3 and 4 to joystick directions
     *   (this makes the second analog stick on ps3/4 controllers work)
     * - if the controller has eight or more axes, map axis 6 and 7 to joystick directions
     *   (this makes the dpad on ps5 pad work)
     * - if the controller has no axes nor hats, use the first four buttons for the
     *   joystick directions
     * - all remaining buttons are mapped to fire buttons. the second next to fire button 2
     *   and the third next to button 3.
     */
    if (num_hats > 0) {
        for (n = 0; n < num_hats; n++) {
            new_joystick_device->hat_mapping[n].up.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->hat_mapping[n].up.value.joy_pin = JOYSTICK_DIRECTION_UP;
            new_joystick_device->hat_mapping[n].down.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->hat_mapping[n].down.value.joy_pin = JOYSTICK_DIRECTION_DOWN;
            new_joystick_device->hat_mapping[n].left.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->hat_mapping[n].left.value.joy_pin = JOYSTICK_DIRECTION_LEFT;
            new_joystick_device->hat_mapping[n].right.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->hat_mapping[n].right.value.joy_pin = JOYSTICK_DIRECTION_RIGHT;
        }
    }
    if (num_axes > 1) {
        /* first two axes */
        new_joystick_device->axis_mapping[0].positive_direction.action = JOY_ACTION_JOYSTICK;
        new_joystick_device->axis_mapping[0].positive_direction.value.joy_pin = JOYSTICK_DIRECTION_RIGHT;
        new_joystick_device->axis_mapping[0].negative_direction.action = JOY_ACTION_JOYSTICK;
        new_joystick_device->axis_mapping[0].negative_direction.value.joy_pin = JOYSTICK_DIRECTION_LEFT;
        new_joystick_device->axis_mapping[1].positive_direction.action = JOY_ACTION_JOYSTICK;
        new_joystick_device->axis_mapping[1].positive_direction.value.joy_pin = JOYSTICK_DIRECTION_DOWN;
        new_joystick_device->axis_mapping[1].negative_direction.action = JOY_ACTION_JOYSTICK;
        new_joystick_device->axis_mapping[1].negative_direction.value.joy_pin = JOYSTICK_DIRECTION_UP;

#if !defined(MACOS_COMPILE)
        if (num_axes == 4) {
            /* next two axes */
            /* CAUTION: make sure to not map axes 2 and/or 5 for pads with > 4 axes, those are
                        the analog triggers on ps3/ps4 pads and their neutral position is at
                        one extreme of the axis, resulting in some direction being pressed all the time */
#else
        if (num_axes >= 4) {
            /* the caution above does not apply to macOS. */
#endif
            new_joystick_device->axis_mapping[2].positive_direction.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->axis_mapping[2].positive_direction.value.joy_pin = JOYSTICK_DIRECTION_RIGHT;
            new_joystick_device->axis_mapping[2].negative_direction.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->axis_mapping[2].negative_direction.value.joy_pin = JOYSTICK_DIRECTION_LEFT;
            new_joystick_device->axis_mapping[3].positive_direction.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->axis_mapping[3].positive_direction.value.joy_pin = JOYSTICK_DIRECTION_DOWN;
            new_joystick_device->axis_mapping[3].negative_direction.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->axis_mapping[3].negative_direction.value.joy_pin = JOYSTICK_DIRECTION_UP;
        }
#if defined(UNIX_COMPILE) && !defined(MACOS_COMPILE)
        /* CAUTION: this does not work correctly with the current windows joystick code */
        if (num_axes >= 6) {
            /* next two axes (eg second analog stick on ps3/ps4 pads) */
            new_joystick_device->axis_mapping[3].positive_direction.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->axis_mapping[3].positive_direction.value.joy_pin = JOYSTICK_DIRECTION_RIGHT;
            new_joystick_device->axis_mapping[3].negative_direction.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->axis_mapping[3].negative_direction.value.joy_pin = JOYSTICK_DIRECTION_LEFT;
            new_joystick_device->axis_mapping[4].positive_direction.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->axis_mapping[4].positive_direction.value.joy_pin = JOYSTICK_DIRECTION_DOWN;
            new_joystick_device->axis_mapping[4].negative_direction.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->axis_mapping[4].negative_direction.value.joy_pin = JOYSTICK_DIRECTION_UP;
        }
        if (num_axes >= 8) {
            /* next two axes (dpad on ps5 pad) */
            new_joystick_device->axis_mapping[6].positive_direction.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->axis_mapping[6].positive_direction.value.joy_pin = JOYSTICK_DIRECTION_RIGHT;
            new_joystick_device->axis_mapping[6].negative_direction.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->axis_mapping[6].negative_direction.value.joy_pin = JOYSTICK_DIRECTION_LEFT;
            new_joystick_device->axis_mapping[7].positive_direction.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->axis_mapping[7].positive_direction.value.joy_pin = JOYSTICK_DIRECTION_DOWN;
            new_joystick_device->axis_mapping[7].negative_direction.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->axis_mapping[7].negative_direction.value.joy_pin = JOYSTICK_DIRECTION_UP;
        }
#endif
    }

    n = 0;
    if ((num_hats == 0) && (num_axes == 0)) {
        if (num_buttons > 3) {
            /* FIXME: find a common adajoystick_devicespter or controller to use as a reference */
            new_joystick_device->button_mapping[0].mapping.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->button_mapping[0].mapping.value.joy_pin = 1;
            new_joystick_device->button_mapping[1].mapping.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->button_mapping[1].mapping.value.joy_pin = 2;
            new_joystick_device->button_mapping[2].mapping.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->button_mapping[2].mapping.value.joy_pin = 4;
            new_joystick_device->button_mapping[3].mapping.action = JOY_ACTION_JOYSTICK;
            new_joystick_device->button_mapping[3].mapping.value.joy_pin = 8;
            n = 4;
        }
    }
    if (num_buttons > n) {
        new_joystick_device->button_mapping[n].mapping.action = JOY_ACTION_JOYSTICK;
        new_joystick_device->button_mapping[n].mapping.value.joy_pin = 16;
        n++;
    }
    if (num_buttons > n) {
        new_joystick_device->button_mapping[n].mapping.action = JOY_ACTION_JOYSTICK;
        new_joystick_device->button_mapping[n].mapping.value.joy_pin = 32;
        n++;
    }
    if (num_buttons > n) {
        new_joystick_device->button_mapping[n].mapping.action = JOY_ACTION_JOYSTICK;
        new_joystick_device->button_mapping[n].mapping.value.joy_pin = 64;
        n++;
    }
    for ( ; n < num_buttons; n++) {
        switch (n & 3) {
            case 0:
            case 3:
            default:
                new_joystick_device->button_mapping[n].mapping.action = JOY_ACTION_JOYSTICK;
                new_joystick_device->button_mapping[n].mapping.value.joy_pin = 16;
                break;
            case 1:
                new_joystick_device->button_mapping[n].mapping.action = JOY_ACTION_UI_ACTIVATE;
                break;
            case 2:
                new_joystick_device->button_mapping[n].mapping.action = JOY_ACTION_MAP;
                break;
        }
    }
    memset(gtkjoy_pins, 0, sizeof(int) * JOYPORT_MAX_PORTS * JOYPORT_MAX_PINS);
#if 0 /* for testing */
    new_joystick_device->button_mapping[0].action = JOY_ACTION_KEYBOARD;
    new_joystick_device->button_mapping[0].value.key[0] = 2; /* row */
    new_joystick_device->button_mapping[0].value.key[1] = 7; /* column */
#endif
#if 0 /* for testing */ /* FIXME */
    new_joystick_device->button_mapping[0].action = MENUACTION;
    new_joystick_device->button_mapping[0].value.action = 2;
#endif
}

/* When a host joystick event happens that cause a 'press' of a pin, increment the 'press amount' of that pin */
static void gtkjoy_set_value_press(unsigned int joyport, uint16_t value)
{
    int i;

    for (i = 0; i < JOYPORT_MAX_PINS; i++) {
        if (value & (1 << i)) {
            gtkjoy_pins[joyport][i]++;
        }
    }
    joystick_set_value_or(joyport, value);
}

/* When a host joystick event happens that cause a 'release' of a pin, decrement the 'press amount' of that pin,
   and only release the pin for real if the 'press amount' is 0 */
static void gtkjoy_set_value_release(unsigned int joyport, uint16_t value)
{
    int i;

    for (i = 0; i < JOYPORT_MAX_PINS; i++) {
        if (value & (1 << i)) {
            if (gtkjoy_pins[joyport][i] > 0) {
                gtkjoy_pins[joyport][i]--;
            }
            if (gtkjoy_pins[joyport][i] == 0) {
                joystick_set_value_and(joyport, ~value);
            }
        }
    }
}

static void joy_perform_event(joystick_mapping_t *event, int joyport, int value)
{
    switch (event->action) {
        case JOY_ACTION_JOYSTICK:
            DBG(("joy_perform_event (JOY_ACTION_JOYSTICK) joyport: %d value: %d pin: %02x\n",
                 joyport, value, event->value.joy_pin));
            if (joyport >=0 && joyport < JOYPORT_MAX_PORTS) {
                if (value) {
                    gtkjoy_set_value_press(joyport, event->value.joy_pin);
                } else {
                    gtkjoy_set_value_release(joyport, event->value.joy_pin);
                }
            }
            break;
        case JOY_ACTION_KEYBOARD:
            DBG(("joy_perform_event (JOY_ACTION_KEYBOARD) joyport: %d value: %d key: %02x/%02x\n",
                 joyport, value, (unsigned int)event->value.key[0], (unsigned int)event->value.key[1]));
            keyboard_set_keyarr_any(event->value.key[0], event->value.key[1], value);
            break;
        case JOY_ACTION_UI_ACTIVATE:
            if (value) {
                arch_ui_activate();
            }
            break;
        case JOY_ACTION_UI_FUNCTION:
            if (value && event->value.ui_action > ACTION_NONE) {
                ui_action_trigger(event->value.ui_action);
            }
#if 0   /* FIXME */
        case MENUACTION:
            DBG(("joy_perform_event (MENUACTION) joyport: %d value: %d action: %d\n",
                 joyport, value,event->value.action));
            break;
#endif
        case JOY_ACTION_NONE:
        default:
            break;
    }
}


void joy_axis_event(uint8_t joynum, uint8_t axis, joystick_axis_value_t value)
{
    joystick_axis_value_t prev = joystick_devices[joynum].axis_mapping[axis].prev;
    int joyport = joystick_devices[joynum].joyport;

    if (value == prev) {
        return;
    }

    DBG(("joy_axis_event: joynum: %d axis: %d value: %u prev: %u\n", joynum, axis, value, prev));

    /* release directions first if needed */
    if (prev == JOY_AXIS_POSITIVE) {
        joy_perform_event(&joystick_devices[joynum].axis_mapping[axis].positive_direction, joyport, 0);
    }
    if (prev == JOY_AXIS_NEGATIVE) {
        joy_perform_event(&joystick_devices[joynum].axis_mapping[axis].negative_direction, joyport, 0);
    }

    /* press new direction if needed */
    if (value == JOY_AXIS_POSITIVE) {
        joy_perform_event(&joystick_devices[joynum].axis_mapping[axis].positive_direction, joyport, 1);
    }
    if (value == JOY_AXIS_NEGATIVE) {
        joy_perform_event(&joystick_devices[joynum].axis_mapping[axis].negative_direction, joyport, 1);
    }

    joystick_devices[joynum].axis_mapping[axis].prev = value;
}

void joy_button_event(uint8_t joynum, uint8_t button, uint8_t value)
{
    int pressed = value ? 1 : 0;
#if 0
    int num_buttons = joystick_devices[joynum].num_buttons;
    int joy_pin = joystick_devices[joynum].button_mapping[button].value.joy_pin;
    int n;
    /* combine state of all controller buttons that are mapped to the same
       joystick pin */
    joystick_devices[joynum].button_mapping[button].prev = pressed ? 1 : 0;
    for (n = 0 ; n < num_buttons; n++) {
        if (joystick_devices[joynum].button_mapping[n].value.joy_pin == joy_pin) {
            pressed |= joystick_devices[joynum].button_mapping[n].prev;
        }
    }
#endif
    if (pressed != joystick_devices[joynum].button_mapping[button].prev) {
        DBG(("joy_button_event: joynum: %d, button: %d pressed: %d\n",
             joynum, button, pressed));
        joy_perform_event(&(joystick_devices[joynum].button_mapping[button].mapping),
                          joystick_devices[joynum].joyport, pressed);
        joystick_devices[joynum].button_mapping[button].prev = pressed;
    }
}

void joy_hat_event(uint8_t joynum, uint8_t hat, uint8_t value)
{
    uint8_t prev;
    int joyport;

    prev = joystick_devices[joynum].hat_mapping[hat].prev;
    if (value == prev) {
        return;
    }

    joyport = joystick_devices[joynum].joyport;
    DBG(("joy_hat_event:\n"));
    /* release directions first if needed */
    if (prev & JOYSTICK_DIRECTION_UP && !(value & JOYSTICK_DIRECTION_UP)) {
        joy_perform_event(&joystick_devices[joynum].hat_mapping[hat].up, joyport, 0);
    }
    if (prev & JOYSTICK_DIRECTION_DOWN && !(value & JOYSTICK_DIRECTION_DOWN)) {
        joy_perform_event(&joystick_devices[joynum].hat_mapping[hat].down, joyport, 0);
    }
    if (prev & JOYSTICK_DIRECTION_LEFT && !(value & JOYSTICK_DIRECTION_LEFT)) {
        joy_perform_event(&joystick_devices[joynum].hat_mapping[hat].left, joyport, 0);
    }
    if (prev & JOYSTICK_DIRECTION_RIGHT && !(value & JOYSTICK_DIRECTION_RIGHT)) {
        joy_perform_event(&joystick_devices[joynum].hat_mapping[hat].right, joyport, 0);
    }

    /* press new direction if needed */
    if (!(prev & JOYSTICK_DIRECTION_UP) && value & JOYSTICK_DIRECTION_UP) {
        joy_perform_event(&joystick_devices[joynum].hat_mapping[hat].up, joyport, 1);
    }
    if (!(prev & JOYSTICK_DIRECTION_DOWN) && value & JOYSTICK_DIRECTION_DOWN) {
        joy_perform_event(&joystick_devices[joynum].hat_mapping[hat].down, joyport, 1);
    }
    if (!(prev & JOYSTICK_DIRECTION_LEFT) && value & JOYSTICK_DIRECTION_LEFT) {
        joy_perform_event(&joystick_devices[joynum].hat_mapping[hat].left, joyport, 1);
    }
    if (!(prev & JOYSTICK_DIRECTION_RIGHT) && value & JOYSTICK_DIRECTION_RIGHT) {
        joy_perform_event(&joystick_devices[joynum].hat_mapping[hat].right, joyport, 1);
    }

    joystick_devices[joynum].hat_mapping[hat].prev = value;
}

#ifdef HAVE_SDL_NUMJOYSTICKS
joystick_axis_value_t joy_axis_prev(uint8_t joynum, uint8_t axis)
{
    return joystick_devices[joynum].axis_mapping[axis].prev;
}

joystick_mapping_t *joy_get_axis_mapping(uint8_t                joynum,
                                         uint8_t                axis,
                                         joystick_axis_value_t  value,
                                         joystick_axis_value_t *prev)
{
    joystick_mapping_t *retval = joy_get_axis_mapping_not_setting_value(
            joynum, axis, joystick_devices[joynum].axis_mapping[axis].prev);
    if (prev)
        *prev = joystick_devices[joynum].axis_mapping[axis].prev;
    joystick_devices[joynum].axis_mapping[axis].prev = value;
    return retval;
}

joystick_mapping_t *joy_get_axis_mapping_not_setting_value(uint8_t               joynum,
                                                           uint8_t               axis,
                                                           joystick_axis_value_t value)
{
    if (value == JOY_AXIS_POSITIVE) {
        return &joystick_devices[joynum].axis_mapping[axis].positive_direction;
    }
    if (value == JOY_AXIS_NEGATIVE) {
        return &joystick_devices[joynum].axis_mapping[axis].negative_direction;
    }
    return NULL;
}

joystick_mapping_t *joy_get_button_mapping(uint8_t  joynum,
                                           uint8_t  button,
                                           uint8_t  value,
                                           uint8_t *prev)
{
    joystick_mapping_t *retval = joy_get_button_mapping_not_setting_value(
            joynum, button, joystick_devices[joynum].button_mapping[button].prev);
     if (prev)
        *prev = joystick_devices[joynum].button_mapping[button].prev;
    joystick_devices[joynum].button_mapping[button].prev = value;
    return retval;
}

joystick_mapping_t *joy_get_button_mapping_not_setting_value(uint8_t joynum,
                                                             uint8_t button,
                                                             uint8_t value)
{
    if (value)
        return &joystick_devices[joynum].button_mapping[button].mapping;
    return NULL;
}

joystick_axis_value_t joy_hat_prev(uint8_t joynum, uint8_t hat) {
    return joystick_devices[joynum].hat_mapping[hat].prev;
}

joystick_mapping_t *joy_get_hat_mapping(uint8_t  joynum,
                                        uint8_t  hat,
                                        uint8_t  value,
                                        uint8_t *prev)
{
    joystick_mapping_t *retval = joy_get_hat_mapping_not_setting_value(
            joynum, hat, joystick_devices[joynum].hat_mapping[hat].prev);
     if (prev)
        *prev = joystick_devices[joynum].hat_mapping[hat].prev;
    joystick_devices[joynum].hat_mapping[hat].prev = value;
    return retval;
}

joystick_mapping_t *joy_get_hat_mapping_not_setting_value(uint8_t joynum,
                                                          uint8_t hat,
                                                          uint8_t value) {
    joystick_mapping_t *mapping = NULL;

    if (value & JOYSTICK_DIRECTION_UP) {
        mapping = &joystick_devices[joynum].hat_mapping[hat].up;
    } else if (value & JOYSTICK_DIRECTION_DOWN) {
        mapping = &joystick_devices[joynum].hat_mapping[hat].down;
    } else if (value & JOYSTICK_DIRECTION_LEFT) {
        mapping = &joystick_devices[joynum].hat_mapping[hat].left;
    } else if (value & JOYSTICK_DIRECTION_RIGHT) {
        mapping = &joystick_devices[joynum].hat_mapping[hat].right;
    }

    return mapping;
}
#endif

static int joystickdeviceidx = 0;

void joystick_ui_reset_device_list(void)
{
    joystickdeviceidx = 0;
}

const char *joystick_ui_get_next_device_name(int *id)
{
    if (joystickdeviceidx >=0 && joystickdeviceidx < num_joystick_devices) {
        *id = joystickdeviceidx + JOYDEV_REALJOYSTICK_MIN;
        return joystick_devices[joystickdeviceidx++].jname;
    }
    return NULL;
}

void joystick(void)
{
    int i;
    for (i = 0; i < num_joystick_devices; i++) {
        joystick_devices[i].driver->poll(i, joystick_devices[i].priv);
    }
}

void joystick_close(void)
{
    int i;

    for (i = 0; i < num_joystick_devices; i++) {
        joystick_devices[i].driver->close(joystick_devices[i].priv);
        lib_free(joystick_devices[i].axis_mapping);
        lib_free(joystick_devices[i].button_mapping);
        lib_free(joystick_devices[i].hat_mapping);
    }

    lib_free(joystick_devices);
    joystick_devices = NULL;
}

void joystick_resources_shutdown(void)
{
    if (joymap_factory) {
        lib_free(joymap_factory);
        joymap_factory = NULL;
    }
    if (joymap_file) {
        lib_free(joymap_file);
        joymap_file = NULL;
    }
}
