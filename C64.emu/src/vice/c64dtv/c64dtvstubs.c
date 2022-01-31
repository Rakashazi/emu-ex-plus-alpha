
/*
 * c64dtvstubs.c - dummies for unneeded/unused functions
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include <stdlib.h>

#include "machine.h"
#include "tapecart.h"
#include "tapeport.h"


tapeport_desc_t *tapeport_get_valid_devices(int port, int sort)
{
    return NULL;
}

const char *tapeport_get_device_type_desc(int type)
{
    return NULL;
}

int tapeport_valid_port(int port)
{
    return 0;
}

int machine_autodetect_psid(const char *name)
{
    return -1;
}

int tapeport_device_register(int id, tapeport_device_t *device)
{
    return 0;
}

void tapeport_trigger_flux_change(unsigned int on, int port)
{
}

void tapeport_set_tape_sense(int sense, int port)
{
}

int tapecart_is_valid(const char *filename)
{
    return 0;   /* FALSE */
}

int tapecart_attach_tcrt(const char *filename, void *unused)
{
    return -1;
}
