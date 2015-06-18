/*
 * supercard.c - Supercard+ emulation.
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

#include <stdio.h>
#include <string.h>

#include "cmdline.h"
#include "drive.h"
#include "drivemem.h"
#include "drivetypes.h"
#include "lib.h"
#include "log.h"
#include "supercard.h"
#include "resources.h"
#include "util.h"

/* #define DEBUGSC */

#ifdef DEBUGSC
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/*
    Supercard+ by Jim Drew

    - 8k additional RAM at $6000-$7fff
    - 2k ROM at $1000-$17ff (note: the ROM is actually 8k, only first 2k used)

    FIXME: supercard supports an additional index hole sensor in the 1541, and
           also the one in the 1571 - how that works is currently unknown.

    to test use: x64 -supercard supercard.rom -drive8ram6000 -drive8supercard

 */

#define SUPERCARD_ROM_SIZE 0x2000

static BYTE supercard_rom[SUPERCARD_ROM_SIZE];

int supercard_load(const char *name)
{
    DBG(("supercard_load <%s>\n", name));

    if (util_check_null_string(name)) {
        return 0;
    }

    if (util_file_load(name, supercard_rom,
                       SUPERCARD_ROM_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return 0;
}

static BYTE supercard_read(drive_context_t *drv, WORD addr)
{
    DBG(("supercard_read <%04x> <%02x>\n", addr, supercard_rom[addr & 0x07ff]));
    return supercard_rom[addr & 0x07ff];
}

void supercard_mem_init(struct drive_context_s *drv, unsigned int type)
{
    drivecpud_context_t *cpud = drv->cpud;

    DBG(("supercard_mem_init <type:%d> <sc:%d>\n", type, drv->drive->supercard));

    if (!drv->drive->supercard) {
        return;
    }

    /* Setup additional supercard rom */
    switch (type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
        drivemem_set_func(cpud, 0x10, 0x18, supercard_read, NULL, supercard_rom, 0x100017fd);
        break;
    default:
        break;
    }
}

void supercard_init(drive_context_t *drv)
{
}

void supercard_reset(drive_context_t *drv)
{
}
