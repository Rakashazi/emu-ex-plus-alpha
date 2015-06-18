/*
 * userport_joystick.c - Generic userport joystick emulation.
 *
 * Written by
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
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "joystick.h"
#include "machine.h"
#include "resources.h"
#include "translate.h"
#include "types.h"
#include "userport_joystick.h"

int userport_joystick_enable = 0;
int userport_joystick_type = 0;

static int userport_joystick_cga_select = 0;
static BYTE userport_joystick_button_sp2 = 0xff;

/* ------------------------------------------------------------------------- */

static int set_userport_joystick_enable(int val, void *param)
{
    userport_joystick_enable = val ? 1 : 0;

    return 0;
}

static int set_userport_joystick_type(int val, void *param)
{
    switch (val) {
        case USERPORT_JOYSTICK_CGA:
        case USERPORT_JOYSTICK_PET:
        case USERPORT_JOYSTICK_HUMMER:
        case USERPORT_JOYSTICK_OEM:
            break;
        case USERPORT_JOYSTICK_HIT:
        case USERPORT_JOYSTICK_KINGSOFT:
        case USERPORT_JOYSTICK_STARBYTE:
            if (machine_class == VICE_MACHINE_C64
                || machine_class == VICE_MACHINE_C128
                || machine_class == VICE_MACHINE_C64SC
                || machine_class == VICE_MACHINE_SCPU64) {
                break;
            }
            return -1;
        default:
            return -1;
    }

    userport_joystick_type = val;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "UserportJoy", 0, RES_EVENT_NO, NULL,
      &userport_joystick_enable, set_userport_joystick_enable, NULL },
    { "UserportJoyType", USERPORT_JOYSTICK_CGA, RES_EVENT_NO, NULL,
      &userport_joystick_type, set_userport_joystick_type, NULL },
    { NULL }
};

int userport_joystick_resources_init(void)
{
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-userportjoy", SET_RESOURCE, 0,
      NULL, NULL, "UserportJoy", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_USERPORT_JOY,
      NULL, NULL },
    { "+userportjoy", SET_RESOURCE, 0,
      NULL, NULL, "UserportJoy", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_USERPORT_JOY,
      NULL, NULL },
    { "-userportjoytype", SET_RESOURCE, 1,
      NULL, NULL, "UserportJoyType", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_SET_USERPORT_JOY_TYPE,
      NULL, NULL },
    { NULL }
};

int userport_joystick_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

/* FIXME: direction and unused bits are not yet taken into account */

/* C64/C128 only */
void userport_joystick_store_pa2(BYTE value)
{
}

void userport_joystick_store_pbx(BYTE value)
{
    if (userport_joystick_enable && userport_joystick_type == USERPORT_JOYSTICK_CGA) {
        userport_joystick_cga_select = (value & 0x80) ? 0 : 1;
    }
}

/* C64/C128 only */
void userport_joystick_store_sdr(BYTE value)
{
    if (userport_joystick_enable) {
        switch (userport_joystick_type) {
            case USERPORT_JOYSTICK_HIT:
            case USERPORT_JOYSTICK_KINGSOFT:
                userport_joystick_button_sp2 = (get_joystick_value(4) & 0x10) ? 0 : 0xff;
                break;
            case USERPORT_JOYSTICK_STARBYTE:
                userport_joystick_button_sp2 = (get_joystick_value(3) & 0x10) ? 0 : 0xff;
                break;
        }
    }
}

/* C64/C128 only */
BYTE userport_joystick_read_pa2(BYTE orig)
{
    BYTE retval = orig;

    if (userport_joystick_enable) {
        retval &= 0xfb;
        switch (userport_joystick_type) {
            case USERPORT_JOYSTICK_HIT:
                retval |= (BYTE)((get_joystick_value(3) & 0x10) ? 0 : 4);
                break;
            case USERPORT_JOYSTICK_KINGSOFT:
                retval |= (BYTE)((get_joystick_value(3) & 0x01) ? 0 : 4);
                break;
            case USERPORT_JOYSTICK_STARBYTE:
                retval |= (BYTE)((get_joystick_value(4) & 0x01) ? 0 : 4);
                break;
        }
    }
    return retval;
}

BYTE userport_joystick_read_pbx(BYTE orig)
{
    BYTE retval = orig;
    BYTE jv3;
    BYTE jv4;

    if (userport_joystick_enable) {
        jv3 = get_joystick_value(3);
        jv4 = get_joystick_value(4);
        switch (userport_joystick_type) {
            case USERPORT_JOYSTICK_HIT:
                retval = (BYTE)~((jv3 & 0xf) | ((jv4 & 0xf) << 4));
                break;
            case USERPORT_JOYSTICK_CGA:
                retval = (BYTE)~((jv3 & 0x10) | ((jv4 & 0x10) << 1) | (get_joystick_value(userport_joystick_cga_select + 3) & 0xf));
                break;
            case USERPORT_JOYSTICK_PET:
                retval = ((jv3 & 0xf) | ((jv4 & 0xf) << 4));
                retval |= (jv3 & 0x10) ? 3 : 0;
                retval |= (jv4 & 0x10) ? 0x30 : 0;
                retval = (BYTE)~retval;
                break;
            case USERPORT_JOYSTICK_HUMMER:
                retval = (BYTE)~(jv3 & 0x1f);
                break;
            case USERPORT_JOYSTICK_OEM:
                retval = ((jv3 & 1) << 7);
                retval |= ((jv3 & 2) << 5);
                retval |= ((jv3 & 4) << 3);
                retval |= ((jv3 & 8) << 1);
                retval |= ((jv3 & 16) >> 1);
                retval = (BYTE)~retval;
                break;
            case USERPORT_JOYSTICK_KINGSOFT:
                retval = ((jv4 >> 3) & 1) << 0;
                retval |= ((jv4 >> 2) & 1) << 1;
                retval |= ((jv4 >> 1) & 1) << 2;
                retval |= ((jv4 >> 0) & 1) << 3;
                retval |= ((jv3 >> 4) & 1) << 4;
                retval |= ((jv3 >> 3) & 1) << 5;
                retval |= ((jv3 >> 2) & 1) << 6;
                retval |= ((jv3 >> 1) & 1) << 7;
                retval = (BYTE)~retval;
                break;
            case USERPORT_JOYSTICK_STARBYTE:
                retval = ((jv3 >> 1) & 1) << 0;
                retval |= ((jv3 >> 3) & 1) << 1;
                retval |= ((jv3 >> 2) & 1) << 2;
                retval |= ((jv3 >> 0) & 1) << 3;
                retval |= ((jv4 >> 4) & 1) << 4;
                retval |= ((jv4 >> 1) & 1) << 5;
                retval |= ((jv4 >> 3) & 1) << 6;
                retval |= ((jv4 >> 2) & 1) << 7;
                retval = (BYTE)~retval;
                break;
        }
    }
    return retval;
}

/* C64/C128 only */
BYTE userport_joystick_read_sdr(BYTE orig)
{
    BYTE retval = orig;

    if (userport_joystick_enable) {
        switch (userport_joystick_type) {
            case USERPORT_JOYSTICK_HIT:
            case USERPORT_JOYSTICK_KINGSOFT:
            case USERPORT_JOYSTICK_STARBYTE:
                retval = userport_joystick_button_sp2;
                break;
        }
    }
    return retval;
}
