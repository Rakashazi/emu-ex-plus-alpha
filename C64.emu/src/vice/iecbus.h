
/*! \file iecbus.h
 *
 *  \brief IEC bus handling.
 *
 *  \author Andreas Boose <viceteam@t-online.de>
 *
 *  \page iecbus IEC bus handling
 *  \htmlinclude iec-bus.txt
 */

/*
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

#ifndef VICE_IECBUS_H
#define VICE_IECBUS_H

#include "types.h"

#define IECBUS_NUM 16

#define IECBUS_STATUS_TRUEDRIVE      0
#define IECBUS_STATUS_DRIVETYPE      1
#define IECBUS_STATUS_IECDEVICE      2
#define IECBUS_STATUS_VIRTUALDEVICES 3

/*!
 *
 */
enum {
    IECBUS_DEVICE_READ_DATA  = 0x01,
    IECBUS_DEVICE_READ_CLK   = 0x04,
    IECBUS_DEVICE_READ_ATN   = 0x80,

    IECBUS_DEVICE_ATNA       = 0x10,

    IECBUS_DEVICE_WRITE_CLK  = 0x40,
    IECBUS_DEVICE_WRITE_DATA = 0x80
};

typedef struct iecbus_s {
    /*!
     * the drive output ports as described by the
     * IECBUS_DEVICE_WRITE_... macros
     */
    BYTE drv_bus[IECBUS_NUM];

    /*! the drive output ports as seen by the drive */
    BYTE drv_data[IECBUS_NUM];

    /*!
     * the drive input ports, as seen by the drive
     * and also by the IECBUS_DEVICE_READ_... macros
     */
    BYTE drv_port;

    /*!
     * the computer output ports as described by the
     * IECBUS_DEVICE_WRITE_... macros
    */
    BYTE cpu_bus;

    /*! the computer output ports as seen by the computer */
    BYTE cpu_port;

    /*! \todo document */
    BYTE iec_fast_1541;
} iecbus_t;

extern iecbus_t iecbus;

extern iecbus_t *iecbus_drive_port(void);

extern void iecbus_init(void);
extern void iecbus_cpu_undump(BYTE data);
extern void iecbus_status_set(unsigned int type, unsigned int unit,
                              unsigned int enable);

extern BYTE (*iecbus_callback_read)(CLOCK);
extern void (*iecbus_callback_write)(BYTE, CLOCK);

extern BYTE iecbus_device_read(void);
extern int  iecbus_device_write(unsigned int unit, BYTE data);
extern void (*iecbus_update_ports)(void);

#endif
