/*
 * catweaselmkiii.h
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

#ifndef VICE_CATWEASELMKIII_H
#define VICE_CATWEASELMKIII_H

#include "types.h"

extern int catweaselmkiii_open(void);
extern int catweaselmkiii_close(void);
extern int catweaselmkiii_read(WORD addr, int chipno);
extern void catweaselmkiii_store(WORD addr, BYTE val, int chipno);
extern void catweaselmkiii_set_machine_parameter(long cycles_per_sec);

extern int catweaselmkiii_available(void);

#define CW_VENDOR           0xe159
#define CW_DEVICE           0x0001
#define CW_MK3_SUBVENDOR    0x1212
#define CW_MK3_SUBDEVICE    0x0002
#define CW_MK4_SUBVENDOR1   0x5213
#define CW_MK4_SUBVENDOR2   0x5200
#define CW_MK4_SUBDEVICE1   0x0002
#define CW_MK4_SUBDEVICE2   0x0003

// generic registers
#define CW_DATA_DIRECTION   0x02
#define CW_SELECT_BANK      0x03
#define CW_PORT_OUT_DIR     CW_SELECT_BANK
#define CW_PORT_AUX         0x05
#define CW_PORT_IN_DIR      0x07

// values for CW_SELECT_BANK
#define CW_BANK_RESETFPGA   0x00
#define CW_BANK_FIFO        0x01
#define CW_BANK_FLOPPY      0x41
#define CW_BANK_IO          0x81
#define CW_BANK_IRQ         0xC1

// registers in FLOPPY bank
#define CW_FLOPPY_JOYDAT        0xC0
#define CW_FLOPPY_JOYBUT        0xC8
#define CW_FLOPPY_JOYBUT_DIR    0xCC
#define CW_FLOPPY_KEYDAT        0xD0
#define CW_FLOPPY_KEYSTATUS     0xD4
#define CW_FLOPPY_SIDDAT        0xD8
#define CW_FLOPPY_SIDADDR       0xDC
#define CW_FLOPPY_MEMORY        0xE0
#define CW_FLOPPY_RESETPOINTER  0xE4
#define CW_FLOPPY_CONTROL       0xE8
#define CW_FLOPPY_OPTION        0xEC
#define CW_FLOPPY_START_A       0xF0
#define CW_FLOPPY_START_B       0xF4
#define CW_FLOPPY_IRQ           0xFC

// registers in IO bank
#define CW_IO_MOUSEY1    0xC0
#define CW_IO_MOUSEX1    0xC4
#define CW_IO_MOUSEY2    0xC8
#define CW_IO_MOUSEX2    0xCC
#define CW_IO_BUTTON     0xD0

// registers in CW_BANK_IRQ
#define CW_IRQ_R0    0xC0
#define CW_IRQ_R1    0xC4
#define CW_IRQ_M0    0xC8
#define CW_IRQ_M1    0xCC

// bits in registers in CW_BANK_IRQ
#define CW_IRQ_MK3FLOPPY              0x01
#define CW_IRQ_INDEX                  0x02
#define CW_IRQ_FLOPPY_START           0x04
#define CW_IRQ_FLOPPY_END             0x08
#define CW_IRQ_SID_FIFO_HALF_EMPTY    0x10
#define CW_IRQ_SID_READ               0x20
#define CW_IRQ_MEM_EQUAL              0x40
#define CW_IRQ_KEYBOARD               0x80
#define CW_IRQ_SID_FIFO_EMPTY         0x01
#define CW_IRQ_SID_FEEDBACK           0x02

#endif
