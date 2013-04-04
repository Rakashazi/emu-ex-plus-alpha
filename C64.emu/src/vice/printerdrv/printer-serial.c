/*
 * printer-serial.c - Serial printer interface.
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

#include "interface-serial.h"
#include "printer.h"


int printer_serial_init_resources(void)
{
    return interface_serial_init_resources();
}

int printer_serial_init_cmdline_options(void)
{
    return interface_serial_init_cmdline_options();
}

void printer_serial_init(void)
{
    interface_serial_init();
}

int printer_serial_late_init(void)
{
    return interface_serial_late_init();
}

int printer_serial_close(unsigned int unit)
{
    return interface_serial_close(unit);
}

void printer_serial_shutdown(void)
{
    interface_serial_shutdown();
}
