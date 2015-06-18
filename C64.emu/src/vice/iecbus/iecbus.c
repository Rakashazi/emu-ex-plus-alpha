/*
 * iecbus.c - IEC bus handling.
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

#include <stdio.h>
#include <string.h>

#include "cia.h"
#include "drive.h"
#include "drivetypes.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "printer.h"
#include "via.h"
#include "types.h"
#include "serial.h"


#define IECBUS_DEVICE_NONE      0
#define IECBUS_DEVICE_TRUEDRIVE 1
#define IECBUS_DEVICE_IECDEVICE 2


BYTE (*iecbus_callback_read)(CLOCK) = NULL;
void (*iecbus_callback_write)(BYTE, CLOCK) = NULL;
void (*iecbus_update_ports)(void) = NULL;

iecbus_t iecbus;

static unsigned int iecbus_device[IECBUS_NUM];

static BYTE iec_old_atn = 0x10;


#include "debug.h"

#ifdef DEBUG

#include "log.h"

static void debug_iec_cpu_write(unsigned int data)
{
    if (debug.iec) {
        BYTE value = ~data;

        log_debug("$DD00 store: %s %s %s",
                  value & 0x20 ? "DATA OUT" : "        ",
                  value & 0x10 ? "CLK OUT" : "       ",
                  value & 0x08 ? "ATN OUT" : "       "
                  );
    }
}
# define DEBUG_IEC_CPU_WRITE(_data) debug_iec_cpu_write(_data)

static void debug_iec_cpu_read(unsigned int data)
{
    if (debug.iec) {
        BYTE value = data;

        log_debug("$DD00 read:  %s %s %s %s %s",
                  value & 0x20 ? "DATA OUT" : "        ",
                  value & 0x10 ? "CLK OUT" : "       ",
                  value & 0x08 ? "ATN OUT" : "       ",
                  value & 0x80 ? "       " : "DATA IN",
                  value & 0x40 ? "      " : "CLK IN"
                  );
    }
}
# define DEBUG_IEC_CPU_READ(_data) debug_iec_cpu_read(_data)

void debug_iec_drv_write(unsigned int data)
{
    if (debug.iec) {
        BYTE value = data;
        static BYTE oldvalue = 0;

        if (value != oldvalue) {
            oldvalue = value;

            log_debug("$1800 store: %s %s %s",
                      value & 0x02 ? "DATA OUT" : "        ",
                      value & 0x08 ? "CLK OUT" : "       ",
                      value & 0x10 ? "ATNA   " : "       "
                      );
        }
    }
}

void debug_iec_drv_read(unsigned int data)
{
    if (debug.iec) {
        BYTE value = data;
        static BYTE oldvalue = { 0 };
        const char * data_correct = "";

        if (value != oldvalue) {
            unsigned int atn = value & 0x80 ? 1 : 0;
            unsigned int atna = value & 0x10 ? 1 : 0;
            unsigned int data = value & 0x01 ? 1 : 0;

            oldvalue = value;

            if (atn ^ atna) {
                if (!data) {
                    data_correct = " ***** ERROR: ATN, ATNA & DATA! *****";
                }
            }

            log_debug("$1800 read:  %s %s %s %s %s %s%s",
                      value & 0x02 ? "DATA OUT" : "        ",
                      value & 0x08 ? "CLK OUT" : "       ",
                      value & 0x10 ? "ATNA   " : "       ",

                      value & 0x01 ? "DATA IN" : "       ",
                      value & 0x04 ? "CLK IN" : "       ",
                      value & 0x80 ? "ATN" : "   ",
                      data_correct
                      );
        }
    }
}

void debug_iec_bus_write(unsigned int data)
{
#if 0
    if (debug.iec) {
        BYTE value = data;

        log_debug("  BUS store: %s %s %s",
                  value & 0x02 ? "DATA OUT" : "        ",
                  value & 0x08 ? "CLK OUT" : "       ",
                  value & 0x10 ? "ATNA   " : "       "
                  );
    }
#endif
}

void debug_iec_bus_read(unsigned int data)
{
#if 0
    if (debug.iec) {
        BYTE value = data;

        log_debug("  BUS read:  %s %s %s %s %s %s",
                  value & 0x02 ? "DATA OUT" : "        ",
                  value & 0x08 ? "CLK OUT" : "       ",
                  value & 0x10 ? "ATNA   " : "       ",

                  value & 0x01 ? "DATA IN" : "       ",
                  value & 0x04 ? "CLK IN" : "       ",
                  value & 0x80 ? "ATN" : "   "
                  );
    }
#endif
}

#else

# define DEBUG_IEC_CPU_WRITE(_data)
# define DEBUG_IEC_CPU_READ(_data)

#endif

void iecbus_init(void)
{
    memset(&iecbus, 0xff, sizeof(iecbus_t));
    iecbus.drv_port = IECBUS_DEVICE_READ_DATA
                      | IECBUS_DEVICE_READ_CLK
                      | IECBUS_DEVICE_READ_ATN;
}

void iecbus_cpu_undump(BYTE data)
{
    iec_update_cpu_bus(data);
    iec_old_atn = iecbus.cpu_bus & 0x10;
}

/* No drive is enabled.  */
static BYTE iecbus_cpu_read_conf0(CLOCK clock)
{
    DEBUG_IEC_CPU_READ((iecbus.iec_fast_1541 & 0x30u) << 2);

    return (iecbus.iec_fast_1541 & 0x30u) << 2;
}

static void iecbus_cpu_write_conf0(BYTE data, CLOCK clock)
{
    DEBUG_IEC_CPU_WRITE(data);

    iecbus.iec_fast_1541 = data;
}

/* Only the first drive is enabled.  */
static BYTE iecbus_cpu_read_conf1(CLOCK clock)
{
    drive_cpu_execute_all(clock);

    DEBUG_IEC_CPU_READ(iecbus.cpu_port);

    return iecbus.cpu_port;
}

static void iecbus_cpu_write_conf1(BYTE data, CLOCK clock)
{
    drive_t *drive;

    drive = drive_context[0]->drive;
    drive_cpu_execute_one(drive_context[0], clock);

    DEBUG_IEC_CPU_WRITE(data);

    iec_update_cpu_bus(data);

    if (iec_old_atn != (iecbus.cpu_bus & 0x10)) {
        iec_old_atn = iecbus.cpu_bus & 0x10;
        switch (drive->type) {
            case DRIVE_TYPE_1581:
                if (!iec_old_atn) {
                    ciacore_set_flag(drive_context[0]->cia1581);
                }
                break;
            case DRIVE_TYPE_2000:
            case DRIVE_TYPE_4000:
                viacore_signal(drive_context[0]->via4000, VIA_SIG_CA2,
                               iec_old_atn ? 0 : VIA_SIG_RISE);
                break;
            default:
                viacore_signal(drive_context[0]->via1d1541, VIA_SIG_CA1,
                               iec_old_atn ? 0 : VIA_SIG_RISE);
        }
    }

    switch (drive->type) {
        case DRIVE_TYPE_1581:
        case DRIVE_TYPE_2000:
        case DRIVE_TYPE_4000:
            iecbus.drv_bus[8] = (((iecbus.drv_data[8] << 3) & 0x40)
                                 | ((iecbus.drv_data[8] << 6)
                                    & ((iecbus.drv_data[8] | iecbus.cpu_bus) << 3)
                                    & 0x80));
            break;
        default:
            iecbus.drv_bus[8] = (((iecbus.drv_data[8] << 3) & 0x40)
                                 | ((iecbus.drv_data[8] << 6)
                                    & ((~iecbus.drv_data[8] ^ iecbus.cpu_bus) << 3)
                                    & 0x80));
    }
    iec_update_ports();
}

/* Only the second drive is enabled.  */
static BYTE iecbus_cpu_read_conf2(CLOCK clock)
{
    drive_cpu_execute_all(clock);

    DEBUG_IEC_CPU_READ(iecbus.cpu_port);

    return iecbus.cpu_port;
}

static void iecbus_cpu_write_conf2(BYTE data, CLOCK clock)
{
    drive_t *drive;

    drive = drive_context[1]->drive;
    drive_cpu_execute_one(drive_context[1], clock);

    DEBUG_IEC_CPU_WRITE(data);

    iec_update_cpu_bus(data);

    if (iec_old_atn != (iecbus.cpu_bus & 0x10)) {
        iec_old_atn = iecbus.cpu_bus & 0x10;
        switch (drive->type) {
            case DRIVE_TYPE_1581:
                if (!iec_old_atn) {
                    ciacore_set_flag(drive_context[1]->cia1581);
                }
                break;
            case DRIVE_TYPE_2000:
            case DRIVE_TYPE_4000:
                viacore_signal(drive_context[1]->via4000, VIA_SIG_CA2,
                               iec_old_atn ? 0 : VIA_SIG_RISE);
                break;
            default:
                viacore_signal(drive_context[1]->via1d1541, VIA_SIG_CA1,
                               iec_old_atn ? 0 : VIA_SIG_RISE);
        }
    }

    switch (drive->type) {
        case DRIVE_TYPE_1581:
        case DRIVE_TYPE_2000:
        case DRIVE_TYPE_4000:
            iecbus.drv_bus[9] = (((iecbus.drv_data[9] << 3) & 0x40)
                                 | ((iecbus.drv_data[9] << 6)
                                    & ((iecbus.drv_data[9] | iecbus.cpu_bus) << 3)
                                    & 0x80));
            break;
        default:
            iecbus.drv_bus[9] = (((iecbus.drv_data[9] << 3) & 0x40)
                                 | ((iecbus.drv_data[9] << 6)
                                    & ((~iecbus.drv_data[9] ^ iecbus.cpu_bus) << 3)
                                    & 0x80));
    }

    iec_update_ports();
}

static BYTE iecbus_cpu_read_conf3(CLOCK clock)
{
    drive_cpu_execute_all(clock);
    serial_iec_device_exec(clock);

    DEBUG_IEC_CPU_READ(iecbus.cpu_port);

    return iecbus.cpu_port;
}

static void iecbus_cpu_write_conf3(BYTE data, CLOCK clock)
{
    unsigned int dnr;

    drive_cpu_execute_all(clock);
    serial_iec_device_exec(clock);

    DEBUG_IEC_CPU_WRITE(data);

    iec_update_cpu_bus(data);

    if (iec_old_atn != (iecbus.cpu_bus & 0x10)) {
        iec_old_atn = iecbus.cpu_bus & 0x10;

        for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
            if (iecbus_device[8 + dnr] == IECBUS_DEVICE_TRUEDRIVE) {
                switch (drive_context[dnr]->drive->type) {
                    case DRIVE_TYPE_1581:
                        if (!iec_old_atn) {
                            ciacore_set_flag(drive_context[dnr]->cia1581);
                        }
                        break;
                    case DRIVE_TYPE_2000:
                    case DRIVE_TYPE_4000:
                        viacore_signal(drive_context[dnr]->via4000, VIA_SIG_CA2,
                                       iec_old_atn ? 0 : VIA_SIG_RISE);
                        break;
                    default:
                        viacore_signal(drive_context[dnr]->via1d1541, VIA_SIG_CA1,
                                       iec_old_atn ? 0 : VIA_SIG_RISE);
                }
            }
        }
    }

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        if (iecbus_device[8 + dnr] == IECBUS_DEVICE_TRUEDRIVE) {
            unsigned int unit;
            unit = dnr + 8;
            switch (drive_context[dnr]->drive->type) {
                case DRIVE_TYPE_1581:
                case DRIVE_TYPE_2000:
                case DRIVE_TYPE_4000:
                    iecbus.drv_bus[unit] = (((iecbus.drv_data[unit] << 3) & 0x40)
                                            | ((iecbus.drv_data[unit] << 6)
                                               & ((iecbus.drv_data[unit]
                                                   | iecbus.cpu_bus) << 3) & 0x80));
                    break;
                default:
                    iecbus.drv_bus[unit] = (((iecbus.drv_data[unit] << 3) & 0x40)
                                            | ((iecbus.drv_data[unit] << 6)
                                               & ((~iecbus.drv_data[unit]
                                                   ^ iecbus.cpu_bus) << 3) & 0x80));
            }
        }
    }

    iec_update_ports();
}

static void calculate_callback_index(void)
{
    unsigned int callback_index;

    callback_index = (iecbus_device[8] << 0)
                     | (iecbus_device[9] << 2)
                     | (iecbus_device[10] << 6)
                     | (iecbus_device[11] << 8)
                     | (iecbus_device[4] << 10)
                     | (iecbus_device[5] << 12)
                     | (iecbus_device[6] << 14)
                     | (iecbus_device[7] << 16);

    switch (callback_index) {
        case 0:
            iecbus_callback_read = iecbus_cpu_read_conf0;
            iecbus_callback_write = iecbus_cpu_write_conf0;
            break;
        case IECBUS_DEVICE_TRUEDRIVE << 0:
                iecbus_callback_read = iecbus_cpu_read_conf1;
            iecbus_callback_write = iecbus_cpu_write_conf1;
            break;
        case IECBUS_DEVICE_TRUEDRIVE << 2:
                iecbus_callback_read = iecbus_cpu_read_conf2;
            iecbus_callback_write = iecbus_cpu_write_conf2;
            break;
        default:
            iecbus_callback_read = iecbus_cpu_read_conf3;
            iecbus_callback_write = iecbus_cpu_write_conf3;
            break;
    }
}

/*

iecbus_status_set() sets IEC bus devices according to the following table:

TDE DE ID VD                                       iecbus_device
 0  0  0  0  nothing enabled                       IECBUS_DEVICE_NONE
 0  0  0  1  trap device enabled                   IECBUS_DEVICE_NONE
 0  0  1  0  IEC device enabled                    IECBUS_DEVICE_IECDEVICE
 0  0  1  1  IEC device enabled+trap dev. enabled  IECBUS_DEVICE_IECDEVICE
 0  1  0  0  nothing enabled                       IECBUS_DEVICE_NONE
 0  1  0  1  trap device enabled                   IECBUS_DEVICE_NONE
 0  1  1  0  IEC device enabled                    IECBUS_DEVICE_IECDEVICE
 0  1  1  1  IEC device enabled+trap dev. enabled  IECBUS_DEVICE_IECDEVICE
--------------------------------------------------------------------------
 1  0  0  0  nothing enabled                       IECBUS_DEVICE_NONE
 1  0  0  1  nothing enabled                       IECBUS_DEVICE_NONE
 1  0  1  0  IEC device enabled                    IECBUS_DEVICE_IECDEVICE
 1  0  1  1  IEC device enabled                    IECBUS_DEVICE_IECDEVICE
 1  1  0  0  TDE drive enabled                     IECBUS_DEVICE_TRUEDRIVE
 1  1  0  1  TDE drive enabled                     IECBUS_DEVICE_TRUEDRIVE
 1  1  1  0  IEC device enabled                    IECBUS_DEVICE_IECDEVICE
 1  1  1  1  IEC device enabled                    IECBUS_DEVICE_IECDEVICE

TDE = true drive emulation (global switch)
DE = device enable (device switch)
ID = IEC devices (device switch)
VD = virtual devices (global switch)

*/

static const unsigned int iecbus_device_index[16] = {
    IECBUS_DEVICE_NONE,
    IECBUS_DEVICE_NONE,
    IECBUS_DEVICE_IECDEVICE,
    IECBUS_DEVICE_IECDEVICE,
    IECBUS_DEVICE_NONE,
    IECBUS_DEVICE_NONE,
    IECBUS_DEVICE_IECDEVICE,
    IECBUS_DEVICE_IECDEVICE,
    IECBUS_DEVICE_NONE,
    IECBUS_DEVICE_NONE,
    IECBUS_DEVICE_IECDEVICE,
    IECBUS_DEVICE_IECDEVICE,
    IECBUS_DEVICE_TRUEDRIVE,
    IECBUS_DEVICE_TRUEDRIVE,
    IECBUS_DEVICE_IECDEVICE,
    IECBUS_DEVICE_IECDEVICE
};

void iecbus_status_set(unsigned int type, unsigned int unit,
                       unsigned int enable)
{
    static unsigned int truedrive, drivetype[IECBUS_NUM], iecdevice[IECBUS_NUM],
                        virtualdevices;
    unsigned int dev;

    switch (type) {
        case IECBUS_STATUS_TRUEDRIVE:
            truedrive = enable ? (1 << 3) : 0;
            break;
        case IECBUS_STATUS_DRIVETYPE:
            drivetype[unit] = enable ? (1 << 2) : 0;
            break;
        case IECBUS_STATUS_IECDEVICE:
            iecdevice[unit] = enable ? (1 << 1) : 0;
            break;
        case IECBUS_STATUS_VIRTUALDEVICES:
            virtualdevices = enable ? (1 << 0) : 0;
            break;
    }

    for (dev = 0; dev < IECBUS_NUM; dev++) {
        unsigned int index;

        index = truedrive | drivetype[dev] | iecdevice[dev] | virtualdevices;
        iecbus_device[dev] = iecbus_device_index[index];
    }

    calculate_callback_index();
}


BYTE iecbus_device_read(void)
{
    return iecbus.drv_port;
}


int iecbus_device_write(unsigned int unit, BYTE data)
{
    if (unit < IECBUS_NUM) {
        iecbus.drv_bus[unit] = data;
        if (iecbus_update_ports) {
            (*iecbus_update_ports)();
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
