/*
 * stardos-exp.c - StarDOS emulation.
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
#include "stardos-exp.h"
#include "resources.h"
#include "util.h"

/* #define DEBUGSD */

#ifdef DEBUGSD
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/*
    StarDOS

    - 8k additional ROM at $A000-$Bfff

    to test use:

    x64sc -cartstar StarDosCartRomV1-4-decoded.bin -stardos stardos1541romv1-4-a000-decoded.bin -dos1541 stardos1541romv1-4-e000-decoded.bin -drive8stardos

 */

#define STARDOS_ROM_SIZE 0x2000

static BYTE stardos_rom[STARDOS_ROM_SIZE];

int stardos_exp_load(const char *name)
{
    DBG(("stardos_exp_load <%s>\n", name));

    if (util_check_null_string(name)) {
        return 0;
    }

    if (util_file_load(name, stardos_rom,
                       STARDOS_ROM_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return 0;
}

static BYTE stardos_exp_read(drive_context_t *drv, WORD addr)
{
    DBG(("stardos_exp_read <%04x> <%02x>\n", addr, stardos_rom[addr & 0x1fff]));
    return stardos_rom[addr & 0x1fff];
}

void stardos_exp_mem_init(struct drive_context_s *drv, unsigned int type)
{
    drivecpud_context_t *cpud = drv->cpud;

    DBG(("stardos_exp_mem_init <type:%d> <sc:%d>\n", type, drv->drive->stardos));
    if (!drv->drive->stardos) {
        return;
    }

    /* Setup additional stardos rom */
    switch (type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
 /* FIXME: StarDOS for 157x exists apparently, needs more research */
 /* case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR: */
        drivemem_set_func(cpud, 0xA0, 0xC0, stardos_exp_read, NULL, NULL, stardos_rom, 0xa000bffd);
        break;
    default:
        break;
    }
}

void stardos_exp_init(drive_context_t *drv)
{
}

void stardos_exp_reset(drive_context_t *drv)
{
}
