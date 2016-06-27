/*
 * serial-iec.h
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

#ifndef VICE_SERIAL_IEC_H
#define VICE_SERIAL_IEC_H

#include "types.h"

extern int serial_iec_open(unsigned int unit, unsigned int secondary, const char *name, unsigned int length);
extern int serial_iec_close(unsigned int unit, unsigned int secondary);
extern int serial_iec_read(unsigned int unit, unsigned int secondary, BYTE *data);
extern int serial_iec_write(unsigned int unit, unsigned int secondary, BYTE data);
extern int serial_iec_flush(unsigned int unit, unsigned int secondary);

#endif
