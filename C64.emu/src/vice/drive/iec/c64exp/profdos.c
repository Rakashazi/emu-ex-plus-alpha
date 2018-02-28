/*
 * profdos.c - Professional DOS emulation.
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
#include <string.h>

#include "cmdline.h"
#include "drive.h"
#include "drivemem.h"
#include "drivetypes.h"
#include "lib.h"
#include "log.h"
#include "profdos.h"
#include "resources.h"
#include "util.h"


#define PROFDOS_ROM_SIZE 0x2000

static BYTE profdos_1571_rom[PROFDOS_ROM_SIZE];

static unsigned int profdos_al[DRIVE_NUM];


int profdos_load_1571(const char *name)
{
    if (util_check_null_string(name)) {
        return 0;
    }

    if (util_file_load(name, profdos_1571_rom,
                       PROFDOS_ROM_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return 0;
}

static BYTE profdos_read(drive_context_t *drv, WORD addr)
{
    return profdos_1571_rom[addr & 0x1fff];
}

static BYTE profdos_read2(drive_context_t *drv, WORD addr)
{
    if (addr >= 0x7000) {
        if (!(addr & 0x0800)) {
            addr = (WORD)((addr & 0xff0f) | (profdos_al[drv->mynumber] << 4));
        } else {
            addr = (WORD)((addr & 0xff00)
                          | (profdos_al[drv->mynumber] << 4) | ((addr >> 4) & 15));
        }

        profdos_al[drv->mynumber] = addr & 15;
    }

    return profdos_1571_rom[addr & 0x1fff];
}

void profdos_mem_init(struct drive_context_s *drv, unsigned int type)
{
    drivecpud_context_t *cpud = drv->cpud;

    if (!drv->drive->profdos) {
        return;
    }

    /* Setup additional profdos rom */
    switch (type) {
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
        drivemem_set_func(cpud, 0x60, 0x70, profdos_read, NULL, NULL, profdos_1571_rom, 0x60006ffd);
        drivemem_set_func(cpud, 0x70, 0x80, profdos_read2, NULL, NULL, NULL, 0);
        break;
    default:
        break;
    }
}

void profdos_init(drive_context_t *drv)
{
}

void profdos_reset(drive_context_t *drv)
{
    profdos_al[drv->mynumber] = 0;
}
