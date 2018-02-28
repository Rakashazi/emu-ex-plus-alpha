/*
 * vic20printer.c
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

#include "machine-printer.h"
#include "printer.h"
#include "via.h"
#include "vic20.h"


void machine_printer_setup_context(struct machine_context_s *machine_context)
{
}

int machine_printer_resources_init(void)
{
    if (printer_serial_init_resources() < 0 || printer_userport_init_resources()) {
        return -1;
    }
    return 0;
}

void machine_printer_resources_shutdown(void)
{
}

int machine_printer_cmdline_options_init(void)
{
    if (printer_serial_init_cmdline_options() < 0 || printer_userport_init_cmdline_options() < 0) {
        return -1;
    }
    return 0;
}

void machine_printer_init(void)
{
    printer_serial_init();
}

void machine_printer_shutdown(void)
{
    printer_serial_shutdown();
}
