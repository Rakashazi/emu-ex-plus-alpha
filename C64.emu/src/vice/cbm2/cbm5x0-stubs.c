/*
 * cbm5x0-stubs.c
 *
 * Written by
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

#include <stdio.h>
#include <stdbool.h>

#include "c64/cart/clockport.h"
#include "cartridge.h"
#include "mididrv.h"
#include "pet/petpia.h"
#include "types.h"
#include "userport.h"
#include "userport_io_sim.h"
#ifdef HAVE_LIBCURL
#include "userport_wic64.h"
#endif


void userport_io_sim_set_pbx_out_lines(uint8_t val)
{
}

#ifdef WINDOWS_COMPILE
void mididrv_ui_reset_device_list(int device)
{
}

char *mididrv_ui_get_next_device_name(int device, int *id)
{
    return NULL;
}
#endif

/*******************************************************************************
    clockport
*******************************************************************************/

clockport_supported_devices_t clockport_supported_devices[] = { { 0, NULL } };

bool pia1_get_diagnostic_pin(void)
{
    return false;
}

/******************************************************************************
 *                                   Userport                                 *
 *****************************************************************************/

int userport_device_register(int id, userport_device_t *device)
{
    return -1;
}

#ifdef HAVE_LIBCURL
const tzones_t *userport_wic64_get_timezones(size_t *num_zones)
{
    return NULL;
}

void userport_wic64_factory_reset(void)
{
}
#endif
