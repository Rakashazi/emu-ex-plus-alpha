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
#include "joyport.h"
#include "joystick.h"
#include "machine.h"
#include "resources.h"
#include "snapshot.h"
#include "translate.h"
#include "types.h"
#include "userport.h"
#include "userport_joystick.h"

/* CGA userport joystick adapter (C64/C128/CBM2/PET/VIC20)

C64/C128 | CBM2 | PET | VIC20 | 74LS257 | JOY1 | JOY2 | NOTES
-------------------------------------------------------------
    C    |  14  |  C  |   C   |    4    |  -   |  -   | PB0 <- JOYx UP OUTPUT
    D    |  13  |  D  |   D   |    7    |  -   |  -   | PB1 <- JOYx DOWN OUTPUT
    E    |  12  |  E  |   E   |    9    |  -   |  -   | PB2 <- JOYx LEFT OUTPUT
    F    |  11  |  F  |   F   |   12    |  -   |  -   | PB3 <- JOYx RIGHT OUTPUT
    H    |  10  |  H  |   H   |    -    |  -   |  6   | PB4 <- JOY2 FIRE
    J    |   9  |  J  |   J   |    -    |  6   |  -   | PB5 <- JOY1 FIRE
    L    |   7  |  L  |   L   |    1    |  -   |  -   | PB7 -> INPUT SELECT
    -    |   -  |  -  |   -   |    2    |  1   |  -   | JOYx UP INPUT 0 <- JOY1 UP
    -    |   -  |  -  |   -   |    5    |  2   |  -   | JOYx DOWN INPUT 0 <- JOY1 DOWN
    -    |   -  |  -  |   -   |   11    |  3   |  -   | JOYx LEFT INPUT 0 <- JOY1 LEFT
    -    |   -  |  -  |   -   |   14    |  4   |  -   | JOYx RIGHT INPUT 0 <- JOY1 RIGHT
    -    |   -  |  -  |   -   |    3    |  -   |  1   | JOYx UP INPUT 1 <- JOY2 UP
    -    |   -  |  -  |   -   |    6    |  -   |  2   | JOYx DOWN INPUT 1 <- JOY2 DOWN
    -    |   -  |  -  |   -   |   10    |  -   |  3   | JOYx LEFT INPUT 1 <- JOY2 LEFT
    -    |   -  |  -  |   -   |   13    |  -   |  4   | JOYx RIGHT INPUT 1 <- JOY2 RIGHT
*/

/* PET userport joystick adapter (C64/C128/CBM2/PET/PLUS4/VIC20)

C64/C128 | CBM2 | PET | PLUS4 | VIC20 | JOY1 | JOY2 | NOTES
-----------------------------------------------------------
    C    |  14  |  C  |   B   |   C   |  1   |  -   | PB0 <- JOY1 UP
    D    |  13  |  D  |   K   |   D   |  2   |  -   | PB1 <- JOY1 DOWN
    E    |  12  |  E  |   4   |   E   |  3   |  -   | PB2 <- JOY1 LEFT
    F    |  11  |  F  |   5   |   F   |  4   |  -   | PB3 <- JOY1 RIGHT
    H    |  10  |  H  |   6   |   H   |  -   |  1   | PB4 <- JOY2 UP
    J    |   9  |  J  |   7   |   J   |  -   |  2   | PB5 <- JOY2 DOWN
    K    |   8  |  K  |   J   |   K   |  -   |  3   | PB6 <- JOY2 LEFT
    L    |   7  |  L  |   F   |   L   |  -   |  4   | PB7 <-> JOY2 RIGHT

JOY1 FIRE is hooked up to pull down both JOY1 LEFT & JOY1 RIGHT
JOY2 FIRE is hooked up to pull down both JOY2 LEFT & JOY2 RIGHT
*/

/* C64DTV HUMMER userport joystick adapter (C64/C128/C64DTV/CBM2/PET/PLUS4/VIC20)

C64/C128 | C64DTV | CBM2 | PET | PLUS4 | VIC20 | JOY | NOTES
------------------------------------------------------------
    C    |  USR0  |  14  |  C  |   B   |   C   |  1  | PB0 <- JOY UP
    D    |  USR1  |  13  |  D  |   K   |   D   |  2  | PB1 <- JOY DOWN
    E    |  USR2  |  12  |  E  |   4   |   E   |  3  | PB2 <- JOY LEFT
    F    |  USR3  |  11  |  F  |   5   |   F   |  4  | PB3 <- JOY RIGHT
    H    |  USR4  |  10  |  H  |   6   |   H   |  6  | PB4 <- JOY FIRE
*/

/* VIC20 OEM userport joystick adapter (C64/C128/CBM2/PET/PLUS4/VIC20)

C64/C128 | CBM2 | PET | PLUS4 | VIC20 | JOY | NOTES
---------------------------------------------------
    F    |  11  |  F  |   5   |   F   |  6  | PB3 <- JOY FIRE
    H    |  10  |  H  |   6   |   H   |  4  | PB4 <- JOY RIGHT
    J    |   9  |  J  |   7   |   J   |  3  | PB5 <- JOY LEFT
    K    |   8  |  K  |   J   |   K   |  2  | PB6 <- JOY DOWN
    L    |   7  |  L  |   F   |   L   |  1  | PB7 <- JOY UP
*/

/* C64 HIT userport joystick adapter (C64/C128)

C64/C128 | JOY1 | JOY2 | NOTES
------------------------------
    4    |  -   |  -   | CNT1 -> CNT2
    6    |  -   |  -   | CNT2 <- CNT1
    7    |  -   |  6   | SP2 <- JOY2 FIRE
    C    |  1   |  -   | PB0 <- JOY1 UP
    D    |  2   |  -   | PB1 <- JOY1 DOWN
    E    |  3   |  -   | PB2 <- JOY1 LEFT
    F    |  4   |  -   | PB3 <- JOY1 RIGHT
    H    |  -   |  1   | PB4 <- JOY2 UP
    J    |  -   |  2   | PB5 <- JOY2 DOWN
    K    |  -   |  3   | PB6 <- JOY2 LEFT
    L    |  -   |  4   | PB7 <- JOY2 RIGHT
    M    |  -   |  6   | PA2 <- JOY1 FIRE
*/

/* C64 KingSoft userport joystick adapter (C64/C128)

C64/C128 | JOY1 | JOY2 | NOTES
------------------------------
    4    |  -   |  -   | CNT1 -> CNT2
    6    |  -   |  -   | CNT2 <- CNT1
    7    |  -   |  6   | SP2 <- JOY2 FIRE
    C    |  -   |  4   | PB0 <- JOY2 RIGHT
    D    |  -   |  3   | PB1 <- JOY2 LEFT
    E    |  -   |  2   | PB2 <- JOY2 DOWN
    F    |  -   |  1   | PB3 <- JOY2 UP
    H    |  6   |  -   | PB4 <- JOY1 FIRE
    J    |  4   |  -   | PB5 <- JOY1 RIGHT
    K    |  3   |  -   | PB6 <- JOY1 LEFT
    L    |  2   |  -   | PB7 <- JOY1 DOWN
    M    |  1   |  -   | PA2 <- JOY1 UP
*/

/* C64 StarByte userport joystick adapter (C64/C128)

C64/C128 | JOY1 | JOY2 | NOTES
------------------------------
   4     |  -   |  -   | CNT1 -> CNT2
   6     |  -   |  -   | CNT2 <- CNT1
   7     |  6   |  -   | SP2 <- JOY1 FIRE
   C     |  2   |  -   | PB0 <- JOY1 DOWN
   D     |  4   |  -   | PB1 <- JOY1 RIGHT
   E     |  3   |  -   | PB2 <- JOY1 LEFT
   F     |  1   |  -   | PB3 <- JOY1 UP
   H     |  -   |  6   | PB4 <- JOY2 FIRE
   J     |  -   |  2   | PB5 <- JOY2 DOWN
   K     |  -   |  4   | PB6 <- JOY2 RIGHT
   L     |  -   |  3   | PB7 <- JOY2 LEFT
   M     |  -   |  1   | PA2 <- JOY2 UP
*/

/* ------------------------------------------------------------------------- */

int userport_joystick_enable = 0;
int userport_joystick_type = USERPORT_JOYSTICK_HUMMER;	/* default for x64dtv */

static int userport_joystick_cga_select = 0;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void userport_joystick_cga_read_pbx(void);
static void userport_joystick_cga_store_pbx(BYTE value);
static int userport_joystick_cga_write_snapshot_module(snapshot_t *s);
static int userport_joystick_cga_read_snapshot_module(snapshot_t *s);

static void userport_joystick_pet_read_pbx(void);
static void userport_joystick_pet_hit_store_pbx(BYTE value);
static int userport_joystick_pet_write_snapshot_module(snapshot_t *s);
static int userport_joystick_pet_read_snapshot_module(snapshot_t *s);

static void userport_joystick_hummer_read_pbx(void);
static void userport_joystick_hummer_store_pbx(BYTE value);
static int userport_joystick_hummer_oem_write_snapshot_module(snapshot_t *s);
static int userport_joystick_hummer_read_snapshot_module(snapshot_t *s);

static void userport_joystick_oem_read_pbx(void);
static void userport_joystick_oem_store_pbx(BYTE value);
static int userport_joystick_oem_read_snapshot_module(snapshot_t *s);

static void userport_joystick_hit_read_pbx(void);
static void userport_joystick_hit_read_pa2(void);
static void userport_joystick_hit_store_sp1(BYTE val);
static void userport_joystick_hit_read_sp2(void);
static int userport_joystick_hit_write_snapshot_module(snapshot_t *s);
static int userport_joystick_hit_read_snapshot_module(snapshot_t *s);

static void userport_joystick_kingsoft_read_pbx(void);
static void userport_joystick_kingsoft_store_pbx(BYTE value);
static void userport_joystick_kingsoft_read_pa2(void);
static void userport_joystick_kingsoft_store_sp1(BYTE val);
static void userport_joystick_kingsoft_read_sp2(void);
static int userport_joystick_kingsoft_write_snapshot_module(snapshot_t *s);
static int userport_joystick_kingsoft_read_snapshot_module(snapshot_t *s);

static void userport_joystick_starbyte_read_pbx(void);
static void userport_joystick_starbyte_store_pbx(BYTE value);
static void userport_joystick_starbyte_read_pa2(void);
static void userport_joystick_starbyte_store_sp1(BYTE val);
static void userport_joystick_starbyte_read_sp2(void);
static int userport_joystick_starbyte_write_snapshot_module(snapshot_t *s);
static int userport_joystick_starbyte_read_snapshot_module(snapshot_t *s);

static userport_device_t cga_device = {
    USERPORT_DEVICE_JOYSTICK_CGA,
    "CGA userport joy adapter",
    IDGS_CGA_JOY_ADAPTER,
    userport_joystick_cga_read_pbx,
    userport_joystick_cga_store_pbx,
    NULL, /* NO pa2 read */
    NULL, /* NO pa2 write */
    NULL, /* NO pa3 read */
    NULL, /* NO pa3 write */
    0, /* NO pc pin needed */
    NULL, /* NO sp1 write */
    NULL, /* NO sp1 read */
    NULL, /* NO sp2 write */
    NULL, /* NO sp2 read */
    "UserportJoy",
    0xff,
    0x3f, /* validity mask doesn't change */
    0,
    0
};

static userport_snapshot_t cga_snapshot = {
    USERPORT_DEVICE_JOYSTICK_CGA,
    userport_joystick_cga_write_snapshot_module,
    userport_joystick_cga_read_snapshot_module
};

static userport_device_t pet_device = {
    USERPORT_DEVICE_JOYSTICK_PET,
    "PET userport joy adapter",
    IDGS_PET_JOY_ADAPTER,
    userport_joystick_pet_read_pbx,
    userport_joystick_pet_hit_store_pbx,
    NULL, /* NO pa2 read */
    NULL, /* NO pa2 write */
    NULL, /* NO pa3 read */
    NULL, /* NO pa3 write */
    0, /* NO pc pin needed */
    NULL, /* NO sp1 write */
    NULL, /* NO sp1 read */
    NULL, /* NO sp2 write */
    NULL, /* NO sp2 read */
    "UserportJoy",
    0xff,
    0xff, /* validity mask doesn't change */
    0,
    0
};

static userport_snapshot_t pet_snapshot = {
    USERPORT_DEVICE_JOYSTICK_PET,
    userport_joystick_pet_write_snapshot_module,
    userport_joystick_pet_read_snapshot_module
};

static userport_device_t hummer_device = {
    USERPORT_DEVICE_JOYSTICK_HUMMER,
    "Hummer userport joy adapter",
    IDGS_HUMMER_JOY_ADAPTER,
    userport_joystick_hummer_read_pbx,
    userport_joystick_hummer_store_pbx,
    NULL, /* NO pa2 read */
    NULL, /* NO pa2 write */
    NULL, /* NO pa3 read */
    NULL, /* NO pa3 write */
    0, /* NO pc pin needed */
    NULL, /* NO sp1 write */
    NULL, /* NO sp1 read */
    NULL, /* NO sp2 write */
    NULL, /* NO sp2 read */
    "UserportJoy",
    0xff,
    0x1f, /* validity mask doesn't change */
    0,
    0
};

static userport_snapshot_t hummer_snapshot = {
    USERPORT_DEVICE_JOYSTICK_HUMMER,
    userport_joystick_hummer_oem_write_snapshot_module,
    userport_joystick_hummer_read_snapshot_module
};

static userport_device_t oem_device = {
    USERPORT_DEVICE_JOYSTICK_OEM,
    "OEM userport joy adapter",
    IDGS_OEM_JOY_ADAPTER,
    userport_joystick_oem_read_pbx,
    userport_joystick_oem_store_pbx,
    NULL, /* NO pa2 read */
    NULL, /* NO pa2 write */
    NULL, /* NO pa3 read */
    NULL, /* NO pa3 write */
    0, /* NO pc pin needed */
    NULL, /* NO sp1 write */
    NULL, /* NO sp1 read */
    NULL, /* NO sp2 write */
    NULL, /* NO sp2 read */
    "UserportJoy",
    0xff,
    0xf8, /* validity mask doesn't change */
    0,
    0
};

static userport_snapshot_t oem_snapshot = {
    USERPORT_DEVICE_JOYSTICK_OEM,
    userport_joystick_hummer_oem_write_snapshot_module,
    userport_joystick_oem_read_snapshot_module
};

static userport_device_t hit_device = {
    USERPORT_DEVICE_JOYSTICK_HIT,
    "HIT userport joy adapter",
    IDGS_HIT_JOY_ADAPTER,
    userport_joystick_hit_read_pbx,
    userport_joystick_pet_hit_store_pbx,
    userport_joystick_hit_read_pa2,
    NULL, /* NO pa2 write */
    NULL, /* NO pa3 read */
    NULL, /* NO pa3 write */
    0, /* NO pc pin needed */
    userport_joystick_hit_store_sp1,
    NULL, /* NO sp1 read */
    NULL, /* NO sp2 write */
    userport_joystick_hit_read_sp2,
    "UserportJoy",
    0xff,
    0xff, /* validity mask doesn't change */
    0,
    0
};

static userport_snapshot_t hit_snapshot = {
    USERPORT_DEVICE_JOYSTICK_HIT,
    userport_joystick_hit_write_snapshot_module,
    userport_joystick_hit_read_snapshot_module
};

static userport_device_t kingsoft_device = {
    USERPORT_DEVICE_JOYSTICK_KINGSOFT,
    "KingSoft userport joy adapter",
    IDGS_KINGSOFT_JOY_ADAPTER,
    userport_joystick_kingsoft_read_pbx,
    userport_joystick_kingsoft_store_pbx,
    userport_joystick_kingsoft_read_pa2,
    NULL, /* NO pa2 write */
    NULL, /* NO pa3 read */
    NULL, /* NO pa3 write */
    0, /* NO pc pin needed */
    userport_joystick_kingsoft_store_sp1,
    NULL, /* NO sp1 read */
    NULL, /* NO sp2 write */
    userport_joystick_kingsoft_read_sp2,
    "UserportJoy",
    0xff,
    0xff, /* validity mask doesn't change */
    0,
    0
};

static userport_snapshot_t kingsoft_snapshot = {
    USERPORT_DEVICE_JOYSTICK_KINGSOFT,
    userport_joystick_kingsoft_write_snapshot_module,
    userport_joystick_kingsoft_read_snapshot_module
};

static userport_device_t starbyte_device = {
    USERPORT_DEVICE_JOYSTICK_STARBYTE,
    "StarByte userport joy adapter",
    IDGS_STARBYTE_JOY_ADAPTER,
    userport_joystick_starbyte_read_pbx,
    userport_joystick_starbyte_store_pbx,
    userport_joystick_starbyte_read_pa2,
    NULL, /* NO pa2 write */
    NULL, /* NO pa3 read */
    NULL, /* NO pa3 write */
    0, /* NO pc pin needed */
    userport_joystick_starbyte_store_sp1,
    NULL, /* NO sp1 read */
    NULL, /* NO sp2 write */
    userport_joystick_starbyte_read_sp2,
    "UserportJoy",
    0xff,
    0xff, /* validity mask doesn't change */
    0,
    0
};

static userport_snapshot_t starbyte_snapshot = {
    USERPORT_DEVICE_JOYSTICK_STARBYTE,
    userport_joystick_starbyte_write_snapshot_module,
    userport_joystick_starbyte_read_snapshot_module
};

static userport_device_list_t *userport_joystick_list_item = NULL;

/* ------------------------------------------------------------------------- */

static int register_userport_adapter(int adapter)
{
    switch (adapter) {
        case USERPORT_JOYSTICK_CGA:
            userport_joystick_list_item = userport_device_register(&cga_device);
            break;
        case USERPORT_JOYSTICK_PET:
            userport_joystick_list_item = userport_device_register(&pet_device);
            break;
        case USERPORT_JOYSTICK_HUMMER:
            userport_joystick_list_item = userport_device_register(&hummer_device);
            break;
        case USERPORT_JOYSTICK_OEM:
            userport_joystick_list_item = userport_device_register(&oem_device);
            break;
        case USERPORT_JOYSTICK_HIT:
            userport_joystick_list_item = userport_device_register(&hit_device);
            break;
        case USERPORT_JOYSTICK_KINGSOFT:
            userport_joystick_list_item = userport_device_register(&kingsoft_device);
            break;
        case USERPORT_JOYSTICK_STARBYTE:
            userport_joystick_list_item = userport_device_register(&starbyte_device);
            break;
        default:
            return -1;
    }
    if (userport_joystick_list_item != NULL) {
        return 0;
    }
    return -1;
}

static int set_userport_joystick_enable(int value, void *param)
{
    int val = value ? 1 : 0;

    if (userport_joystick_enable == val) {
        return 0;
    }

    if (val) {
        if (register_userport_adapter(userport_joystick_type) < 0) {
            return -1;
        }
    } else {
        userport_device_unregister(userport_joystick_list_item);
        userport_joystick_list_item = NULL;
    }

    userport_joystick_enable = val;

    return 0;
}

static int set_userport_joystick_type(int val, void *param)
{
    switch (val) {
        case USERPORT_JOYSTICK_CGA:
            if (machine_class != VICE_MACHINE_PLUS4) {
                break;
            }
            return -1;
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

    if (userport_joystick_enable) {
        userport_device_unregister(userport_joystick_list_item);
        userport_joystick_list_item = NULL;
        if (register_userport_adapter(val) < 0) {
            return -1;
        }
    }

    userport_joystick_type = val;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "UserportJoy", 0, RES_EVENT_NO, NULL,
      &userport_joystick_enable, set_userport_joystick_enable, NULL },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_type_plus4[] = {
    { "UserportJoyType", USERPORT_JOYSTICK_PET, RES_EVENT_NO, NULL,
      &userport_joystick_type, set_userport_joystick_type, NULL },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_type[] = {
    { "UserportJoyType", USERPORT_JOYSTICK_CGA, RES_EVENT_NO, NULL,
      &userport_joystick_type, set_userport_joystick_type, NULL },
    RESOURCE_INT_LIST_END
};

int userport_joystick_resources_init(void)
{
    userport_snapshot_register(&cga_snapshot);
    userport_snapshot_register(&pet_snapshot);
    userport_snapshot_register(&hummer_snapshot);
    userport_snapshot_register(&oem_snapshot);
    userport_snapshot_register(&hit_snapshot);
    userport_snapshot_register(&kingsoft_snapshot);
    userport_snapshot_register(&starbyte_snapshot);

    if (machine_class != VICE_MACHINE_C64DTV) {
        if (machine_class == VICE_MACHINE_PLUS4) {
            if (resources_register_int(resources_int_type_plus4) < 0) {
                return -1;
            }
        } else {
            if (resources_register_int(resources_int_type) < 0) {
                return -1;
            }
        }
    }

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
    CMDLINE_LIST_END
};

static const cmdline_option_t cmdline_options_type[] =
{
    { "-userportjoytype", SET_RESOURCE, 1,
      NULL, NULL, "UserportJoyType", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_SET_USERPORT_JOY_TYPE,
      NULL, NULL },
    CMDLINE_LIST_END
};

int userport_joystick_cmdline_options_init(void)
{
    if (machine_class != VICE_MACHINE_C64DTV) {
        if (cmdline_register_options(cmdline_options_type) < 0) {
            return -1;
        }
    }
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

static void userport_joystick_cga_read_pbx(void)
{
    BYTE retval;
    BYTE jv3 = ~read_joyport_dig(JOYPORT_3);
    BYTE jv4 = ~read_joyport_dig(JOYPORT_4);

    if (userport_joystick_cga_select) {
        retval = (BYTE)~((jv4 & 0xf) | (jv3 & 0x10) | ((jv4 & 0x10) << 1));
    } else {
        retval = (BYTE)~((jv3 & 0xf) | (jv3 & 0x10) | ((jv4 & 0x10) << 1));
    }
    cga_device.retval = retval;
}

static void userport_joystick_cga_store_pbx(BYTE value)
{
    userport_joystick_cga_select = (value & 0x80) ? 0 : 1;
}

/* ------------------------------------------------------------------------- */

static void userport_joystick_pet_read_pbx(void)
{
    BYTE retval;
    BYTE jv3 = ~read_joyport_dig(JOYPORT_3);
    BYTE jv4 = ~read_joyport_dig(JOYPORT_4);

    retval = ((jv3 & 0xf) | ((jv4 & 0xf) << 4));
    retval |= (jv3 & 0x10) ? 3 : 0;
    retval |= (jv4 & 0x10) ? 0x30 : 0;
    retval = (BYTE)~retval;

    pet_device.retval = retval;
}

static void userport_joystick_pet_hit_store_pbx(BYTE value)
{
    BYTE j1 = value & 0xf;
    BYTE j2 = (value & 0xf0) >> 4;

    store_joyport_dig(JOYPORT_3, j1, 0xf);
    store_joyport_dig(JOYPORT_4, j2, 0xf);
}

/* ------------------------------------------------------------------------- */

static void userport_joystick_hummer_read_pbx(void)
{
    BYTE retval;
    BYTE jv3 = ~read_joyport_dig(JOYPORT_3);

    retval = (BYTE)~(jv3 & 0x1f);

    hummer_device.retval = retval;
}

static void userport_joystick_hummer_store_pbx(BYTE value)
{
    BYTE j1 = value & 0x1f;

    store_joyport_dig(JOYPORT_3, j1, 0x1f);
}

/* ------------------------------------------------------------------------- */

static void userport_joystick_oem_read_pbx(void)
{
    BYTE retval;
    BYTE jv3 = ~read_joyport_dig(JOYPORT_3);

    retval = ((jv3 & 1) << 7);
    retval |= ((jv3 & 2) << 5);
    retval |= ((jv3 & 4) << 3);
    retval |= ((jv3 & 8) << 1);
    retval |= ((jv3 & 16) >> 1);
    retval = (BYTE)~retval;

    oem_device.retval = retval;
}

static void userport_joystick_oem_store_pbx(BYTE value)
{
    BYTE j1 = (value & 8) << 1;

    j1 |= (value & 0x10) >> 1;
    j1 |= (value & 0x20) >> 3;
    j1 |= (value & 0x40) >> 5;
    j1 |= (value & 0x80) >> 7;

    store_joyport_dig(JOYPORT_3, j1, 0x1f);
}

/* ------------------------------------------------------------------------- */

static void userport_joystick_hit_read_pbx(void)
{
    BYTE retval;
    BYTE jv3 = ~read_joyport_dig(JOYPORT_3);
    BYTE jv4 = ~read_joyport_dig(JOYPORT_4);

    retval = (BYTE)~((jv3 & 0xf) | ((jv4 & 0xf) << 4));

    hit_device.retval = retval;
}

static void userport_joystick_hit_read_pa2(void)
{
    BYTE jv1 = ~read_joyport_dig(JOYPORT_3);

    hit_device.retval = (jv1 & 0x10) ? 0 : 1;
}

static BYTE hit_sp2_retval = 0xff;

static void userport_joystick_hit_store_sp1(BYTE val)
{
    hit_sp2_retval = ((~read_joyport_dig(JOYPORT_4)) & 0x10) ? 0 : 0xff;
}

static void userport_joystick_hit_read_sp2(void)
{
    hit_device.retval = hit_sp2_retval;
}

/* ------------------------------------------------------------------------- */

static void userport_joystick_kingsoft_read_pbx(void)
{
    BYTE retval;
    BYTE jv3 = ~read_joyport_dig(JOYPORT_3);
    BYTE jv4 = ~read_joyport_dig(JOYPORT_4);

    retval = ((jv4 >> 3) & 1) << 0;
    retval |= ((jv4 >> 2) & 1) << 1;
    retval |= ((jv4 >> 1) & 1) << 2;
    retval |= ((jv4 >> 0) & 1) << 3;
    retval |= ((jv3 >> 4) & 1) << 4;
    retval |= ((jv3 >> 3) & 1) << 5;
    retval |= ((jv3 >> 2) & 1) << 6;
    retval |= ((jv3 >> 1) & 1) << 7;
    retval = (BYTE)~retval;

    kingsoft_device.retval = retval;
}

static void userport_joystick_kingsoft_store_pbx(BYTE value)
{
    BYTE j1 = value & 0x10;
    BYTE j2 = (value & 1) << 3;

    j1 |= (value & 0x20) >> 2;
    j1 |= (value & 0x40) >> 4;
    j1 |= (value & 0x80) >> 6;
    j2 |= (value & 2) << 1;
    j2 |= (value & 4) >> 1;
    j2 |= (value & 8) >> 3;

    store_joyport_dig(JOYPORT_3, j1, 0x1e);
    store_joyport_dig(JOYPORT_4, j2, 0xf);
}

static void userport_joystick_kingsoft_read_pa2(void)
{
    BYTE jv1 = ~read_joyport_dig(JOYPORT_3);

    kingsoft_device.retval = (jv1 & 1) ? 0 : 1;
}

static BYTE kingsoft_sp2_retval = 0xff;

static void userport_joystick_kingsoft_store_sp1(BYTE val)
{
    kingsoft_sp2_retval = ((~read_joyport_dig(JOYPORT_4)) & 0x10) ? 0 : 0xff;
}

static void userport_joystick_kingsoft_read_sp2(void)
{
    kingsoft_device.retval = kingsoft_sp2_retval;
}

/* ------------------------------------------------------------------------- */

static void userport_joystick_starbyte_read_pbx(void)
{
    BYTE retval;
    BYTE jv3 = ~read_joyport_dig(JOYPORT_3);
    BYTE jv4 = ~read_joyport_dig(JOYPORT_4);

    retval = ((jv3 >> 1) & 1) << 0;
    retval |= ((jv3 >> 3) & 1) << 1;
    retval |= ((jv3 >> 2) & 1) << 2;
    retval |= ((jv3 >> 0) & 1) << 3;
    retval |= ((jv4 >> 4) & 1) << 4;
    retval |= ((jv4 >> 1) & 1) << 5;
    retval |= ((jv4 >> 3) & 1) << 6;
    retval |= ((jv4 >> 2) & 1) << 7;
    retval = (BYTE)~retval;

    starbyte_device.retval = retval;
}

static void userport_joystick_starbyte_store_pbx(BYTE value)
{
    BYTE j1 = (value & 1) << 1;
    BYTE j2 = value & 0x10;

    j1 |= (value & 2) << 2;
    j1 |= value & 4;
    j1 |= (value & 8) >> 3;

    j2 |= (value & 0x20) >> 4;
    j2 |= (value & 0x40) >> 3;
    j2 |= (value & 0x80) >> 5;

    store_joyport_dig(JOYPORT_3, j1, 0xf);
    store_joyport_dig(JOYPORT_4, j2, 0x1e);
}

static void userport_joystick_starbyte_read_pa2(void)
{
    BYTE jv2 = ~read_joyport_dig(JOYPORT_4);

    starbyte_device.retval = (jv2 & 1) ? 0 : 1;
}

static BYTE starbyte_sp2_retval = 0xff;

static void userport_joystick_starbyte_store_sp1(BYTE val)
{
    starbyte_sp2_retval = ((~read_joyport_dig(JOYPORT_3)) & 0x10) ? 0 : 0xff;
}

static void userport_joystick_starbyte_read_sp2(void)
{
    starbyte_device.retval = starbyte_sp2_retval;
}

/* ------------------------------------------------------------------------- */

/* UP_JOY_CGA snapshot module format:

   type  | name   | description
   ----------------------------
   BYTE  | select | joyport select
 */

static char cga_module_name[] = "UP_JOY_CGA";
#define CGA_VER_MAJOR   0
#define CGA_VER_MINOR   0

static int userport_joystick_cga_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, cga_module_name, CGA_VER_MAJOR, CGA_VER_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, (BYTE)userport_joystick_cga_select) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (0
        || joyport_snapshot_write_module(s, JOYPORT_3) < 0
        || joyport_snapshot_write_module(s, JOYPORT_4) < 0) {
        return -1;
    }
    return 0;
}

static int userport_joystick_cga_read_snapshot_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    /* enable device */
    set_userport_joystick_type(USERPORT_JOYSTICK_CGA, NULL);
    set_userport_joystick_enable(1, NULL);

    m = snapshot_module_open(s, cga_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > CGA_VER_MAJOR || minor_version > CGA_VER_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_B_INT(m, &userport_joystick_cga_select) < 0) {
        goto fail;
    }
    snapshot_module_close(m);

    if (0
        || joyport_snapshot_read_module(s, JOYPORT_3) < 0
        || joyport_snapshot_read_module(s, JOYPORT_4) < 0) {
        return -1;
    }
    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}

/* ------------------------------------------------------------------------- */

static int userport_joystick_pet_write_snapshot_module(snapshot_t *s)
{
    if (0
        || joyport_snapshot_write_module(s, JOYPORT_3) < 0
        || joyport_snapshot_write_module(s, JOYPORT_4) < 0) {
        return -1;
    }
    return 0;
}

static int userport_joystick_pet_read_snapshot_module(snapshot_t *s)
{
    /* enable device */
    set_userport_joystick_type(USERPORT_JOYSTICK_PET, NULL);
    set_userport_joystick_enable(1, NULL);

    if (0
        || joyport_snapshot_read_module(s, JOYPORT_3) < 0
        || joyport_snapshot_read_module(s, JOYPORT_4) < 0) {
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static int userport_joystick_hummer_oem_write_snapshot_module(snapshot_t *s)
{
    return joyport_snapshot_write_module(s, JOYPORT_3);
}

static int userport_joystick_hummer_read_snapshot_module(snapshot_t *s)
{
    /* enable device */
    set_userport_joystick_type(USERPORT_JOYSTICK_HUMMER, NULL);
    set_userport_joystick_enable(1, NULL);

    return joyport_snapshot_read_module(s, JOYPORT_3);
}

static int userport_joystick_oem_read_snapshot_module(snapshot_t *s)
{
    /* enable device */
    set_userport_joystick_type(USERPORT_JOYSTICK_OEM, NULL);
    set_userport_joystick_enable(1, NULL);

    return joyport_snapshot_read_module(s, JOYPORT_3);
}

/* ------------------------------------------------------------------------- */

/* UP_JOY_HIT snapshot module format:

   type  | name   | description
   ----------------------------
   BYTE  | retval | current serial port brigde value
 */

static char hit_module_name[] = "UP_JOY_HIT";
#define HIT_VER_MAJOR   0
#define HIT_VER_MINOR   0

static int userport_joystick_hit_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, hit_module_name, HIT_VER_MAJOR, HIT_VER_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, hit_sp2_retval) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (0
        || joyport_snapshot_write_module(s, JOYPORT_3) < 0
        || joyport_snapshot_write_module(s, JOYPORT_4) < 0) {
        return -1;
    }
    return 0;
}

static int userport_joystick_hit_read_snapshot_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    /* enable device */
    set_userport_joystick_type(USERPORT_JOYSTICK_HIT, NULL);
    set_userport_joystick_enable(1, NULL);

    m = snapshot_module_open(s, hit_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > HIT_VER_MAJOR || minor_version > HIT_VER_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_B(m, &hit_sp2_retval) < 0) {
        goto fail;
    }
    snapshot_module_close(m);

    if (0
        || joyport_snapshot_read_module(s, JOYPORT_3) < 0
        || joyport_snapshot_read_module(s, JOYPORT_4) < 0) {
        return -1;
    }
    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}

/* ------------------------------------------------------------------------- */

/* UP_JOY_KINGSOFT snapshot module format:

   type  | name   | description
   ----------------------------
   BYTE  | retval | current serial port brigde value
 */

static char kingsoft_module_name[] = "UP_JOY_KINGSOFT";
#define KINGSOFT_VER_MAJOR   0
#define KINGSOFT_VER_MINOR   0

static int userport_joystick_kingsoft_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, kingsoft_module_name, KINGSOFT_VER_MAJOR, KINGSOFT_VER_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, kingsoft_sp2_retval) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (0
        || joyport_snapshot_write_module(s, JOYPORT_3) < 0
        || joyport_snapshot_write_module(s, JOYPORT_4) < 0) {
        return -1;
    }
    return 0;
}

static int userport_joystick_kingsoft_read_snapshot_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    /* enable device */
    set_userport_joystick_type(USERPORT_JOYSTICK_KINGSOFT, NULL);
    set_userport_joystick_enable(1, NULL);

    m = snapshot_module_open(s, kingsoft_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > KINGSOFT_VER_MAJOR || minor_version > KINGSOFT_VER_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_B(m, &kingsoft_sp2_retval) < 0) {
        goto fail;
    }
    snapshot_module_close(m);

    if (0
        || joyport_snapshot_read_module(s, JOYPORT_3) < 0
        || joyport_snapshot_read_module(s, JOYPORT_4) < 0) {
        return -1;
    }
    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}

/* ------------------------------------------------------------------------- */

/* UP_JOY_STARBYTE snapshot module format:

   type  | name   | description
   ----------------------------
   BYTE  | retval | current serial port brigde value
 */

static char starbyte_module_name[] = "UP_JOY_STARBYTE";
#define STARBYTE_VER_MAJOR   0
#define STARBYTE_VER_MINOR   0

static int userport_joystick_starbyte_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, starbyte_module_name, STARBYTE_VER_MAJOR, STARBYTE_VER_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, starbyte_sp2_retval) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (0
        || joyport_snapshot_write_module(s, JOYPORT_3) < 0
        || joyport_snapshot_write_module(s, JOYPORT_4) < 0) {
        return -1;
    }
    return 0;
}

static int userport_joystick_starbyte_read_snapshot_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    /* enable device */
    set_userport_joystick_type(USERPORT_JOYSTICK_STARBYTE, NULL);
    set_userport_joystick_enable(1, NULL);

    m = snapshot_module_open(s, starbyte_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version != STARBYTE_VER_MAJOR || minor_version != STARBYTE_VER_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_B(m, &starbyte_sp2_retval) < 0) {
        goto fail;
    }
    snapshot_module_close(m);

    if (0
        || joyport_snapshot_read_module(s, JOYPORT_3) < 0
        || joyport_snapshot_read_module(s, JOYPORT_4) < 0) {
        return -1;
    }
    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
