
/*
 * pet-stubs.c - dummies for unneeded/unused functions
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#include "c64/cart/clockport.h"
#include "cartridge.h"
#include "mididrv.h"
#ifdef HAVE_LIBCURL
#include "userport_wic64.h"
#endif

/* dummy function to satisfy the global cartridge system */
int cartridge_attach_image(int type, const char *name)
{
    return -1;
}

void cartridge_detach_image(int type)
{
}

int cartridge_save_image(int type, const char *filename)
{
    return -1;
}

int cartridge_save_secondary_image(int type, const char *filename)
{
    return -1;
}

int cartridge_flush_image(int type)
{
    return -1;
}

int cartridge_flush_secondary_image(int type)
{
    return -1;
}

int cartridge_can_save_image(int crtid)
{
    return 0;
}

int cartridge_can_flush_image(int crtid)
{
    return 0;
}

int cartridge_can_save_secondary_image(int crtid)
{
    return 0;
}

int cartridge_can_flush_secondary_image(int crtid)
{
    return 0;
}

int cartridge_enable(int crtid)
{
    return -1;
}

int cartridge_disable(int crtid)
{
    return -1;
}

int cartridge_type_enabled(int crtid)
{
    return 0;
}

void cartridge_set_default(void)
{
}

void cartridge_unset_default(void)
{
}

cartridge_info_t *cartridge_get_info_list(void)
{
    return NULL;
}

/* return cartridge type of main slot
   returns 0 (CARTRIDGE_CRT) if crt file */
int cartridge_get_id(int slot)
{
    return CARTRIDGE_NONE;
}

char *cartridge_get_filename_by_slot(int slot)
{
    return NULL;
}

char *cartridge_get_secondary_filename_by_slot(int slot)
{
    return NULL;
}

void cartridge_trigger_freeze(void)
{
}

/* dummy functions needed for the UI */
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
