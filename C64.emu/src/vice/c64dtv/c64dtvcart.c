/*
 * c64dtvcart.c - C64 cartridge emulation stubs.
 *
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
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
#include "types.h"

#include "c64cart.h"

/* Expansion port signals. */
export_t export = { 0, 0, 0, 0};

static BYTE romh_banks[1]; /* dummy */

int cartridge_save_image(int type, const char *filename)
{
    return 0;
}

int cartridge_resources_init(void)
{
    return 0;
}

void cartridge_resources_shutdown(void)
{
}

int cartridge_cmdline_options_init(void)
{
    return 0;
}

int cartridge_attach_image(int type, const char *filename)
{
    return 0;
}

void cartridge_detach_image(int type)
{
}

void cartridge_set_default(void)
{
}

void cartridge_init(void)
{
}

void cartridge_trigger_freeze(void)
{
}

void cartridge_trigger_freeze_nmi_only(void)
{
}

const char *cartridge_get_file_name(int type)
{
    return 0; /* NULL */
}

BYTE *ultimax_romh_phi1_ptr(WORD addr)
{
    return romh_banks;
}

BYTE *ultimax_romh_phi2_ptr(WORD addr)
{
    return romh_banks;
}
