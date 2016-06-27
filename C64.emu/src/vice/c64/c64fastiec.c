/*
 * c64fastiec.c - Fast IEC bus handling for the C64.
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

#include "c64fastiec.h"
#include "c64.h"
#include "cia.h"
#include "via.h"
#include "drive.h"
#include "drivetypes.h"
#include "iecdrive.h"
#include "maincpu.h"
#include "types.h"


/* Fast IEC for C64 with burst mod */

static int fast_drive_direction[DRIVE_NUM];
int burst_mod;

int set_burst_mod(int mode, void *param)
{
    switch (mode) {
        case BURST_MOD_NONE:
        case BURST_MOD_CIA1:
        case BURST_MOD_CIA2:
            break;
        default:
            return -1;
    }

    burst_mod = mode;
    return 0;
}

void c64fastiec_init(void)
{
    unsigned int dnr;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        fast_drive_direction[dnr] = 1;
    }
}

void c64fastiec_fast_cpu_write(BYTE data)
{
    drive_t *drive;
    unsigned int dnr;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;
        if (drive->enable) {
            drive_cpu_execute_one(drive_context[dnr], maincpu_clk);
            switch (drive->type) {
                case DRIVE_TYPE_1570:
                case DRIVE_TYPE_1571:
                case DRIVE_TYPE_1571CR:
                    ciacore_set_sdr(drive_context[dnr]->cia1571, data);
                    break;
                case DRIVE_TYPE_1581:
                    ciacore_set_sdr(drive_context[dnr]->cia1581, data);
                    break;
                case DRIVE_TYPE_2000:
                case DRIVE_TYPE_4000:
                    viacore_set_sr(drive_context[dnr]->via4000, data);
                    break;
            }
        }
    }
}

void iec_fast_drive_write(BYTE data, unsigned int dnr)
{
    if (fast_drive_direction[dnr]) {
        switch (burst_mod) {
            case BURST_MOD_CIA1:
                ciacore_set_sdr(machine_context.cia1, data);
                break;
            case BURST_MOD_CIA2:
                ciacore_set_sdr(machine_context.cia2, data);
                break;
        }
    }
}

void iec_fast_drive_direction(int direction, unsigned int dnr)
{
    /* 0: input */
    fast_drive_direction[dnr] = direction;
}
