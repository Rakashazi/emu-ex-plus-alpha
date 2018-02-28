/*
 * drive-check.c
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

#include "drive-check.h"
#include "drive.h"
#include "drivetypes.h"
#include "iecdrive.h"
#include "machine-drive.h"


static unsigned int drive_check_ieee(unsigned int type)
{
    switch (type) {
        case DRIVE_TYPE_2031:
        case DRIVE_TYPE_2040:
        case DRIVE_TYPE_3040:
        case DRIVE_TYPE_4040:
        case DRIVE_TYPE_1001:
        case DRIVE_TYPE_8050:
        case DRIVE_TYPE_8250:
            return 1;
    }

    return 0;
}

static unsigned int drive_check_iec(unsigned int type)
{
    switch (type) {
        case DRIVE_TYPE_1540:
        case DRIVE_TYPE_1541:
        case DRIVE_TYPE_1541II:
        case DRIVE_TYPE_1570:
        case DRIVE_TYPE_1571:
        case DRIVE_TYPE_1571CR:
        case DRIVE_TYPE_1581:
        case DRIVE_TYPE_2000:
        case DRIVE_TYPE_4000:
            return 1;
    }

    return 0;
}

unsigned int drive_check_old(unsigned int type)
{
    switch (type) {
        case DRIVE_TYPE_2040:
        case DRIVE_TYPE_3040:
        case DRIVE_TYPE_4040:
        case DRIVE_TYPE_1001:
        case DRIVE_TYPE_8050:
        case DRIVE_TYPE_8250:
            return 1;
    }

    return 0;
}

unsigned int drive_check_dual(unsigned int type)
{
    switch (type) {
        case DRIVE_TYPE_2040:
        case DRIVE_TYPE_3040:
        case DRIVE_TYPE_4040:
        case DRIVE_TYPE_8050:
        case DRIVE_TYPE_8250:
            return 1;
    }

    return 0;
}

static unsigned int drive_check_tcbm(unsigned int type)
{
    switch (type) {
        case DRIVE_TYPE_1551:
            return 1;
    }

    return 0;
}

unsigned int drive_check_bus(unsigned int drive_type,
                             unsigned int bus_map)
{
    if (drive_type == DRIVE_TYPE_NONE) {
        return 1;
    }

    if (drive_check_ieee(drive_type) && (bus_map & IEC_BUS_IEEE)) {
        return 1;
    }

    if (drive_check_iec(drive_type) && (bus_map & IEC_BUS_IEC)) {
        return 1;
    }

    if (drive_check_tcbm(drive_type) && (bus_map & IEC_BUS_TCBM)) {
        return 1;
    }

    return 0;
}

int drive_check_type(unsigned int drive_type, unsigned int dnr)
{
    if (!drive_check_bus(drive_type, iec_available_busses())) {
        return 0;
    }

    if (drive_check_dual(drive_type)) {
        if (is_drive1(dnr)) {
            /* Dual drives only supported on even device numbers.  */
            return 0;
        } else {
            if (drive_context[mk_drive1(dnr)]->drive->type != DRIVE_TYPE_NONE) {
                /* Disable dual drive if second device is enabled.  */
                return 0;
            }
        }
    } else {
        if (is_drive1(dnr)) {
            if (drive_check_dual(drive_context[mk_drive0(dnr)]->drive->type)) {
                /* Disable second device if dual drive is enabled.  */
                return drive_type == DRIVE_TYPE_NONE;
            }
        }
    }

    if (machine_drive_rom_check_loaded(drive_type) < 0) {
        return 0;
    }

    return 1;
}

int drive_check_expansion(int drive_type)
{
    switch (drive_type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
        return 1;
    }
    return 0;
}

int drive_check_expansion2000(int drive_type)
{
    switch (drive_type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
        return 1;
    }
    return 0;
}

int drive_check_expansion4000(int drive_type)
{
    switch (drive_type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
        return 1;
    }
    return 0;
}

int drive_check_expansion6000(int drive_type)
{
    switch (drive_type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
        return 1;
    }
    return 0;
}

int drive_check_expansion8000(int drive_type)
{
    switch (drive_type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
        return 1;
    }
    return 0;
}

int drive_check_expansionA000(int drive_type)
{
    switch (drive_type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
        return 1;
    }
    return 0;
}

int drive_check_parallel_cable(int drive_type)
{
    switch (drive_type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
        return 1;
    }
    return 0;
}

int drive_check_extend_policy(int drive_type)
{
    switch (drive_type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
    case DRIVE_TYPE_1551:
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
    case DRIVE_TYPE_2031:
        return 1;
    }
    return 0;
}

int drive_check_idle_method(int drive_type)
{
    switch (drive_type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
    case DRIVE_TYPE_1551:
    case DRIVE_TYPE_1581:
    case DRIVE_TYPE_2000:
    case DRIVE_TYPE_4000:
    case DRIVE_TYPE_2031:
    case DRIVE_TYPE_2040:
    case DRIVE_TYPE_3040:
    case DRIVE_TYPE_4040:
        return 1;
    }
    return 0;
}

int drive_check_profdos(int drive_type)
{
    switch (drive_type) {
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
        return 1;
    }
    return 0;
}

int drive_check_supercard(int drive_type)
{
    switch (drive_type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
        return 1;
    }
    return 0;
}

int drive_check_stardos(int drive_type)
{
    switch (drive_type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
 /* FIXME: stardos apparently exists for 157x, needs more research */
 /* case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR: */
        return 1;
    }
    return 0;
}
