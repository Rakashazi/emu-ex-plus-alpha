/*
 * masceruade-stubs.c - C64 expansion port stubs handling for the VIC20 masC=uerade adapter.
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
#include <string.h>

#include "mascuerade-stubs.h"

export_list_t *c64export_query_list(export_list_t *item)
{
    return item;
}

void c64export_dump(void)
{
}

int c64export_add(const c64export_resource_t *export_res)
{
    return 0;
}

int c64export_remove(const c64export_resource_t *export_res)
{
    return 0;
}

int c64export_resources_init(void)
{
    return 0;
}

int mmc64_cart_enabled(void)
{
    return 0;
}

int mmcreplay_cart_enabled(void)
{
    return 0;
}

int retroreplay_cart_enabled(void)
{
    return 0;
}

int cartridge_type_enabled(int type)
{
    return 0;
}

int cartridge_flush_image(int type)
{
    return 0;
}

int cartridge_save_image(int type, const char *filename)
{
    return 0;
}

int mmc64_clockport_enabled = 0;
int mmc64_hw_clockport = 0;
int rr_clockport_enabled = 0;
int mmcr_clockport_enabled = 0;
