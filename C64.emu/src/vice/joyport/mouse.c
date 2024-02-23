/*
 * mouse.c - Common mouse handling
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * NEOS & Amiga mouse and paddle support by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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

/* Important requirements to arch specific mouse drivers for proper operation:
 * - mousedrv_get_x and mousedrv_get_y MUST return a value with at
 *   least 16 valid bits in LSB.
 * - mousedrv_get_timestamp MUST give the time stamp when the last
 *   mouse movement happened. A button press is not a movement!
*/

/* #define DEBUG_MOUSE */

#ifdef DEBUG_MOUSE
#define DBG(_x_)  log_debug _x_
#else
#define DBG(_x_)
#endif

#include <stdlib.h> /* abs */
#include <math.h>   /* fabsf */

#include "vice.h"

#include "archdep.h"
#include "cmdline.h"
#include "joyport.h"
#include "machine.h"
#include "maincpu.h"
#include "mouse.h"
#include "mousedrv.h"
#include "resources.h"
#include "snapshot.h"

#include "mouse_1351.h"
#include "mouse_neos.h"
#include "mouse_paddle.h"
#include "mouse_quadrature.h"

/* Log descriptor.  */
#ifdef DEBUG_MOUSE
static log_t mouse_log = LOG_ERR;
#endif

/* Weird trial and error based number here :( larger causes mouse jumps. */
#define MOUSE_MAX_DIFF 63.0f

/******************************************************************************/

static tick_t mouse_timestamp = 0;

static int mouse_sx, mouse_sy;

/******************************************************************************/
/* extern variables (used elsewhere in the codebase) */

int _mouse_enabled = 0;

/* Use xvic defaults, if resources get registered the factory default will overwrite these */
int mouse_type = MOUSE_TYPE_PADDLE;

/******************************************************************************/
static float mouse_move_x = 0.0f;
static float mouse_move_y = 0.0f;

static int last_mouse_x = 0;
static int last_mouse_y = 0;

static int16_t mouse_x = 0;
static int16_t mouse_y = 0;

/* The mousedev only updates its returned coordinates at certain *
 * frequency. We try to estimate this interval by timestamping unique
 * successive readings. The estimated interval is then converted from
 * vsynchapi units to emulated cpu cycles which in turn are used to
 * clock the quardrature emulation. */
static tick_t mouse_latest_os_timestamp = 0; /* in vsynchapi units */

/* The mouse coordinates returned from the latest unique mousedrv reading */
static int16_t mouse_latest_x = 0;
static int16_t mouse_latest_y = 0;

static CLOCK update_x_emu_iv;      /* in cpu cycle units */
static CLOCK update_y_emu_iv;      /* in cpu cycle units */
static CLOCK next_update_x_emu_ts; /* in cpu cycle units */
static CLOCK next_update_y_emu_ts; /* in cpu cycle units */

/* the ratio between emulated cpu cycles and vsynchapi time units */
static float emu_units_per_os_units;

static int update_limit = 512;


int mouse_get_mouse_sx(void)
{
    return mouse_sx;
}

int mouse_get_mouse_sy(void)
{
    return mouse_sy;
}

/* --------------------------------------------------------- */

/* this is called by the UI to move the mouse position */
void mouse_move(float dx, float dy)
{
    /* Capture the relative mouse movement to be processed later in mouse_poll() */
    mouse_move_x += dx;
    mouse_move_y -= dy;
    mouse_timestamp = tick_now();
    DBG(("mouse_move dx:%f dy:%f x:%f y:%f", dx, dy, mouse_move_x, mouse_move_y));
}

/* used by the individual devices to get the mouse position */
void mouse_get_raw_int16(int16_t *x, int16_t *y)
{
    *x = (int16_t)mouse_x;
    *y = (int16_t)mouse_y;
}

void mouse_get_last_int16(int16_t *x, int16_t *y)
{
    *x = (int16_t)last_mouse_x;
    *y = (int16_t)last_mouse_y;
}

/*
    to avoid strange side effects two things are done here:

    - max delta is limited to MOUSE_MAX_DIFF
    - if the delta is limited, then the current position is linearly
      interpolated towards the real position using MOUSE_MAX_DIFF for the axis
      with the largest delta
*/
static void mouse_move_apply_limit(void)
{
    /* Limit the distance that mouse_x/y can have changed since last poll.
     * If we don't do this the mouse moment overflows and a large move
     * can result in either a move in the opposite direction, or the wrong
     * move in the right direction.
     */

    if (fabsf(mouse_move_x) >= fabsf(mouse_move_y)) {
        if (mouse_move_x > MOUSE_MAX_DIFF) {
            mouse_move_y *= MOUSE_MAX_DIFF / mouse_move_x;
            mouse_move_x = MOUSE_MAX_DIFF;
        } else if (mouse_move_x < -MOUSE_MAX_DIFF) {
            mouse_move_y *= -MOUSE_MAX_DIFF / mouse_move_x;
            mouse_move_x = -MOUSE_MAX_DIFF;
        }
    } else {
        if (mouse_move_y > MOUSE_MAX_DIFF) {
            mouse_move_x *= MOUSE_MAX_DIFF / mouse_move_y;
            mouse_move_y = MOUSE_MAX_DIFF;
        } else if (mouse_move_y < -MOUSE_MAX_DIFF) {
            mouse_move_x *= -MOUSE_MAX_DIFF / mouse_move_y;
            mouse_move_y = -MOUSE_MAX_DIFF;
        }
    }
}

/* poll the mouse, returns the digital joyport lines */

/* FIXME: at least the quadrature specific stuff should get moved out of this
          and made private to mouse_quadrature.c */
void mouse_poll(void)
{
    int16_t delta_x, delta_y;

    int16_t new_x, new_y;
    tick_t os_now, os_iv, os_iv2;
    CLOCK emu_now, emu_iv, emu_iv2;
    int diff_x, diff_y;

    DBG(("mouse_poll"));

    /* Ensure the mouse hasn't moved too far since the last poll */
    mouse_move_apply_limit();

    /* Capture an integer representation of how far the mouse has moved */
    delta_x = (int16_t)mouse_move_x;
    delta_y = (int16_t)mouse_move_y;

    /* Update the view of where the mouse is based on the accumulated delta */
    mouse_x += delta_x;
    mouse_y += delta_y;

    /* Subtract the int delta from the floating point, preserving fractional elemement */
    mouse_move_x -= delta_x;
    mouse_move_y -= delta_y;

    /* OK - on with the show, get new mouse values */
    new_x = (int16_t)mouse_x;
    new_y = (int16_t)mouse_y;

    /* range of new_x and new_y are [0,63] */
    /* fetch now for both emu and os */
    os_now = mouse_timestamp;
    emu_now = maincpu_clk;

    /* update x-wheel until we're ahead */
    while (((mouse_latest_x ^ last_mouse_x) & 0xffff) && next_update_x_emu_ts <= emu_now) {
        last_mouse_x += mouse_sx;
        next_update_x_emu_ts += update_x_emu_iv;
    }

    /* update y-wheel until we're ahead */
    while (((mouse_latest_y ^ last_mouse_y) & 0xffff) && next_update_y_emu_ts <= emu_now) {
        last_mouse_y -= mouse_sy;
        next_update_y_emu_ts += update_y_emu_iv;
    }

    /* check if the new values belong to a new mouse reading */
    if (mouse_latest_os_timestamp == 0) {
        /* only first time, init stuff */
        last_mouse_x = mouse_latest_x = new_x;
        last_mouse_y = mouse_latest_y = new_y;
        mouse_latest_os_timestamp = os_now;
    } else if (os_now != mouse_latest_os_timestamp && (new_x != mouse_latest_x || new_y != mouse_latest_y)) {
        /* yes, we have a new unique mouse coordinate reading */

        /* calculate the interval between the latest two mousedrv
         * updates in emulated cycles */
        os_iv = os_now - mouse_latest_os_timestamp;
        /* FIXME: call function only once */
        if (os_iv > tick_per_second()) {
            os_iv = tick_per_second(); /* more than a second response time?! */
        }
        emu_iv = (CLOCK)((float)os_iv * emu_units_per_os_units);
        /* FIXME: call function only once, remove cast */
        if (emu_iv > (unsigned long)machine_get_cycles_per_frame() * 2) {
            emu_iv = (CLOCK)machine_get_cycles_per_frame() * 2;   /* move in not more than 2 frames */
        }
#ifdef DEBUG_MOUSE
        log_message(mouse_log,
                    "New interval os_now %u, os_iv %u, emu_iv %lu",
                    os_now, os_iv, emu_iv);
#endif

        /* Let's set up quadrature emulation */
        diff_x = (int16_t)(new_x - last_mouse_x);
        diff_y = (int16_t)(new_y - last_mouse_y);

        if (diff_x != 0) {
            mouse_sx = diff_x >= 0 ? 1 : -1;
            /* lets calculate the interval between x-quad rotations */
            update_x_emu_iv = emu_iv / (CLOCK)abs(diff_x);
            /* and the emulated cpu cycle count when to do the first one */
            next_update_x_emu_ts = emu_now;
        } else {
            mouse_sx = 0;
            update_x_emu_iv = (CLOCK)update_limit;
        }
        if (diff_y != 0) {
            mouse_sy = diff_y >= 0 ? -1 : 1;
            /* lets calculate the interval between y-quad rotations */
            update_y_emu_iv = (emu_iv / (CLOCK)abs(diff_y));
            /* and the emulated cpu cycle count when to do the first one */
            next_update_y_emu_ts = emu_now;
        } else {
            mouse_sy = 0;
            update_y_emu_iv = (CLOCK)update_limit;
        }
        if (update_x_emu_iv < (unsigned int)update_limit) {
            if (update_x_emu_iv) {
                update_y_emu_iv = update_y_emu_iv * (CLOCK)update_limit / update_x_emu_iv;
            }
            update_x_emu_iv = (CLOCK)update_limit;
        }
        if (update_y_emu_iv < (unsigned int)update_limit) {
            if (update_y_emu_iv) {
                update_x_emu_iv = update_x_emu_iv * (CLOCK)update_limit / update_y_emu_iv;
            }
            update_y_emu_iv = (CLOCK)update_limit;
        }

#ifdef DEBUG_MOUSE
        log_message(mouse_log, "cpu %lu iv %lu,%lu old %d,%d new %d,%d",
                    emu_now, update_x_emu_iv, update_y_emu_iv,
                    mouse_latest_x, mouse_latest_y, new_x, new_y);
#endif

        /* The mouse read is probably old. Do the movement since then */
        os_iv2 = tick_now_delta(os_now);
        /* FIXME: call function only once */
        if (os_iv2 > tick_per_second()) {
            os_iv2 = tick_per_second(); /* more than a second response time?! */
        }
        emu_iv2 = (CLOCK)((float)os_iv2 * emu_units_per_os_units);
        /* FIXME: call function only once, remove cast */
        if (emu_iv2 > (unsigned long)machine_get_cycles_per_second()) {
            emu_iv2 = (CLOCK)machine_get_cycles_per_second();   /* more than a second? */
        }

        /* update x-wheel until we're ahead */
        while (((new_x ^ last_mouse_x) & 0xffff) && next_update_x_emu_ts < emu_now + emu_iv2) {
            last_mouse_x += mouse_sx;
            next_update_x_emu_ts += update_x_emu_iv;
        }

        /* update y-wheel until we're ahead */
        while (((new_y ^ last_mouse_y) & 0xffff) && next_update_y_emu_ts <= emu_now + emu_iv2) {
            last_mouse_y -= mouse_sy;
            next_update_y_emu_ts += update_y_emu_iv;
        }

        /* store the new coordinates for next time */
        mouse_latest_x = new_x;
        mouse_latest_y = new_y;
        mouse_latest_os_timestamp = os_now;
    }
}

void mouse_reset(void)
{
    mousedrv_mouse_changed();

    mouse_get_raw_int16(&mouse_latest_x, &mouse_latest_y);
    last_mouse_x = mouse_latest_x;
    last_mouse_y = mouse_latest_y;
    mouse_latest_os_timestamp = 0;
}

void mouse_init(void)
{
    /* FIXME: some of these can perhaps be moved into individual devices */
    emu_units_per_os_units = (float)(machine_get_cycles_per_second() / tick_per_second());
    update_limit = (int)(machine_get_cycles_per_frame() / 31 / 2);
#ifdef DEBUG_MOUSE
    mouse_log = log_open("Mouse");
    log_message(mouse_log, "cpu cycles / time unit %.5f", emu_units_per_os_units);
#endif

    mouse_amiga_st_init();
    mouse_neos_init();

    mousedrv_init();
    mouse_reset();
}

void mouse_shutdown(void)
{
    smart_mouse_shutdown();
}

/*--------------------------------------------------------------------------*/
/* Main API */

static void mouse_button_left(int pressed)
{
    switch (mouse_type) {
        case MOUSE_TYPE_1351:
        case MOUSE_TYPE_SMART:
        case MOUSE_TYPE_MICROMYS:
            mouse_1351_button_left(pressed);
            break;
        case MOUSE_TYPE_KOALAPAD:
        case MOUSE_TYPE_PADDLE:
            paddles_button_left(pressed);
            break;
        case MOUSE_TYPE_NEOS:
            mouse_neos_button_left(pressed);
            break;
        case MOUSE_TYPE_AMIGA:
        case MOUSE_TYPE_ST:
            mouse_amiga_st_button_left(pressed);
            break;
        default:
            break;
    }
}

static void mouse_button_right(int pressed)
{
    switch (mouse_type) {
        case MOUSE_TYPE_1351:
        case MOUSE_TYPE_SMART:
        case MOUSE_TYPE_MICROMYS:
            mouse_1351_button_right(pressed);
            break;
        case MOUSE_TYPE_KOALAPAD:
        case MOUSE_TYPE_PADDLE:
            paddles_button_right(pressed);
            break;
        case MOUSE_TYPE_NEOS:
            mouse_neos_button_right(pressed);
            break;
        case MOUSE_TYPE_AMIGA:
        case MOUSE_TYPE_ST:
            mouse_amiga_st_button_right(pressed);
            break;
        default:
            break;
    }
}

static void mouse_button_middle(int pressed)
{
    switch (mouse_type) {
        case MOUSE_TYPE_MICROMYS:
            micromys_mouse_button_middle(pressed);
            break;
        case MOUSE_TYPE_AMIGA:
        case MOUSE_TYPE_ST:
            mouse_amiga_st_button_right(pressed);
            break;
        default:
            break;
    }
}

static void mouse_button_up(int pressed)
{
    switch (mouse_type) {
        case MOUSE_TYPE_MICROMYS:
            micromys_mouse_button_up(pressed);
            break;
        default:
            break;
    }
}

static void mouse_button_down(int pressed)
{
    switch (mouse_type) {
        case MOUSE_TYPE_MICROMYS:
            micromys_mouse_button_down(pressed);
            break;
        default:
            break;
    }
}

/*--------------------------------------------------------------------------*/

void mouse_set_machine_parameter(long clock_rate)
{
    neos_mouse_set_machine_parameter(clock_rate);
}

/*--------------------------------------------------------------------------*/

typedef struct mt_id_s {
    int mt;
    int id;
} mt_id_t;

static const mt_id_t mt_id[] = {
    { MOUSE_TYPE_PADDLE,   JOYPORT_ID_PADDLES },
    { MOUSE_TYPE_MF_JOY,   JOYPORT_ID_MF_JOYSTICK },
    { MOUSE_TYPE_1351,     JOYPORT_ID_MOUSE_1351 },
    { MOUSE_TYPE_NEOS,     JOYPORT_ID_MOUSE_NEOS },
    { MOUSE_TYPE_AMIGA,    JOYPORT_ID_MOUSE_AMIGA },
    { MOUSE_TYPE_CX22,     JOYPORT_ID_MOUSE_CX22 },
    { MOUSE_TYPE_ST,       JOYPORT_ID_MOUSE_ST },
    { MOUSE_TYPE_SMART,    JOYPORT_ID_MOUSE_SMART },
    { MOUSE_TYPE_MICROMYS, JOYPORT_ID_MOUSE_MICROMYS },
    { MOUSE_TYPE_KOALAPAD, JOYPORT_ID_KOALAPAD },
    { -1,                  -1 }
};

/* convert from joyport ID to mouse type */
int mouse_id_to_type(int id)
{
    int i;

    for (i = 0; mt_id[i].mt != -1; ++i) {
        if (mt_id[i].id == id) {
            return mt_id[i].mt;
        }
    }
    return -1;
}

/* convert from mouse type to joyport ID */
int mouse_type_to_id(int mt)
{
    int i;

    for (i = 0; mt_id[i].mt != -1; ++i) {
        if (mt_id[i].mt == mt) {
            return mt_id[i].id;
        }
    }
    return -1;
}

/* --------------------------------------------------------- */
/* Resources & cmdline */

static int set_mouse_enabled(int new_state, void *param)
{
    if (_mouse_enabled == new_state) {
        return 0;
    }

    _mouse_enabled = new_state ? 1 : 0;

    mouse_reset();
    mouse_neos_set_enabled(_mouse_enabled);

    if (mouse_type != -1) {
        /* FIXME: we don't know the port here, using JOYPORT_ID_UNKNOWN will
                  make joyport_display_joyport search for the device in all
                  available ports */
        joyport_display_joyport(JOYPORT_ID_UNKNOWN, mouse_type_to_id(mouse_type), 0);
    }
    return 0;
}

static const resource_int_t resources_int[] = {
    { "Mouse", ARCHDEP_MOUSE_ENABLE_DEFAULT, RES_EVENT_SAME, NULL,
      &_mouse_enabled, set_mouse_enabled, NULL },
    RESOURCE_INT_LIST_END
};

static const mouse_func_t mouse_funcs =
{
    mouse_button_left,
    mouse_button_right,
    mouse_button_middle,
    mouse_button_up,
    mouse_button_down
};

int mouse_resources_init(void)
{
    DBG(("mouse_resources_init\n"));
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }

    if (paddles_resources_init() < 0) {
        return -1;
    }

    if (smart_mouse_resources_init() < 0) {
        return -1;
    }

    return mousedrv_resources_init(&mouse_funcs);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-mouse", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Mouse", (void *)1,
      NULL, "Enable mouse grab" },
    { "+mouse", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Mouse", (void *)0,
      NULL, "Disable mouse grab" },
    CMDLINE_LIST_END
};

int mouse_cmdline_options_init(void)
{
    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    if (paddles_cmdline_options_init() < 0) {
        return -1;
    }

    if (smart_mouse_cmdline_options_init() < 0) {
        return -1;
    }

    return mousedrv_cmdline_options_init();
}

/* --------------------------------------------------------- */


/* Format of the common part of the mouse snapshots:

   type   | name                   | description
   ---------------------------------------------
   WORD   | latest X               | latest X
   WORD   | latest Y               | latest Y
   DWORD  | last mouse X           | last mouse X
   DWORD  | last mouse Y           | last mouse Y
   DWORD  | mouse_sx               | SX
   DWORD  | mouse_sy               | SY
   DWORD  | update limit           | update limit
   DWORD  | latest os ts           | latest os ts
   DOUBLE | emu units per os units | emu units per os units
   DWORD  | next update x emu ts   | next update X emu ts
   DWORD  | next update y emu ts   | next update Y emu ts
   DWORD  | update x emu iv        | update X emu IV
   DWORD  | update y emu iv        | update Y emu IV
 */

int write_mouse_common_snapshot(snapshot_module_t *m)
{
    if (0
        || SMW_W(m, (uint16_t)mouse_latest_x) < 0
        || SMW_W(m, (uint16_t)mouse_latest_y) < 0
        || SMW_DW(m, (uint32_t)last_mouse_x) < 0
        || SMW_DW(m, (uint32_t)last_mouse_y) < 0
        || SMW_DW(m, (uint32_t)mouse_sx) < 0
        || SMW_DW(m, (uint32_t)mouse_sy) < 0
        || SMW_DW(m, (uint32_t)update_limit) < 0
        || SMW_DW(m, (uint32_t)mouse_latest_os_timestamp) < 0
        || SMW_DB(m, (double)emu_units_per_os_units) < 0
        || SMW_DW(m, (uint32_t)next_update_x_emu_ts) < 0
        || SMW_DW(m, (uint32_t)next_update_y_emu_ts) < 0
        || SMW_DW(m, (uint32_t)update_x_emu_iv) < 0
        || SMW_DW(m, (uint32_t)update_y_emu_iv) < 0) {
        return -1;
    }
    return 0;
}

int read_mouse_common_snapshot(snapshot_module_t *m)
{
    uint16_t tmp_mouse_latest_x;
    uint16_t tmp_mouse_latest_y;
    double tmp_db;
    uint32_t tmpc1;
    uint32_t tmpc2;
    uint32_t tmpc3;
    uint32_t tmpc4;
    unsigned long tmp_mouse_latest_os_timestamp;

    if (0
        || SMR_W(m, &tmp_mouse_latest_x) < 0
        || SMR_W(m, &tmp_mouse_latest_y) < 0
        || SMR_DW_INT(m, &last_mouse_x) < 0
        || SMR_DW_INT(m, &last_mouse_y) < 0
        || SMR_DW_INT(m, &mouse_sx) < 0
        || SMR_DW_INT(m, &mouse_sy) < 0
        || SMR_DW_INT(m, &update_limit) < 0
        || SMR_DW_UL(m, &tmp_mouse_latest_os_timestamp) < 0
        || SMR_DB(m, &tmp_db) < 0
        || SMR_DW(m, &tmpc1) < 0
        || SMR_DW(m, &tmpc2) < 0
        || SMR_DW(m, &tmpc3) < 0
        || SMR_DW(m, &tmpc4) < 0) {
        return -1;
    }

    mouse_latest_x = (int16_t)tmp_mouse_latest_x;
    mouse_latest_y = (int16_t)tmp_mouse_latest_y;
    emu_units_per_os_units = (float)tmp_db;
    mouse_latest_os_timestamp = (tick_t)tmp_mouse_latest_os_timestamp;
    next_update_x_emu_ts = (CLOCK)tmpc1;
    next_update_y_emu_ts = (CLOCK)tmpc2;
    update_x_emu_iv = (CLOCK)tmpc3;
    update_y_emu_iv = (CLOCK)tmpc4;
    return 0;
}
