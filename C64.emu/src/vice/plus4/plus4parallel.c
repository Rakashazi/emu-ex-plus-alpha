/*
 * plus4parallel.c - Parallel cable handling for the Plus4.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include "drive.h"
#include "drivetypes.h"
#include "iecdrive.h"
#include "joyport.h"
#include "maincpu.h"
#include "plus4parallel.h"
#include "ted.h"
#include "types.h"
#include "userport.h"

static uint8_t parallel_cable_cpu_value = 0xff;
static uint8_t parallel_cable_drive_value[NUM_DISK_UNITS] = { 0xff, 0xff, 0xff, 0xff };

static int parallel_cable_enabled = 0;

void parallel_cable_drive_write(int port, uint8_t data, int handshake, unsigned int dnr)
{
    parallel_cable_drive_value[dnr] = data;
}

uint8_t parallel_cable_drive_read(int type, int handshake)
{
    return parallel_cable_cpu_value & parallel_cable_drive_value[0] & parallel_cable_drive_value[1];
}

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void parallel_cable_cpu_write(int type, uint8_t data);
static uint8_t parallel_cable_cpu_read(int type, uint8_t orig);
static void userport_par_cable_store_pbx(uint8_t data, int pulse);
static uint8_t userport_par_cable_read_pbx(uint8_t orig);
static int userport_par_cable_enable(int value);

static userport_device_t par_cable_device = {
    "Userport parallel drive cable",      /* device name */
    JOYSTICK_ADAPTER_ID_NONE,             /* NOT a joystick adapter */
    USERPORT_DEVICE_TYPE_DRIVE_PAR_CABLE, /* device is a parallel drive cable */
    userport_par_cable_enable,            /* enable function */
    userport_par_cable_read_pbx,          /* read pb0-pb7 function */
    userport_par_cable_store_pbx,         /* NO store pb0-pb7 function */
    NULL,                                 /* NO read pa2 pin function */
    NULL,                                 /* NO store pa2 pin function */
    NULL,                                 /* NO read pa3 pin function */
    NULL,                                 /* NO store pa3 pin function */
    0,                                    /* pc pin is NOT needed */
    NULL,                                 /* NO store sp1 pin function */
    NULL,                                 /* NO read sp1 pin function */
    NULL,                                 /* NO store sp2 pin function */
    NULL,                                 /* NO read sp2 pin function */
    NULL,                                 /* NO reset pin function */
    NULL,                                 /* NO power toggle function */
    NULL,                                 /* NO snapshot write function */
    NULL                                  /* NO snapshot read function */
};

static int userport_par_cable_enable(int value)
{
    int val = value ? 1 : 0;

    parallel_cable_enabled = val;

    return 0;
}

static void userport_par_cable_store_pbx(uint8_t data, int pulse)
{
    parallel_cable_cpu_write(DRIVE_PC_STANDARD, data);
}

static uint8_t userport_par_cable_read_pbx(uint8_t orig)
{
    return parallel_cable_cpu_read(DRIVE_PC_STANDARD, orig);
}

int parallel_cable_cpu_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_DRIVE_PAR_CABLE, &par_cable_device);
}

/* ------------------------------------------------------------------------- */

static void parallel_cable_cpu_write(int type, uint8_t data)
{
    if (!(diskunit_context[0]->enable)
        && !(diskunit_context[1]->enable)) {
        return;
    }

    drive_cpu_execute_all(last_write_cycle);

    parallel_cable_cpu_value = data;
}

static uint8_t parallel_cable_cpu_read(int type, uint8_t orig)
{
    uint8_t data = 0xff;

    if (!(diskunit_context[0]->enable)
        && !(diskunit_context[1]->enable)) {
        return orig;
    }

    drive_cpu_execute_all(maincpu_clk);

    return data & (parallel_cable_cpu_value & parallel_cable_drive_value[0] & parallel_cable_drive_value[1]);
}

void parallel_cable_cpu_undump(int type, uint8_t data)
{
    parallel_cable_cpu_value = data;
}
