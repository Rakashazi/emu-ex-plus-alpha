/*
 * printer-uerport.c - Userport printer interface.
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

#include "interface-userport.h"
#include "printer.h"
#include "types.h"


int printer_userport_init_resources(void)
{
    return interface_userport_init_resources();
}

int printer_userport_init_cmdline_options(void)
{
    return interface_userport_init_cmdline_options();
}

void printer_userport_init(void (*set_busy)(unsigned int))
{
    interface_userport_init(set_busy);
}

void printer_userport_write_data(BYTE b)
{
    interface_userport_write_data(b);
}

void printer_userport_write_strobe(int s)
{
    interface_userport_write_strobe(s);
}
