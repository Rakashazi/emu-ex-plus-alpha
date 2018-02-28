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
#include <string.h> /* memset */
#include "vice.h"

#include "alarm.h"
#include "cmdline.h"
#include "joyport.h"
#include "joystick.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mouse.h"
#include "mousedrv.h"
#include "resources.h"
#include "snapshot.h"
#include "translate.h"
#include "vsyncapi.h"
#include "clkguard.h"
#include "ds1202_1302.h"

/* Control port <--> mouse/paddles/pad connections:

   cport | 1351         | I/O
   --------------------------
     1   | right button |  I
     5   | Y-position   |  I
     6   | left button  |  I
     9   | X-position   |  I

   cport | neos         | I/O
   --------------------------
     1   | D0           |  I
     2   | D1           |  I
     3   | D2           |  I
     4   | D3           |  I
     6   | strobe       |  O
     6   | left button  |  I
     9   | right button |  I

   cport | amiga         | I/O
   ---------------------------
     1   | V-pulse       |  I
     2   | H-pulse       |  I
     3   | VQ-pulse      |  I
     4   | HQ-pulse      |  I
     5   | middle button |  I
     6   | left button   |  I
     9   | right button  |  I

   cport | paddles         | I/O
   -----------------------------
     3   | paddle X button |  I
     4   | paddle Y button |  I
     5   | paddle Y value  |  I
     9   | paddle X value  |  I

   cport | cx22        | I/O
   -------------------------
     1   | X direction |  I
     2   | X motion    |  I
     3   | Y direction |  I
     4   | Y motion    |  I
     6   | button      |  I

   cport | atari-st     | I/O
   --------------------------
     1   | XB           |  I
     2   | XA           |  I
     3   | YA           |  I
     4   | YB           |  I
     6   | left button  |  I
     9   | right button |  I

   cport | smart mouse  | I/O
   --------------------------
     1   | right button |  I
     2   | RTC clock    |  O
     3   | RTC I/O      | I/O
     4   | RTC RST      | I/O
     5   | Y-position   |  I
     6   | left button  |  I
     9   | X-position   |  I

   cport | micromys      | I/O
   ---------------------------
     1   | right button  |  I
     2   | middle button |  I
     3   | wheel up      |  I
     4   | wheel down    |  I
     5   | Y-position    |  I
     6   | left button   |  I
     9   | X-position    |  I

   cport | koalapad     | I/O
   --------------------------
     3   | left button  |  I
     4   | right button |  I
     5   | Y-position   |  I
     9   | X-position   |  I
 */

/* Log descriptor.  */
#ifdef DEBUG_MOUSE
static log_t mouse_log = LOG_ERR;
#endif

static void mouse_button_left(int pressed);
static void mouse_button_right(int pressed);
static void mouse_button_middle(int pressed);
static void mouse_button_up(int pressed);
static void mouse_button_down(int pressed);

/* --------------------------------------------------------- */
/* extern variables */

int _mouse_enabled = 0;

/* Use xvic defaults, if resources get registered the factory
   default will overwrite these */
int mouse_type = MOUSE_TYPE_PADDLE;

/* --------------------------------------------------------- */
/* 1351 mouse */

/*
    to avoid strange side effects two things are done here:

    - max delta is limited to MOUSE_MAX_DIFF
    - if the delta is limited, then the current position is linearly
      interpolated towards the real position using MOUSE_MAX_DIFF for the axis
      with the largest delta
*/

static int update_limit = 512;
static int last_mouse_x = 0;
static int last_mouse_y = 0;
static rtc_ds1202_1302_t *ds1202; /* smartmouse */
static int ds1202_rtc_save; /* smartmouse rtc data save */
static BYTE mouse_digital_val = 0;

/*
    note: for the expected behaviour look at testprogs/SID/paddles/readme.txt

    "The Final Cartridge 3" (and possibly others?) do not work if the mouse
    is inserted (as in enabled) after it has started. so either enable mouse
    emulation on the commandline, or (re)start (or reset incase of a cart)
    after enabling it in the gui. (see testprogs/SID/paddles/fc3detect.asm)

    HACK: when both ports are selected, a proper combined value should be
          returned. however, since we are currently only emulating a single
          mouse or pair of paddles, always returning its respective value in
          this case is ok.
*/

static BYTE mouse_get_1351_x(void)
{
    if (_mouse_enabled) {
        mouse_poll();
        return (BYTE)((last_mouse_x & 0x7f) + 0x40);
    }
    return 0xff;
}

static BYTE mouse_get_1351_y(void)
{
    if (_mouse_enabled) {
        mouse_poll();
        return (BYTE)((last_mouse_y & 0x7f) + 0x40);
    }
    return 0xff;
}

/* --------------------------------------------------------- */
/* NEOS mouse */

static CLOCK neos_last_trigger = 0;
static CLOCK neos_time_out_cycles = 232;

static int neos_and_amiga_buttons;
static int neos_prev;

static BYTE neos_x;
static BYTE neos_y;
static BYTE neos_lastx;
static BYTE neos_lasty;

enum {
    NEOS_XH = 0,
    NEOS_XL,
    NEOS_YH,
    NEOS_YL
} neos_state = NEOS_YL;


void neos_mouse_set_machine_parameter(long clock_rate)
{
    neos_time_out_cycles = (CLOCK)((clock_rate / 10000) * 2);
}

static void neos_get_new_movement(void)
{
    BYTE new_x, new_y;

    new_x = (BYTE)(mousedrv_get_x() >> 1);
    new_y = (BYTE)(mousedrv_get_y() >> 1);
    neos_x = (BYTE)(neos_lastx - new_x);
    neos_lastx = new_x;

    neos_y = (BYTE)(new_y - neos_lasty);
    neos_lasty = new_y;
}

void neos_mouse_store(BYTE val)
{
    if ((neos_prev & 16) != (val & 16)) {
        switch (neos_state) {
            case NEOS_YL:
                if ((val ^ neos_prev) & neos_prev & 16) {
                    neos_state = NEOS_XH;
                    neos_get_new_movement();
                }
                break;
            case NEOS_XH:
                if ((val ^ neos_prev) & val & 16) {
                    ++neos_state;
                    neos_last_trigger = maincpu_clk;
                }
                break;
            case NEOS_XL:
                if ((val ^ neos_prev) & neos_prev & 16) {
                    ++neos_state;
                }
                break;
            case NEOS_YH:
                if ((val ^ neos_prev) & val & 16) {
                    ++neos_state;
                }
                break;
            default:
                /* NOP */
                break;
        }
        neos_last_trigger = maincpu_clk;
        neos_prev = val;
    }
}

BYTE neos_mouse_read(void)
{
    if (neos_state != NEOS_XH && maincpu_clk > neos_last_trigger + neos_time_out_cycles) {
        neos_state = NEOS_XH;
        neos_get_new_movement();
    }

    switch (neos_state) {
        case NEOS_XH:
            return ((neos_x >> 4) & 0xf) | 0xf0;
            break;
        case NEOS_XL:
            return (neos_x & 0xf) | 0xf0;
            break;
        case NEOS_YH:
            return ((neos_y >> 4) & 0xf) | 0xf0;
            break;
        case NEOS_YL:
            return (neos_y & 0xf) | 0xf0;
            break;
        default:
            return 0xff;
            break;
    }
}

/* --------------------------------------------------------- */
/* quadrature encoding mice support (currently experimental) */

/* The mousedev only updates its returned coordinates at certain *
 * frequency. We try to estimate this interval by timestamping unique
 * successive readings. The estimated interval is then converted from
 * vsynchapi units to emulated cpu cycles which in turn are used to
 * clock the quardrature emulation. */
static unsigned long latest_os_ts = 0; /* in vsynchapi units */
/* The mouse coordinates returned from the latest unique mousedrv
 * reading */
static SWORD latest_x = 0;
static SWORD latest_y = 0;

static CLOCK update_x_emu_iv = 0;      /* in cpu cycle units */
static CLOCK update_y_emu_iv = 0;      /* in cpu cycle units */
static CLOCK next_update_x_emu_ts = 0; /* in cpu cycle units */
static CLOCK next_update_y_emu_ts = 0; /* in cpu cycle units */
static CLOCK up_down_pulse_end = 0;    /* in cpu cycle units */
static int sx, sy;

/* the ratio between emulated cpu cycles and vsynchapi time units */
static float emu_units_per_os_units;

/* The current emulated quadrature state of the polled mouse, range is
 * [0,3] */
static BYTE quadrature_x = 0;
static BYTE quadrature_y = 0;

static BYTE polled_joyval = 0xff;

static const BYTE amiga_mouse_table[4] = { 0x0, 0x1, 0x5, 0x4 };
static const BYTE st_mouse_table[4] = { 0x0, 0x2, 0x3, 0x1 };

/* Clock overflow handling.  */
static void clk_overflow_callback(CLOCK sub, void *data)
{
    if (next_update_x_emu_ts > (CLOCK) 0) {
        next_update_x_emu_ts -= sub;
    }
    if (next_update_y_emu_ts > (CLOCK) 0) {
        next_update_y_emu_ts -= sub;
    }
    if (up_down_pulse_end > (CLOCK) 0) {
        up_down_pulse_end -= sub;
    }
}

BYTE mouse_poll(void)
{
    SWORD new_x, new_y;
    unsigned long os_now, os_iv, os_iv2;
    CLOCK emu_now, emu_iv, emu_iv2;
    int diff_x, diff_y;

    /* get new mouse values */
    new_x = (SWORD)mousedrv_get_x();
    new_y = (SWORD)mousedrv_get_y();
    /* range of new_x and new_y are [0,63] */
    /* fetch now for both emu and os */
    os_now = mousedrv_get_timestamp();
    emu_now = maincpu_clk;

    /* update x-wheel until we're ahead */
    while (((latest_x ^ last_mouse_x) & 0xffff) && next_update_x_emu_ts <= emu_now) {
        last_mouse_x += sx;
        next_update_x_emu_ts += update_x_emu_iv;
    }

    /* update y-wheel until we're ahead */
    while (((latest_y ^ last_mouse_y) & 0xffff) && next_update_y_emu_ts <= emu_now) {
        last_mouse_y -= sy;
        next_update_y_emu_ts += update_y_emu_iv;
    }

    /* check if the new values belong to a new mouse reading */
    if (latest_os_ts == 0) {
        /* only first time, init stuff */
        last_mouse_x = latest_x = new_x;
        last_mouse_y = latest_y = new_y;
        latest_os_ts = os_now;
    } else if (os_now != latest_os_ts && (new_x != latest_x || new_y != latest_y)) {
        /* yes, we have a new unique mouse coordinate reading */

        /* calculate the interval between the latest two mousedrv
         * updates in emulated cycles */
        os_iv = os_now - latest_os_ts;
        if (os_iv > (unsigned long)vsyncarch_frequency()) {
            os_iv = (unsigned long)vsyncarch_frequency(); /* more than a second response time?! */
        }
        emu_iv = (CLOCK)((float)os_iv * emu_units_per_os_units);
        if (emu_iv > (unsigned long)machine_get_cycles_per_frame() * 2) {
            emu_iv = (CLOCK)machine_get_cycles_per_frame() * 2;   /* move in not more than 2 frames */
        }
#ifdef DEBUG_MOUSE
        log_message(mouse_log,
                    "New interval os_now %lu, os_iv %lu, emu_iv %lu",
                    os_now, os_iv, emu_iv);
#endif

        /* Let's set up quadrature emulation */
        diff_x = (SWORD)(new_x - last_mouse_x);
        diff_y = (SWORD)(new_y - last_mouse_y);

        if (diff_x != 0) {
            sx = diff_x >= 0 ? 1 : -1;
            /* lets calculate the interval between x-quad rotations */
            update_x_emu_iv = emu_iv / (CLOCK)abs(diff_x);
            /* and the emulated cpu cycle count when to do the first one */
            next_update_x_emu_ts = emu_now;
        } else {
            sx = 0;
            update_x_emu_iv = (CLOCK)update_limit;
        }
        if (diff_y != 0) {
            sy = diff_y >= 0 ? -1 : 1;
            /* lets calculate the interval between y-quad rotations */
            update_y_emu_iv = (emu_iv / (CLOCK)abs(diff_y));
            /* and the emulated cpu cycle count when to do the first one */
            next_update_y_emu_ts = emu_now;
        } else {
            sy = 0;
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
        log_message(mouse_log, "cpu %u iv %u,%u old %d,%d new %d,%d",
                    emu_now, update_x_emu_iv, update_y_emu_iv,
                    latest_x, latest_y, new_x, new_y);
#endif

        /* The mouse read is probably old. Do the movement since then */
        os_iv2 = vsyncarch_gettime() - os_now;
        if (os_iv2 > (unsigned long)vsyncarch_frequency()) {
            os_iv2 = (unsigned long)vsyncarch_frequency(); /* more than a second response time?! */
        }
        emu_iv2 = (CLOCK)((float)os_iv2 * emu_units_per_os_units);
        if (emu_iv2 > (unsigned long)machine_get_cycles_per_second()) {
            emu_iv2 = (CLOCK)machine_get_cycles_per_second();   /* more than a second? */
        }

        /* update x-wheel until we're ahead */
        while (((new_x ^ last_mouse_x) & 0xffff) && next_update_x_emu_ts < emu_now + emu_iv2) {
            last_mouse_x += sx;
            next_update_x_emu_ts += update_x_emu_iv;
        }

        /* update y-wheel until we're ahead */
        while (((new_y ^ last_mouse_y) & 0xffff) && next_update_y_emu_ts <= emu_now + emu_iv2) {
            last_mouse_y -= sy;
            next_update_y_emu_ts += update_y_emu_iv;
        }

        /* store the new coordinates for next time */
        latest_x = new_x;
        latest_y = new_y;
        latest_os_ts = os_now;
    }

    if ((quadrature_x != ((last_mouse_x >> 1) & 3)) || (quadrature_y != ((~last_mouse_y >> 1) & 3))) {
        /* keep within range */
        quadrature_x = (last_mouse_x >> 1) & 3;
        quadrature_y = (~last_mouse_y >> 1) & 3;

        switch (mouse_type) {
            case MOUSE_TYPE_AMIGA:
                polled_joyval = (BYTE)((amiga_mouse_table[quadrature_x] << 1) | amiga_mouse_table[quadrature_y] | 0xf0);
                break;
            case MOUSE_TYPE_CX22:
                polled_joyval = (BYTE)(((quadrature_y & 1) << 3) | ((sy > 0) << 2) | ((quadrature_x & 1) << 1) | (sx > 0) | 0xf0);
                break;
            case MOUSE_TYPE_ST:
                polled_joyval =(BYTE)(st_mouse_table[quadrature_x] | (st_mouse_table[quadrature_y] << 2) | 0xf0);
                break;
            default:
                polled_joyval = 0xff;
        }
    }
    return polled_joyval;
}

static int up_down_counter = 0;

BYTE micromys_mouse_read(void)
{
    /* update wheel until we're ahead */
    while (up_down_counter && up_down_pulse_end <= maincpu_clk) {
        up_down_counter += (up_down_counter < 0) * 2 - 1;
        up_down_pulse_end += 512 * 98; /* 50 ms counted from POT input (98 A/D cycles) */
    }
    if (up_down_counter & 1) {
        return (BYTE)(~(4 << (up_down_counter < 0)));
    }
    return 0xff;
}

/* --------------------------------------------------------- */
/* Paddle support */

static BYTE paddle_val[] = {
/*  x     y  */
    0x00, 0xff, /* no port */
    0x00, 0xff, /* port 1 */
    0x00, 0xff, /* port 2 */
    0x00, 0xff  /* both ports */
};

static SWORD paddle_old[] = {
    -1, -1,
    -1, -1,
    -1, -1,
    -1, -1
};

static inline BYTE mouse_paddle_update(BYTE paddle_v, SWORD *old_v, SWORD new_v)
{
    SWORD new_paddle = (SWORD)(paddle_v + new_v - *old_v);
    *old_v = new_v;

    if (new_paddle > 255) {
        return 255;
    }
    if (new_paddle < 0) {
        return 0;
    }

    return (BYTE)new_paddle;
}

/*
    note: for the expected behaviour look at testprogs/SID/paddles/readme.txt

    HACK: when both ports are selected, a proper combined value should be
          returned. however, since we are currently only emulating a single
          mouse or pair of paddles, always returning its respective value in
          this case is ok.
*/

static BYTE mouse_get_paddle_x(void)
{
    if (_mouse_enabled) {
        paddle_val[2] = mouse_paddle_update(paddle_val[2], &(paddle_old[2]), (SWORD)mousedrv_get_x());
        return (BYTE)(0xff - paddle_val[2]);
    }
    return 0xff;
}

static BYTE mouse_get_paddle_y(void)
{
    if (_mouse_enabled) {
        paddle_val[3] = mouse_paddle_update(paddle_val[3], &(paddle_old[3]), (SWORD)mousedrv_get_y());
        return (BYTE)(0xff - paddle_val[3]);
    }
    return 0xff;
}

/*--------------------------------------------------------------------------*/

typedef struct mt_id_s {
    int mt;
    int id;
} mt_id_t;

static mt_id_t mt_id[] = {
    { MOUSE_TYPE_PADDLE,   JOYPORT_ID_PADDLES },
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

static int id_to_mt(int id)
{
    int i;

    for (i = 0; mt_id[i].mt != -1; ++i) {
        if (mt_id[i].id == id) {
            return mt_id[i].mt;
        }
    }
    return -1;
}

static int mt_to_id(int mt)
{
    int i;

    for (i = 0; mt_id[i].mt != -1; ++i) {
        if (mt_id[i].mt == mt) {
            return mt_id[i].id;
        }
    }
    return -1;
}

static int joyport_mouse_enable(int port, int val)
{
    int mt;

    mousedrv_mouse_changed();
    latest_x = (SWORD)mousedrv_get_x();
    last_mouse_x = latest_x;
    latest_y = (SWORD)mousedrv_get_y();
    last_mouse_y = latest_y;
    neos_lastx = (BYTE)(mousedrv_get_x() >> 1);
    neos_lasty = (BYTE)(mousedrv_get_y() >> 1);
    latest_os_ts = 0;

    if (!val) {
        if (ds1202) {
            ds1202_1302_destroy(ds1202, ds1202_rtc_save);
            ds1202 = NULL;
        }
        mouse_type = -1;
        return 0;
    }

    mt = id_to_mt(val);

    if (mt == -1) {
        return -1;
    }

    if (mt == mouse_type) {
        return 0;
    }

    mouse_type = mt;

    if (mt == MOUSE_TYPE_SMART) {
        ds1202 = ds1202_1302_init("SM", 1202);
    }

    return 0;
}

static BYTE joyport_mouse_value(int port)
{
    return _mouse_enabled ? (BYTE)~mouse_digital_val : 0xff;
}

/* Some prototypes are needed */
static int paddles_write_snapshot(struct snapshot_s *s, int port);
static int paddles_read_snapshot(struct snapshot_s *s, int port);

static joyport_t paddles_joyport_device = {
    "Paddles",
    IDGS_PADDLES,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_REQUIRED,
    joyport_mouse_enable,
    joyport_mouse_value,
    NULL,				/* no store digital */
    mouse_get_paddle_x,
    mouse_get_paddle_y,
    paddles_write_snapshot,
    paddles_read_snapshot
};

/* Some prototypes are needed */
static int mouse_1351_write_snapshot(struct snapshot_s *s, int port);
static int mouse_1351_read_snapshot(struct snapshot_s *s, int port);

static joyport_t mouse_1351_joyport_device = {
    "Mouse (1351)",
    IDGS_MOUSE_1351,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_REQUIRED,
    joyport_mouse_enable,
    joyport_mouse_value,
    NULL,				/* no store digital */
    mouse_get_1351_x,
    mouse_get_1351_y,
    mouse_1351_write_snapshot,
    mouse_1351_read_snapshot
};

static BYTE joyport_mouse_neos_value(int port)
{
    BYTE retval = 0xff;

    if (_mouse_enabled) {
        retval = (BYTE)((~mouse_digital_val) & neos_mouse_read());
        if (retval != (BYTE)~mouse_digital_val) {
            joyport_display_joyport(mt_to_id(mouse_type), (BYTE)(~retval));
        }
    }
    return retval;
}

static BYTE joyport_mouse_neos_amiga_st_read_potx(void)
{
    return _mouse_enabled ? ((neos_and_amiga_buttons & 1) ? 0xff : 0) : 0xff;
}

/* Some prototypes are needed */
static int mouse_neos_write_snapshot(struct snapshot_s *s, int port);
static int mouse_neos_read_snapshot(struct snapshot_s *s, int port);

static joyport_t mouse_neos_joyport_device = {
    "Mouse (NEOS)",
    IDGS_MOUSE_NEOS,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_OPTIONAL,
    joyport_mouse_enable,
    joyport_mouse_neos_value,
    neos_mouse_store,
    joyport_mouse_neos_amiga_st_read_potx,
    NULL,               /* no read pot y */
    mouse_neos_write_snapshot,
    mouse_neos_read_snapshot
};

static BYTE joyport_mouse_poll_value(int port)
{
    BYTE retval = 0xff;

    if (_mouse_enabled) {
        retval = (BYTE)((~mouse_digital_val) & mouse_poll());
        if (retval != (BYTE)~mouse_digital_val) {
            joyport_display_joyport(mt_to_id(mouse_type), (BYTE)(~retval));
        }
    }
    return retval;
}

static BYTE joyport_mouse_amiga_st_read_poty(void)
{
    return _mouse_enabled ? ((neos_and_amiga_buttons & 2) ? 0xff : 0) : 0xff;
}

/* Some prototypes are needed */
static int mouse_amiga_write_snapshot(struct snapshot_s *s, int port);
static int mouse_amiga_read_snapshot(struct snapshot_s *s, int port);

static joyport_t mouse_amiga_joyport_device = {
    "Mouse (Amiga)",
    IDGS_MOUSE_AMIGA,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_OPTIONAL,
    joyport_mouse_enable,
    joyport_mouse_poll_value,
    NULL,               /* no store digital */
    joyport_mouse_neos_amiga_st_read_potx,
    joyport_mouse_amiga_st_read_poty,
    mouse_amiga_write_snapshot,
    mouse_amiga_read_snapshot
};

/* Some prototypes are needed */
static int mouse_cx22_write_snapshot(struct snapshot_s *s, int port);
static int mouse_cx22_read_snapshot(struct snapshot_s *s, int port);

static joyport_t mouse_cx22_joyport_device = {
    "Mouse (CX-22)",
    IDGS_MOUSE_CX22,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_OPTIONAL,
    joyport_mouse_enable,
    joyport_mouse_poll_value,
    NULL,               /* no store digital */
    NULL,               /* no read pot x */
    NULL,               /* no read pot y */
    mouse_cx22_write_snapshot,
    mouse_cx22_read_snapshot
};

/* Some prototypes are needed */
static int mouse_st_write_snapshot(struct snapshot_s *s, int port);
static int mouse_st_read_snapshot(struct snapshot_s *s, int port);

static joyport_t mouse_st_joyport_device = {
    "Mouse (Atari ST)",
    IDGS_MOUSE_ATARI_ST,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_OPTIONAL,
    joyport_mouse_enable,
    joyport_mouse_poll_value,
    NULL,               /* no store digital */
    joyport_mouse_neos_amiga_st_read_potx,
    joyport_mouse_amiga_st_read_poty,
    mouse_st_write_snapshot,
    mouse_st_read_snapshot
};

static BYTE joyport_mouse_smart_value(int port)
{
    BYTE retval = 0xff;

    if (_mouse_enabled) {
        retval = (BYTE)((~mouse_digital_val) & smart_mouse_read());
        if (retval != (BYTE)~mouse_digital_val) {
            joyport_display_joyport(mt_to_id(mouse_type), (BYTE)(~retval));
        }
    }
    return retval;
}

/* Some prototypes are needed */
static int mouse_smart_write_snapshot(struct snapshot_s *s, int port);
static int mouse_smart_read_snapshot(struct snapshot_s *s, int port);

static joyport_t mouse_smart_joyport_device = {
    "Mouse (SmartMouse)",
    IDGS_MOUSE_SMART,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_REQUIRED,
    joyport_mouse_enable,
    joyport_mouse_smart_value,
    smart_mouse_store,
    mouse_get_1351_x,
    mouse_get_1351_y,
    mouse_smart_write_snapshot,
    mouse_smart_read_snapshot
};

static BYTE joyport_mouse_micromys_value(int port)
{
    BYTE retval = 0xff;

    if (_mouse_enabled) {
        retval = (BYTE)((~mouse_digital_val) & micromys_mouse_read());
        if (retval != (BYTE)~mouse_digital_val) {
            joyport_display_joyport(mt_to_id(mouse_type), (BYTE)(~retval));
        }
    }
    return retval;
}

/* Some prototypes are needed */
static int mouse_micromys_write_snapshot(struct snapshot_s *s, int port);
static int mouse_micromys_read_snapshot(struct snapshot_s *s, int port);

static joyport_t mouse_micromys_joyport_device = {
    "Mouse (Micromys)",
    IDGS_MOUSE_MICROMYS,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_REQUIRED,
    joyport_mouse_enable,
    joyport_mouse_micromys_value,
    NULL,               /* no store digital */
    mouse_get_1351_x,
    mouse_get_1351_y,
    mouse_micromys_write_snapshot,
    mouse_micromys_read_snapshot
};

static BYTE joyport_koalapad_pot_x(void)
{
    return _mouse_enabled ? (BYTE)(255 - mouse_get_paddle_x()) : 0xff;
}

/* Some prototypes are needed */
static int koalapad_write_snapshot(struct snapshot_s *s, int port);
static int koalapad_read_snapshot(struct snapshot_s *s, int port);

static joyport_t koalapad_joyport_device = {
    "KoalaPad",
    IDGS_KOALAPAD,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_REQUIRED,
    joyport_mouse_enable,
    joyport_mouse_value,
    NULL,               /* no store digital */
    joyport_koalapad_pot_x,
    mouse_get_paddle_y,
    koalapad_write_snapshot,
    koalapad_read_snapshot
};

static int mouse_joyport_register(void)
{
    if (joyport_device_register(JOYPORT_ID_PADDLES, &paddles_joyport_device) < 0) {
        return -1;
    }
    if (joyport_device_register(JOYPORT_ID_MOUSE_1351, &mouse_1351_joyport_device) < 0) {
        return -1;
    }
    if (joyport_device_register(JOYPORT_ID_MOUSE_NEOS, &mouse_neos_joyport_device) < 0) {
        return -1;
    }
    if (joyport_device_register(JOYPORT_ID_MOUSE_AMIGA, &mouse_amiga_joyport_device) < 0) {
        return -1;
    }
    if (joyport_device_register(JOYPORT_ID_MOUSE_CX22, &mouse_cx22_joyport_device) < 0) {
        return -1;
    }
    if (joyport_device_register(JOYPORT_ID_MOUSE_ST, &mouse_st_joyport_device) < 0) {
        return -1;
    }
    if (joyport_device_register(JOYPORT_ID_MOUSE_SMART, &mouse_smart_joyport_device) < 0) {
        return -1;
    }
    if (joyport_device_register(JOYPORT_ID_MOUSE_MICROMYS, &mouse_micromys_joyport_device) < 0) {
        return -1;
    }
    return joyport_device_register(JOYPORT_ID_KOALAPAD, &koalapad_joyport_device);
}

/* --------------------------------------------------------- */
/* Resources & cmdline */

static int set_mouse_enabled(int val, void *param)
{
    if (_mouse_enabled == val) {
        return 0;
    }

    _mouse_enabled = val ? 1 : 0;
    mousedrv_mouse_changed();
    latest_x = (SWORD)mousedrv_get_x();
    last_mouse_x = latest_x;
    latest_y = (SWORD)mousedrv_get_y();
    last_mouse_y = latest_y;
    neos_lastx = (BYTE)(mousedrv_get_x() >> 1);
    neos_lasty = (BYTE)(mousedrv_get_y() >> 1);
    latest_os_ts = 0;
    if (mouse_type != -1) {
        joyport_display_joyport(mt_to_id(mouse_type), 0);
    }
    return 0;
}

static int set_smart_mouse_rtc_save(int val, void *param)
{
    ds1202_rtc_save = val ? 1 : 0;

    return 0;
}

#ifdef ANDROID_COMPILE
#define MOUSE_ENABLE_DEFAULT  1
#else
#define MOUSE_ENABLE_DEFAULT  0
#endif

static const resource_int_t resources_int[] = {
    { "Mouse", MOUSE_ENABLE_DEFAULT, RES_EVENT_SAME, NULL,
      &_mouse_enabled, set_mouse_enabled, NULL },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_extra_int[] = {
    { "SmartMouseRTCSave", 0, RES_EVENT_SAME, NULL,
      &ds1202_rtc_save, set_smart_mouse_rtc_save, NULL },
    RESOURCE_INT_LIST_END
};

static mouse_func_t mouse_funcs =
{
    mouse_button_left,
    mouse_button_right,
    mouse_button_middle,
    mouse_button_up,
    mouse_button_down
};

int mouse_resources_init(void)
{
    if (mouse_joyport_register() < 0) {
        return -1;
    }
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }

    if (resources_register_int(resources_extra_int) < 0) {
        return -1;
    }

    return mousedrv_resources_init(&mouse_funcs);
}

static const cmdline_option_t cmdline_options[] = {
    { "-mouse", SET_RESOURCE, 0,
      NULL, NULL, "Mouse", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_MOUSE_GRAB,
      NULL, NULL },
    { "+mouse", SET_RESOURCE, 0,
      NULL, NULL, "Mouse", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_MOUSE_GRAB,
      NULL, NULL },
    CMDLINE_LIST_END
};

static const cmdline_option_t cmdline_extra_option[] = {
    { "-smartmousertcsave", SET_RESOURCE, 0,
      NULL, NULL, "SmartMouseRTCSave", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_SMART_MOUSE_RTC_SAVE,
      NULL, NULL },
    { "+smartmousertcsave", SET_RESOURCE, 0,
      NULL, NULL, "SmartMouseRTCSave", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_SMART_MOUSE_RTC_SAVE,
      NULL, NULL },
    CMDLINE_LIST_END
};

int mouse_cmdline_options_init(void)
{
    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    if (cmdline_register_options(cmdline_extra_option) < 0) {
        return -1;
    }

    return mousedrv_cmdline_options_init();
}

void mouse_init(void)
{
    emu_units_per_os_units = (float)(machine_get_cycles_per_second() / vsyncarch_frequency());
    update_limit = (int)(machine_get_cycles_per_frame() / 31 / 2);
#ifdef DEBUG_MOUSE
    mouse_log = log_open("Mouse");
    log_message(mouse_log, "cpu cycles / time unit %.5f",
                emu_units_per_os_units);
#endif

    neos_and_amiga_buttons = 0;
    neos_prev = 0xff;
    mousedrv_init();
    clk_guard_add_callback(maincpu_clk_guard, clk_overflow_callback, NULL);
}

void mouse_shutdown(void)
{
    if (ds1202) {
        ds1202_1302_destroy(ds1202, ds1202_rtc_save);
        ds1202 = NULL;
    }
}

/* --------------------------------------------------------- */
/* Main API */

static void mouse_button_left(int pressed)
{
    BYTE old_val = mouse_digital_val;
    BYTE joypin = (((mouse_type == MOUSE_TYPE_PADDLE) || (mouse_type == MOUSE_TYPE_KOALAPAD)) ? 4 : 16);

    if (pressed) {
        mouse_digital_val |= joypin;
    } else {
        mouse_digital_val &= (BYTE)~joypin;
    }
    if (old_val == mouse_digital_val || mouse_type == -1) {
        return;
    }
    joyport_display_joyport(mt_to_id(mouse_type), mouse_digital_val);
}

static void mouse_button_right(int pressed)
{
    BYTE old_val = mouse_digital_val;

    switch (mouse_type) {
        case MOUSE_TYPE_1351:
        case MOUSE_TYPE_SMART:
        case MOUSE_TYPE_MICROMYS:
            /* "joystick up" */
            if (pressed) {
                mouse_digital_val |= 1;
            } else {
                mouse_digital_val &= (BYTE)~1;
            }
            break;
        case MOUSE_TYPE_KOALAPAD:
        case MOUSE_TYPE_PADDLE:
            /* "joystick right" */
            if (pressed) {
                mouse_digital_val |= 8;
            } else {
                mouse_digital_val &= (BYTE)~8;
            }
            break;
        case MOUSE_TYPE_NEOS:
        case MOUSE_TYPE_AMIGA:
        case MOUSE_TYPE_ST:
            if (pressed) {
                neos_and_amiga_buttons |= 1;
            } else {
                neos_and_amiga_buttons &= ~1;
            }
            break;
        default:
            break;
    }
    if (old_val == mouse_digital_val || mouse_type == -1) {
        return;
    }
    joyport_display_joyport(mt_to_id(mouse_type), mouse_digital_val);
}

static void mouse_button_middle(int pressed)
{
    BYTE old_val = mouse_digital_val;

    switch (mouse_type) {
        case MOUSE_TYPE_MICROMYS:
            if (pressed) {
                mouse_digital_val |= 2;
            } else {
                mouse_digital_val &= (BYTE)~2;
            }
            break;
        case MOUSE_TYPE_AMIGA:
        case MOUSE_TYPE_ST:
            if (pressed) {
                neos_and_amiga_buttons |= 2;
            } else {
                neos_and_amiga_buttons &= ~2;
            }
            break;
        default:
            break;
    }
    if (old_val == mouse_digital_val || mouse_type == -1) {
        return;
    }
    joyport_display_joyport(mt_to_id(mouse_type), mouse_digital_val);
}

static void mouse_button_up(int pressed)
{
    switch (mouse_type) {
        case MOUSE_TYPE_MICROMYS:
            if (pressed) {
                if (!up_down_counter) {
                    up_down_pulse_end = maincpu_clk;
                }
                up_down_counter += 2;
            }
            break;
        default:
            break;
    }
}

static void mouse_button_down(int pressed)
{
    switch (mouse_type) {
        case MOUSE_TYPE_MICROMYS:
            if (pressed) {
                if (!up_down_counter) {
                    up_down_pulse_end = maincpu_clk;
                }
                up_down_counter -= 2;
            }
            break;
        default:
            break;
    }
}

void smart_mouse_store(BYTE val)
{
    ds1202_1302_set_lines(ds1202, !(val & 8), !!(val & 2), !!(val & 4));
}

BYTE smart_mouse_read(void)
{
    return ds1202_1302_read_data_line(ds1202) ? 0xff : 0xfb;
}

/* --------------------------------------------------------- */

static int write_mouse_digital_val_snapshot(snapshot_module_t *m)
{
    return SMW_B(m, mouse_digital_val);
}

static int read_mouse_digital_val_snapshot(snapshot_module_t *m)
{
    return SMR_B(m, &mouse_digital_val);
}

static int write_paddle_val_snapshot(snapshot_module_t *m)
{
    if (0
        || SMW_B(m, paddle_val[2]) < 0
        || SMW_B(m, paddle_val[3]) < 0
        || SMW_W(m, (WORD)paddle_old[2]) < 0
        || SMW_W(m, (WORD)paddle_old[3]) < 0) {
        return -1;
    }
    return 0;
}

static int read_paddle_val_snapshot(snapshot_module_t *m)
{
    WORD paddle_old2;
    WORD paddle_old3;

    if (0
        || SMR_B(m, &paddle_val[2]) < 0
        || SMR_B(m, &paddle_val[3]) < 0
        || SMR_W(m, &paddle_old2) < 0
        || SMR_W(m, &paddle_old3) < 0) {
        return -1;
    }
    paddle_old[2] = (SWORD)paddle_old2;
    paddle_old[3] = (SWORD)paddle_old3;

    return 0;
}

static int write_poll_val_snapshot(snapshot_module_t *m)
{
    if (0
        || SMW_B(m, quadrature_x) < 0
        || SMW_B(m, quadrature_y) < 0
        || SMW_B(m, polled_joyval) < 0
        || SMW_W(m, (WORD)latest_x) < 0
        || SMW_W(m, (WORD)latest_y) < 0
        || SMW_DW(m, (DWORD)last_mouse_x) < 0
        || SMW_DW(m, (DWORD)last_mouse_y) < 0
        || SMW_DW(m, (DWORD)sx) < 0
        || SMW_DW(m, (DWORD)sy) < 0
        || SMW_DW(m, (DWORD)update_limit) < 0
        || SMW_DW(m, (DWORD)latest_os_ts) < 0
        || SMW_DB(m, (double)emu_units_per_os_units) < 0
        || SMW_DW(m, (DWORD)next_update_x_emu_ts) < 0
        || SMW_DW(m, (DWORD)next_update_y_emu_ts) < 0
        || SMW_DW(m, (DWORD)update_x_emu_iv) < 0
        || SMW_DW(m, (DWORD)update_y_emu_iv) < 0) {
        return -1;
    }
    return 0;
}

static int read_poll_val_snapshot(snapshot_module_t *m)
{
    WORD tmp_latest_x;
    WORD tmp_latest_y;
    double tmp_db;
    DWORD tmpc1;
    DWORD tmpc2;
    DWORD tmpc3;
    DWORD tmpc4;

    if (0
        || SMR_B(m, &quadrature_x) < 0
        || SMR_B(m, &quadrature_y) < 0
        || SMR_B(m, &polled_joyval) < 0
        || SMR_W(m, &tmp_latest_x) < 0
        || SMR_W(m, &tmp_latest_y) < 0
        || SMR_DW_INT(m, &last_mouse_x) < 0
        || SMR_DW_INT(m, &last_mouse_y) < 0
        || SMR_DW_INT(m, &sx) < 0
        || SMR_DW_INT(m, &sy) < 0
        || SMR_DW_INT(m, &update_limit) < 0
        || SMR_DW_UL(m, &latest_os_ts) < 0
        || SMR_DB(m, &tmp_db) < 0
        || SMR_DW(m, &tmpc1) < 0
        || SMR_DW(m, &tmpc2) < 0
        || SMR_DW(m, &tmpc3) < 0
        || SMR_DW(m, &tmpc4) < 0) {
        return -1;
    }

    latest_x = (SWORD)tmp_latest_x;
    latest_y = (SWORD)tmp_latest_y;
    emu_units_per_os_units = (float)tmp_db;
    next_update_x_emu_ts = (CLOCK)tmpc1;
    next_update_y_emu_ts = (CLOCK)tmpc2;
    update_x_emu_iv = (CLOCK)tmpc3;
    update_y_emu_iv = (CLOCK)tmpc4;

    return 0;
}

static int write_neos_and_amiga_val_snapshot(snapshot_module_t *m)
{
    return SMW_DW(m, (DWORD)neos_and_amiga_buttons);
}

static int read_neos_and_amiga_val_snapshot(snapshot_module_t *m)
{
    return SMR_DW_INT(m, &neos_and_amiga_buttons);
}

/* --------------------------------------------------------- */

/* PADDLES snapshot module format:

   type  | name               | description
   ----------------------------------------
   BYTE  | digital value      | digital pins return value
   BYTE  | paddle value 2     | paddle value 2
   BYTE  | paddle value 3     | paddle value 3
   BYTE  | old paddle value 2 | old paddle value 2
   BYTE  | old paddle value 3 | old paddle value 3
 */

static char paddles_snap_module_name[] = "PADDLES";
#define PADDLES_VER_MAJOR   0
#define PADDLES_VER_MINOR   0

static int paddles_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, paddles_snap_module_name, PADDLES_VER_MAJOR, PADDLES_VER_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (write_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (write_paddle_val_snapshot(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int paddles_read_snapshot(struct snapshot_s *s, int port)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, paddles_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > PADDLES_VER_MAJOR || minor_version > PADDLES_VER_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (read_paddle_val_snapshot(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

/* --------------------------------------------------------- */

/* MOUSE_1351 snapshot module format:

   type   | name                   | description
   ---------------------------------------------
   BYTE   | digital value          | digital pins return value
   BYTE   | quadrature X           | quadrature X
   BYTE   | quadrature Y           | quadrature Y
   BYTE   | polled joyval          | polled joyval
   WORD   | latest X               | latest X
   WORD   | latest Y               | latest Y
   DWORD  | last mouse X           | last mouse X
   DWORD  | last mouse Y           | last mouse Y
   DWORD  | sx                     | SX
   DWORD  | sy                     | SY
   DWORD  | update limit           | update limit
   DWORD  | latest os ts           | latest os ts
   DOUBLE | emu units per os units | emu units per os units
   DWORD  | next update x emu ts   | next update X emu ts
   DWORD  | next update y emu ts   | next update Y emu ts
   DWORD  | update x emu iv        | update X emu IV
   DWORD  | update y emu iv        | update Y emu IV
 */

static char mouse_1351_snap_module_name[] = "MOUSE_1351";
#define MOUSE_1351_VER_MAJOR   0
#define MOUSE_1351_VER_MINOR   0

static int mouse_1351_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, mouse_1351_snap_module_name, MOUSE_1351_VER_MAJOR, MOUSE_1351_VER_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (write_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (write_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int mouse_1351_read_snapshot(struct snapshot_s *s, int port)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, mouse_1351_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > MOUSE_1351_VER_MAJOR || minor_version > MOUSE_1351_VER_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (read_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

/* --------------------------------------------------------- */

/* MOUSE_NEOS snapshot module format:

   type  | name                 | description
   ------------------------------------------
   BYTE  | digital value        | digital pins return value
   DWORD | buttons              | buttons state
   BYTE  | neos x               | neos X
   BYTE  | neos y               | neos Y
   BYTE  | neos last x          | neos last X
   BYTE  | neos last y          | neos last Y
   DWORD | neos state           | state
   DWORD | neos prev            | previous state
   DWORD | last trigger         | last trigger clock
   DWORD | neos time out cycles | time out cycles
 */

static char mouse_neos_snap_module_name[] = "MOUSE_NEOS";
#define MOUSE_NEOS_VER_MAJOR   0
#define MOUSE_NEOS_VER_MINOR   0

static int mouse_neos_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, mouse_neos_snap_module_name, MOUSE_NEOS_VER_MAJOR, MOUSE_NEOS_VER_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (write_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (write_neos_and_amiga_val_snapshot(m) < 0) {
        goto fail;
    }

    if (0
        || SMW_B(m, neos_x) < 0
        || SMW_B(m, neos_y) < 0
        || SMW_B(m, neos_lastx) < 0
        || SMW_B(m, neos_lasty) < 0
        || SMW_DW(m, (DWORD)neos_state) < 0
        || SMW_DW(m, (DWORD)neos_prev) < 0
        || SMW_DW(m, (DWORD)neos_last_trigger) < 0
        || SMW_DW(m, (DWORD)neos_time_out_cycles) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int mouse_neos_read_snapshot(struct snapshot_s *s, int port)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    DWORD tmpc1;
    DWORD tmpc2;
    int tmp_neos_state;

    m = snapshot_module_open(s, mouse_neos_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept higher versions than current */
    if (major_version > MOUSE_NEOS_VER_MAJOR || minor_version > MOUSE_NEOS_VER_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (read_neos_and_amiga_val_snapshot(m) < 0) {
        goto fail;
    }

    if (0
        || SMR_B(m, &neos_x) < 0
        || SMR_B(m, &neos_y) < 0
        || SMR_B(m, &neos_lastx) < 0
        || SMR_B(m, &neos_lasty) < 0
        || SMR_DW_INT(m, &tmp_neos_state) < 0
        || SMR_DW_INT(m, &neos_prev) < 0
        || SMR_DW(m, &tmpc1) < 0
        || SMR_DW(m, &tmpc2) < 0) {
        goto fail;
    }

    neos_last_trigger = (CLOCK)tmpc1;
    neos_time_out_cycles = (CLOCK)tmpc2;
    neos_state = tmp_neos_state;

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

/* --------------------------------------------------------- */

/* MOUSE_AMIGA snapshot module format:

   type   | name                   | description
   ---------------------------------------------
   BYTE   | digital value          | digital pins return value
   BYTE   | quadrature X           | quadrature X
   BYTE   | quadrature Y           | quadrature Y
   BYTE   | polled joyval          | polled joyval
   WORD   | latest X               | latest X
   WORD   | latest Y               | latest Y
   DWORD  | last mouse X           | last mouse X
   DWORD  | last mouse Y           | last mouse Y
   DWORD  | sx                     | SX
   DWORD  | sy                     | SY
   DWORD  | update limit           | update limit
   DWORD  | latest os ts           | latest os ts
   DOUBLE | emu units per os units | emu units per os units
   DWORD  | next update x emu ts   | next update X emu ts
   DWORD  | next update y emu ts   | next update Y emu ts
   DWORD  | update x emu iv        | update X emu IV
   DWORD  | update y emu iv        | update Y emu IV
   DWORD  | buttons                | buttons state
 */

static char mouse_amiga_snap_module_name[] = "MOUSE_AMIGA";
#define MOUSE_AMIGA_VER_MAJOR   0
#define MOUSE_AMIGA_VER_MINOR   0

static int mouse_amiga_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, mouse_amiga_snap_module_name, MOUSE_AMIGA_VER_MAJOR, MOUSE_AMIGA_VER_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (write_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (write_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    if (write_neos_and_amiga_val_snapshot(m) < 0) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int mouse_amiga_read_snapshot(struct snapshot_s *s, int port)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, mouse_amiga_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > MOUSE_AMIGA_VER_MAJOR || minor_version > MOUSE_AMIGA_VER_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (read_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    if (read_neos_and_amiga_val_snapshot(m) < 0) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

/* --------------------------------------------------------- */

/* MOUSE_CX22 snapshot module format:

   type   | name                   | description
   ---------------------------------------------
   BYTE   | digital value          | digital pins return value
   BYTE   | quadrature X           | quadrature X
   BYTE   | quadrature Y           | quadrature Y
   BYTE   | polled joyval          | polled joyval
   WORD   | latest X               | latest X
   WORD   | latest Y               | latest Y
   DWORD  | last mouse X           | last mouse X
   DWORD  | last mouse Y           | last mouse Y
   DWORD  | sx                     | SX
   DWORD  | sy                     | SY
   DWORD  | update limit           | update limit
   DWORD  | latest os ts           | latest os ts
   DOUBLE | emu units per os units | emu units per os units
   DWORD  | next update x emu ts   | next update X emu ts
   DWORD  | next update y emu ts   | next update Y emu ts
   DWORD  | update x emu iv        | update X emu IV
   DWORD  | update y emu iv        | update Y emu IV
 */

static char mouse_cx22_snap_module_name[] = "MOUSE_CX22";
#define MOUSE_CX22_VER_MAJOR   0
#define MOUSE_CX22_VER_MINOR   0

static int mouse_cx22_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, mouse_cx22_snap_module_name, MOUSE_CX22_VER_MAJOR, MOUSE_CX22_VER_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (write_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (write_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int mouse_cx22_read_snapshot(struct snapshot_s *s, int port)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, mouse_cx22_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > MOUSE_CX22_VER_MAJOR || minor_version > MOUSE_CX22_VER_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (read_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

/* --------------------------------------------------------- */

/* MOUSE_ST snapshot module format:

   type   | name                   | description
   ---------------------------------------------
   BYTE   | digital value          | digital pins return value
   BYTE   | quadrature X           | quadrature X
   BYTE   | quadrature Y           | quadrature Y
   BYTE   | polled joyval          | polled joyval
   WORD   | latest X               | latest X
   WORD   | latest Y               | latest Y
   DWORD  | last mouse X           | last mouse X
   DWORD  | last mouse Y           | last mouse Y
   DWORD  | sx                     | SX
   DWORD  | sy                     | SY
   DWORD  | update limit           | update limit
   DWORD  | latest os ts           | latest os ts
   DOUBLE | emu units per os units | emu units per os units
   DWORD  | next update x emu ts   | next update X emu ts
   DWORD  | next update y emu ts   | next update Y emu ts
   DWORD  | update x emu iv        | update X emu IV
   DWORD  | update y emu iv        | update Y emu IV
   DWORD  | buttons                | buttons state
 */

static char mouse_st_snap_module_name[] = "MOUSE_ST";
#define MOUSE_ST_VER_MAJOR   0
#define MOUSE_ST_VER_MINOR   0

static int mouse_st_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, mouse_st_snap_module_name, MOUSE_ST_VER_MAJOR, MOUSE_ST_VER_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (write_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (write_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    if (write_neos_and_amiga_val_snapshot(m) < 0) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int mouse_st_read_snapshot(struct snapshot_s *s, int port)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, mouse_st_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > MOUSE_ST_VER_MAJOR || minor_version > MOUSE_ST_VER_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (read_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    if (read_neos_and_amiga_val_snapshot(m) < 0) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

/* --------------------------------------------------------- */

/* MOUSE_SMART snapshot module format:

   type   | name                   | description
   ---------------------------------------------
   BYTE   | digital value          | digital pins return value
   BYTE   | quadrature X           | quadrature X
   BYTE   | quadrature Y           | quadrature Y
   BYTE   | polled joyval          | polled joyval
   WORD   | latest X               | latest X
   WORD   | latest Y               | latest Y
   DWORD  | last mouse X           | last mouse X
   DWORD  | last mouse Y           | last mouse Y
   DWORD  | sx                     | SX
   DWORD  | sy                     | SY
   DWORD  | update limit           | update limit
   DWORD  | latest os ts           | latest os ts
   DOUBLE | emu units per os units | emu units per os units
   DWORD  | next update x emu ts   | next update X emu ts
   DWORD  | next update y emu ts   | next update Y emu ts
   DWORD  | update x emu iv        | update X emu IV
   DWORD  | update y emu iv        | update Y emu IV
 */

static char mouse_smart_snap_module_name[] = "MOUSE_SMART";
#define MOUSE_SMART_VER_MAJOR   0
#define MOUSE_SMART_VER_MINOR   0

static int mouse_smart_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, mouse_smart_snap_module_name, MOUSE_SMART_VER_MAJOR, MOUSE_SMART_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (write_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (write_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return ds1202_1302_write_snapshot(ds1202, s);

fail:
    snapshot_module_close(m);
    return -1;
}

static int mouse_smart_read_snapshot(struct snapshot_s *s, int port)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, mouse_smart_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept higher versions than current */
    if (major_version > MOUSE_SMART_VER_MAJOR || minor_version > MOUSE_SMART_VER_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (read_poll_val_snapshot(m) < 0) {
        goto fail;
    }
    snapshot_module_close(m);

    return ds1202_1302_read_snapshot(ds1202, s);

fail:
    snapshot_module_close(m);
    return -1;
}

/* --------------------------------------------------------- */

/* MOUSE_MICROMYS snapshot module format:

   type   | name                   | description
   ---------------------------------------------
   BYTE   | digital value          | digital pins return value
   BYTE   | quadrature X           | quadrature X
   BYTE   | quadrature Y           | quadrature Y
   BYTE   | polled joyval          | polled joyval
   WORD   | latest X               | latest X
   WORD   | latest Y               | latest Y
   DWORD  | last mouse X           | last mouse X
   DWORD  | last mouse Y           | last mouse Y
   DWORD  | sx                     | SX
   DWORD  | sy                     | SY
   DWORD  | update limit           | update limit
   DWORD  | latest os ts           | latest os ts
   DOUBLE | emu units per os units | emu units per os units
   DWORD  | next update x emu ts   | next update X emu ts
   DWORD  | next update y emu ts   | next update Y emu ts
   DWORD  | update x emu iv        | update X emu IV
   DWORD  | update y emu iv        | update Y emu IV
   DWORD  | up down counter        | up down counter
   DWORD  | up down pulse end      | up down pulse end
 */

static char mouse_micromys_snap_module_name[] = "MOUSE_MICROMYS";
#define MOUSE_MICROMYS_VER_MAJOR   0
#define MOUSE_MICROMYS_VER_MINOR   0

static int mouse_micromys_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, mouse_micromys_snap_module_name, MOUSE_MICROMYS_VER_MAJOR, MOUSE_MICROMYS_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (write_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (write_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    if (0
        || SMW_DW(m, (DWORD)up_down_counter) < 0
        || SMW_DW(m, (DWORD)up_down_pulse_end) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int mouse_micromys_read_snapshot(struct snapshot_s *s, int port)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    DWORD tmpc1;

    m = snapshot_module_open(s, mouse_micromys_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > MOUSE_MICROMYS_VER_MAJOR || minor_version > MOUSE_MICROMYS_VER_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (read_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    if (0
        || SMR_DW_INT(m, &up_down_counter) < 0
        || SMR_DW(m, &tmpc1) < 0) {
        goto fail;
    }

    up_down_pulse_end = (CLOCK)tmpc1;

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

/* --------------------------------------------------------- */

/* KOALAPAD snapshot module format:

   type   | name               | description
   -------------------------------------
   BYTE   | digital value      | digital pins return value
   BYTE   | paddle value 2     | paddle value 2
   BYTE   | paddle value 3     | paddle value 3
   WORD   | old paddle value 2 | old paddle value 2
   WORD   | old paddle value 3 | old paddle value 3
 */

static char koalapad_snap_module_name[] = "KOALAPAD";
#define KOALAPAD_VER_MAJOR   0
#define KOALAPAD_VER_MINOR   0

static int koalapad_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, koalapad_snap_module_name, KOALAPAD_VER_MAJOR, KOALAPAD_VER_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (write_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (write_paddle_val_snapshot(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int koalapad_read_snapshot(struct snapshot_s *s, int port)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, koalapad_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > KOALAPAD_VER_MAJOR || minor_version > KOALAPAD_VER_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_digital_val_snapshot(m) < 0) {
        goto fail;
    }

    if (read_paddle_val_snapshot(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
