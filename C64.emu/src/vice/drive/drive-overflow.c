/*
 * drive-overflow.c
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

#include "alarm.h"
#include "clkguard.h"
#include "drive-overflow.h"
#include "drive.h"
#include "drivetypes.h"
#include "interrupt.h"
#include "log.h"
#include "rotation.h"


static void drive_clk_overflow_callback(CLOCK sub, void *data)
{
    unsigned int dnr;
    drive_t *drive;

    dnr = vice_ptr_to_uint(data);
    drive = drive_context[dnr]->drive;

    rotation_rotate_disk(drive);

    rotation_overflow_callback(sub, dnr);

    if (drive->attach_clk > (CLOCK)0) {
        drive->attach_clk -= sub;
    }
    if (drive->detach_clk > (CLOCK)0) {
        drive->detach_clk -= sub;
    }
    if (drive->attach_detach_clk > (CLOCK)0) {
        drive->attach_detach_clk -= sub;
    }
    if (drive->led_last_change_clk > (CLOCK)0) {
        drive->led_last_change_clk -= sub;
    }
    if (drive->led_last_uiupdate_clk > (CLOCK)0) {
        drive->led_last_uiupdate_clk -= sub;
    }

    alarm_context_time_warp(drive_context[dnr]->cpu->alarm_context, sub, -1);
    interrupt_cpu_status_time_warp(drive_context[dnr]->cpu->int_status, sub,
                                   -1);
}

void drive_overflow_init(void)
{
    unsigned int dnr;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        clk_guard_add_callback(drive_context[dnr]->cpu->clk_guard,
                               drive_clk_overflow_callback, uint_to_void_ptr(dnr));
    }
}
