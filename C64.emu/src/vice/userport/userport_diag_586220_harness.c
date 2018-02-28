/*
 * userport_diag_586220_harness.c - Userport part of the 586220 harness emulation.
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

/* Userport part of the 586220 harness (C64/C128)

PIN | PIN | NOTES
-----------------
 4  |  6  | CNT1 <-> CNT2		
 5  |  7  | SP1 <-> SP2
 6  |  4  | CNT2 <-> CNT1
 7  |  5  | SP2 <-> SP1
 8  |  B  | PC2 -> FLAG2
 9  |  M  | PA3 <-> PA2
 B  |  8  | FLAG2 <- PC2
 C  |  H  | PB0 <-> PB4
 D  |  J  | PB1 <-> PB5
 E  |  K  | PB2 <-> PB6
 F  |  L  | PB3 <-> PB7
 H  |  C  | PB4 <-> PB0
 J  |  D  | PB5 <-> PB1
 K  |  E  | PB6 <-> PB2
 L  |  F  | PB7 <-> PB3
 M  |  9  | PA2 <-> PA3
*/

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c64_diag_586220_harness.h"
#include "cmdline.h"
#include "resources.h"
#include "translate.h"
#include "userport.h"
#include "userport_diag_586220_harness.h"

int userport_diag_586220_harness_enabled = 0;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void userport_diag_586220_harness_read_pbx(void);
static void userport_diag_586220_harness_store_pbx(BYTE value);
static void userport_diag_586220_harness_read_pa2(void);
static void userport_diag_586220_harness_store_pa2(BYTE value);
static void userport_diag_586220_harness_read_pa3(void);
static void userport_diag_586220_harness_store_pa3(BYTE value);
static void userport_diag_586220_harness_read_sp1(void);
static void userport_diag_586220_harness_store_sp1(BYTE value);
static void userport_diag_586220_harness_read_sp2(void);
static void userport_diag_586220_harness_store_sp2(BYTE value);

static userport_device_t diag_586220_harness_device = {
    USERPORT_DEVICE_RTC_58321A,
    "Userport diag 586220 harness",
    IDGS_USERPORT_DIAG_586220_HARNESS,
    userport_diag_586220_harness_read_pbx,
    userport_diag_586220_harness_store_pbx,
    userport_diag_586220_harness_read_pa2,
    userport_diag_586220_harness_store_pa2,
    userport_diag_586220_harness_read_pa3,
    userport_diag_586220_harness_store_pa3,
    1, /* pc pin needed */
    userport_diag_586220_harness_store_sp1,
    userport_diag_586220_harness_read_sp1,
    userport_diag_586220_harness_store_sp2,
    userport_diag_586220_harness_read_sp2,
    "UserportDiag586220Harness",
    0xff,
    0xff, /* validity mask doesn't change */
    0,
    0
};

static userport_device_list_t *userport_diag_586220_harness_list_item = NULL;

/* ------------------------------------------------------------------------- */

static int set_userport_diag_586220_harness_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (userport_diag_586220_harness_enabled == val) {
        return 0;
    }

    if (val) {
        userport_diag_586220_harness_list_item = userport_device_register(&diag_586220_harness_device);
        if (userport_diag_586220_harness_list_item == NULL) {
            return -1;
        }
    } else {
        userport_device_unregister(userport_diag_586220_harness_list_item);
        userport_diag_586220_harness_list_item = NULL;
    }

    userport_diag_586220_harness_enabled = val;
    return 0;
}

static const resource_int_t resources_int[] = {
    { "UserportDiag586220Harness", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &userport_diag_586220_harness_enabled, set_userport_diag_586220_harness_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int userport_diag_586220_harness_resources_init(void)
{
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-userportdiag586220harness", SET_RESOURCE, 0,
      NULL, NULL, "UserportDiag586220Harness", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_USERPORT_DIAG_586220_HARNESS,
      NULL, NULL },
    { "+userportdiag586220harness", SET_RESOURCE, 0,
      NULL, NULL, "UserportDiag586220Harness", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_USERPORT_DIAG_586220_HARNESS,
      NULL, NULL },
    CMDLINE_LIST_END
};

int userport_diag_586220_harness_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

static BYTE pax = 0;

static void userport_diag_586220_harness_read_pbx(void)
{
    diag_586220_harness_device.retval = c64_diag_586220_read_userport_pbx();
}

static void userport_diag_586220_harness_store_pbx(BYTE value)
{
    set_userport_flag(1); /* signal lo->hi */
    set_userport_flag(0); /* signal hi->lo */
    c64_diag_586220_store_userport_pbx(value);
}

static void userport_diag_586220_harness_read_pa2(void)
{
    diag_586220_harness_device.retval = (c64_diag_586220_read_userport_pax() & 4) >> 2;
}

static void userport_diag_586220_harness_store_pa2(BYTE value)
{
   pax &= 0xfb;
   pax |= ((value & 1) << 2);

   c64_diag_586220_store_userport_pax(pax);
}

static void userport_diag_586220_harness_read_pa3(void)
{
    diag_586220_harness_device.retval = (c64_diag_586220_read_userport_pax() & 8) >> 3;
}

static void userport_diag_586220_harness_store_pa3(BYTE value)
{
   pax &= 0xf7;
   pax |= ((value & 1) << 3);

   c64_diag_586220_store_userport_pax(pax);
}

static void userport_diag_586220_harness_read_sp1(void)
{
    diag_586220_harness_device.retval = c64_diag_586220_read_userport_sp(0);
}

static void userport_diag_586220_harness_store_sp1(BYTE value)
{
    c64_diag_586220_store_userport_sp(0, value);
}

static void userport_diag_586220_harness_read_sp2(void)
{
    diag_586220_harness_device.retval = c64_diag_586220_read_userport_sp(1);
}

static void userport_diag_586220_harness_store_sp2(BYTE value)
{
    c64_diag_586220_store_userport_sp(1, value);
}
