/*
 * c64romset.c
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

#include <stdio.h>

#include "romset.h"

const char *machine_romset_resources_list[] = {
    "ChargenIntName",
    "ChargenDEName",
    "ChargenFRName",
    "ChargenSEName",
    "KernalIntName",
    "KernalDEName",
    "KernalFIName",
    "KernalFRName",
    "KernalITName",
    "KernalNOName",
    "KernalSEName",
    "BasicLoName",
    "BasicHiName",
    "Kernal64Name",
    "Basic64Name",
    "DosName1540",
    "DosName1541",
    "DosName1541ii",
    "DosName1570",
    "DosName1571",
    "DosName1571cr",
    "DosName1581",
    "DosName2000",
    "DosName4000",
    "DosName2031",
    "DosName2040",
    "DosName3040",
    "DosName4040",
    "DosName1001",
    NULL
};

int machine_romset_file_load(const char *filename)
{
    return romset_file_load(filename);
}

int machine_romset_file_save(const char *filename)
{
    return romset_file_save(filename, machine_romset_resources_list);
}

char *machine_romset_file_list(void)
{
    return romset_file_list(machine_romset_resources_list);
}

int machine_romset_archive_item_create(const char *romset_name)
{
    return romset_archive_item_create(romset_name, machine_romset_resources_list);
}

void machine_romset_init(void)
{
}
