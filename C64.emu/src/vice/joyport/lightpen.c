/*
 * lightpen.c - Lightpen/gun emulation
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#if defined(HAVE_MOUSE) && defined(HAVE_LIGHTPEN)

#include "cmdline.h"
#include "joyport.h"
#include "joystick.h"
#include "machine.h"
#include "maincpu.h"
#include "lightpen.h"
#include "resources.h"
#include "snapshot.h"
#include "translate.h"


/* Control port <--> lightpen connections:

   cport | lightpen up | I/O
   -------------------------
     1   | button      |  I
     6   | trigger     |  I

   cport | lightpen left | I/O
   ---------------------------
     3   | button        |  I
     6   | trigger       |  I

   cport | datel pen | I/O
   -----------------------
     3   | button    |  I
     6   | trigger   |  I

   cport | magnum light phaser | I/O
   ---------------------------------
     6   | trigger             |  I
     9   | button              |  I

   cport | stack light rifle | I/O
   -------------------------------
     3   | button            |  I
     6   | trigger           |  I

   cport | inkwell lightpen | I/O
   ------------------------------
     3   | button 1         |  I
     6   | trigger          |  I
     9   | button 2         |  I
 */

/* --------------------------------------------------------- */
/* extern variables */

int lightpen_enabled = 0;

/* --------------------------------------------------------- */
/* static variables/functions */

#define MAX_WINDOW_NUM 1

static BYTE lightpen_value = 0;

static int lightpen_type;

static int lightpen_buttons;
static int lightpen_button_y;
static int lightpen_button_x;

/* Video chip timing callbacks for each window.
   Returns the CLOCK value of the triggering time (or 0 if off screen).
   For x128, window 1 is VICII, window 0 is VDC. Others always use window 0. */
static lightpen_timing_callback_ptr_t chip_timing_callback[MAX_WINDOW_NUM + 1];

/* Machine dependant callback for triggering the lightpen at the given CLOCK.
   x128 needs to trigger both VICII and VDC, others point this to the video chip function itself. */
static lightpen_trigger_callback_ptr_t chip_trigger_callback;

/* Lightpen/gun type */
struct lp_type_s {
    /* PEN needs button to be pressed to register, GUN doesn't */
    enum { PEN, GUN } type;
    /* Buttons: bitmask for joyport 1 pins, with 0x20 for potY and 0x40 for potX */
    BYTE button1;
    BYTE button2;
    /* x/y offsets to add before timing callback */
    int x_offset;
    int y_offset;
};
typedef struct lp_type_s lp_type_t;

/* note: xoffs=24; yoffs=0 gives "pixel perfect" match */
static const lp_type_t lp_type[LIGHTPEN_TYPE_NUM] = {
    /* Pen with button Up (e.g. Atari CX75) */
    { PEN, 0x00, 0x01, 0, 0 },
    /* Pen with button Left */
    { PEN, 0x00, 0x04, 0, 0 },
    /* Datel Pen */
    { PEN, 0x00, 0x04, 20, -5 },
    /* Magnum Light Phaser */
    { GUN, 0x20, 0x00, 20, -10 },
    /* Stack Light Rifle */
    { GUN, 0x04, 0x00, 20, 0 },
    /* Inkwell Lightpen */
    { GUN, 0x04, 0x20, 20, 0 }
};

typedef struct lp_id_s {
    int lp;
    int id;
} lp_id_t;

static lp_id_t lp_id[] = {
    { LIGHTPEN_TYPE_PEN_U,     JOYPORT_ID_LIGHTPEN_U },
    { LIGHTPEN_TYPE_PEN_L,     JOYPORT_ID_LIGHTPEN_L },
    { LIGHTPEN_TYPE_PEN_DATEL, JOYPORT_ID_LIGHTPEN_DATEL },
    { LIGHTPEN_TYPE_GUN_Y,     JOYPORT_ID_LIGHTGUN_Y },
    { LIGHTPEN_TYPE_GUN_L,     JOYPORT_ID_LIGHTGUN_L },
    { LIGHTPEN_TYPE_INKWELL,   JOYPORT_ID_LIGHTPEN_INKWELL },
    { -1,                      -1 }
};

static inline int joyport_id_to_lighpen_type(int id)
{
    int i;

    for (i = 0; lp_id[i].lp != -1; ++i) {
        if (lp_id[i].id == id) {
            return lp_id[i].lp;
        }
    }
    return -1;
}

static inline int lighpen_type_to_joyport_id(int lp)
{
    int i;

    for (i = 0; lp_id[i].lp != -1; ++i) {
        if (lp_id[i].lp == lp) {
            return lp_id[i].id;
        }
    }
    return -1;
}

static inline void lightpen_check_button_mask(BYTE mask, int pressed)
{
    int id;
    BYTE old_value = lightpen_value;

    if (!mask) {
        return;
    }

    if (pressed) {
        lightpen_value |= mask;
    } else {
        lightpen_value &= (BYTE)~mask;
    }

    if (lightpen_value == old_value) {
        return;
    }

    id = lighpen_type_to_joyport_id(lightpen_type);
    if (id == -1) {
        return;
    }
    joyport_display_joyport(id, lightpen_value);
}

static inline void lightpen_update_buttons(int buttons)
{
    lightpen_buttons = buttons;

    lightpen_button_y = ((((lp_type[lightpen_type].button1 & 0x20) == 0x20) && (buttons & LP_HOST_BUTTON_1))
                         || (((lp_type[lightpen_type].button2 & 0x20) == 0x20) && (buttons & LP_HOST_BUTTON_2)))
                        ? 1 : 0;

    lightpen_button_x = ((((lp_type[lightpen_type].button1 & 0x40) == 0x40) && (buttons & LP_HOST_BUTTON_1))
                         || (((lp_type[lightpen_type].button2 & 0x40) == 0x40) && (buttons & LP_HOST_BUTTON_2)))
                        ? 1 : 0;

    lightpen_check_button_mask((BYTE)(lp_type[lightpen_type].button1 & 0xf), buttons & LP_HOST_BUTTON_1);
    lightpen_check_button_mask((BYTE)(lp_type[lightpen_type].button2 & 0xf), buttons & LP_HOST_BUTTON_2);
}

/* --------------------------------------------------------- */

/* Some prototypes are needed */
static int lightpen_write_snapshot(struct snapshot_s *s, int port);
static int lightpen_read_snapshot(struct snapshot_s *s, int port);

static int joyport_lightpen_enable(int port, int val)
{
    lightpen_enabled = val ? 1 : 0;

    if (!val) {
        lightpen_type = -1;
        return 0;
    }

    lightpen_type = joyport_id_to_lighpen_type(val);

    if (lightpen_type == -1) {
        return -1;
    }

    return 0;
}

static BYTE lightpen_digital_val(int port)
{
    return (BYTE)~lightpen_value;
}

static BYTE lightpen_read_button_y(void)
{
    return (lightpen_enabled && lightpen_button_y) ? 0x00 : 0xff;
}

static BYTE lightpen_read_button_x(void)
{
    return (lightpen_enabled && lightpen_button_x) ? 0x00 : 0xff;
}

static joyport_t lightpen_u_joyport_device = {
    "Light Pen (up trigger)",
    IDGS_LIGHTPEN_UP,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_LIGHTPEN,
    JOYPORT_POT_OPTIONAL,
    joyport_lightpen_enable,
    lightpen_digital_val,
    NULL,                       /* no store digital */
    lightpen_read_button_x,
    lightpen_read_button_y,
    lightpen_write_snapshot,
    lightpen_read_snapshot
};

static joyport_t lightpen_l_joyport_device = {
    "Light Pen (left trigger)",
    IDGS_LIGHTPEN_LEFT,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_LIGHTPEN,
    JOYPORT_POT_OPTIONAL,
    joyport_lightpen_enable,
    lightpen_digital_val,
    NULL,                       /* no store digital */
    lightpen_read_button_x,
    lightpen_read_button_y,
    lightpen_write_snapshot,
    lightpen_read_snapshot
};

static joyport_t lightpen_datel_joyport_device = {
    "Datel Light Pen",
    IDGS_DATEL_LIGHTPEN,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_LIGHTPEN,
    JOYPORT_POT_OPTIONAL,
    joyport_lightpen_enable,
    lightpen_digital_val,
    NULL,                       /* no store digital */
    lightpen_read_button_x,
    lightpen_read_button_y,
    lightpen_write_snapshot,
    lightpen_read_snapshot
};

static joyport_t magnum_light_phaser_joyport_device = {
    "Magnum Light Phaser",
    IDGS_MAGNUM_LIGHT_PHASER,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_LIGHTPEN,
    JOYPORT_POT_OPTIONAL,
    joyport_lightpen_enable,
    lightpen_digital_val,
    NULL,                       /* no store digital */
    lightpen_read_button_x,
    lightpen_read_button_y,
    lightpen_write_snapshot,
    lightpen_read_snapshot
};

static joyport_t stack_light_rifle_joyport_device = {
    "Stack Light Rifle",
    IDGS_STACK_LIGHT_RIFLE,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_LIGHTPEN,
    JOYPORT_POT_OPTIONAL,
    joyport_lightpen_enable,
    lightpen_digital_val,
    NULL,                       /* no store digital */
    lightpen_read_button_x,
    lightpen_read_button_y,
    lightpen_write_snapshot,
    lightpen_read_snapshot
};

static joyport_t inkwell_lightpen_joyport_device = {
    "Inkwell Light Pen",
    IDGS_INKWELL_LIGHTPEN,
    JOYPORT_RES_ID_MOUSE,
    JOYPORT_IS_LIGHTPEN,
    JOYPORT_POT_OPTIONAL,
    joyport_lightpen_enable,
    lightpen_digital_val,
    NULL,                       /* no store digital */
    lightpen_read_button_x,
    lightpen_read_button_y,
    lightpen_write_snapshot,
    lightpen_read_snapshot
};

static int lightpen_joyport_register(void)
{
    if (joyport_device_register(JOYPORT_ID_LIGHTPEN_U, &lightpen_u_joyport_device) < 0) {
        return -1;
    }
    if (joyport_device_register(JOYPORT_ID_LIGHTPEN_L, &lightpen_l_joyport_device) < 0) {
        return -1;
    }
    if (joyport_device_register(JOYPORT_ID_LIGHTPEN_DATEL, &lightpen_datel_joyport_device) < 0) {
        return -1;
    }
    if (joyport_device_register(JOYPORT_ID_LIGHTGUN_Y, &magnum_light_phaser_joyport_device) < 0) {
        return -1;
    }
    if (joyport_device_register(JOYPORT_ID_LIGHTGUN_L, &stack_light_rifle_joyport_device) < 0) {
        return -1;
    }
    return joyport_device_register(JOYPORT_ID_LIGHTPEN_INKWELL, &inkwell_lightpen_joyport_device);
}

/* --------------------------------------------------------- */

int lightpen_resources_init(void)
{
    if (lightpen_joyport_register() < 0) {
        return -1;
    }
    return 0;
}

/* --------------------------------------------------------- */
/* Main API */

void lightpen_init(void)
{
    int i;

    for (i = 0; i < (MAX_WINDOW_NUM + 1); ++i) {
        chip_timing_callback[i] = NULL;
    }

    chip_trigger_callback = NULL;
}

int lightpen_register_timing_callback(lightpen_timing_callback_ptr_t timing_callback, int window)
{
    if ((window < 0) || (window > MAX_WINDOW_NUM)) {
        return -1;
    }

    chip_timing_callback[window] = timing_callback;
    return 0;
}

int lightpen_register_trigger_callback(lightpen_trigger_callback_ptr_t trigger_callback)
{
    chip_trigger_callback = trigger_callback;
    return 0;
}

/* Update lightpen coordinates and button status. Called at the end of each frame.
   For x128, window 1 is VICII, window 0 is VDC. Others always use window 0.
   x and y are the canvas coordinates; double size, hwscale and offsets are removed in the arch side.
   Negative values of x and/or y can be used to indicate that the pointer is off the (emulated) screen. */
void lightpen_update(int window, int x, int y, int buttons)
{
    CLOCK pulse_time;

    if ((window < 0) || (window > MAX_WINDOW_NUM)) {
        return;
    }

    if ((!lightpen_enabled) || (chip_timing_callback[window] == NULL) || (chip_trigger_callback == NULL)) {
        return;
    }

    lightpen_update_buttons(buttons);

    if ((x < 0) || (y < 0)) {
        return;
    }

    x += lp_type[lightpen_type].x_offset;
    y += lp_type[lightpen_type].y_offset;

    if ((x < 0) || (y < 0)) {
        return;
    }

    if ((lp_type[lightpen_type].type == PEN) && !(buttons & LP_HOST_BUTTON_1)) {
        return;
    }

    pulse_time = chip_timing_callback[window](x, y);

    if (pulse_time > 0) {
        chip_trigger_callback(pulse_time);
    }
}

/* --------------------------------------------------------- */

/* LIGHTPEN snapshot module format:

   type  | name     | description
   ------------------------------
   BYTE  | value    | lightpen return value
   BYTE  | type     | lightpen type
   DWORD | buttons  | buttons state
   DWORD | button y | button Y state
   DWORD | button x | button X state
 */

static char snap_module_name[] = "LIGHTPEN";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

static int lightpen_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, lightpen_value) < 0
        || SMW_B(m, (BYTE)lightpen_type) < 0
        || SMW_DW(m, (DWORD)lightpen_buttons) < 0
        || SMW_DW(m, (DWORD)lightpen_button_y) < 0
        || SMW_DW(m, (DWORD)lightpen_button_x) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

static int lightpen_read_snapshot(struct snapshot_s *s, int port)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || SMR_B(m, &lightpen_value) < 0
        || SMR_B_INT(m, &lightpen_type) < 0
        || SMR_DW_INT(m, &lightpen_buttons) < 0
        || SMR_DW_INT(m, &lightpen_button_y) < 0
        || SMR_DW_INT(m, &lightpen_button_x) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
#endif
